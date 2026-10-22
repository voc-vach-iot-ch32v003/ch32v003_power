#include "ch32v003_power.h"
#include "ch32v003_gpio.h"

// Bit cao nhất của SWCFG[2:0] trong AFIO_PCFR1: 1 = tắt SWD (giống ch32v003_gpio)
#define POWER_AFIO_SWD_DISABLE (1UL << 26)

// Đường EXTI 9 nối sẵn với khối tự thức dậy (AWU) của PWR
#define POWER_EXTI_LINE_AWU (1UL << 9)

// Cửa sổ so sánh AWU 6 bit: chu kỳ = (AWUWR + 1) * hệ số chia / LSI
#define POWER_AWU_WINDOW_MAX 63U
#define POWER_AWU_PRESCALER_COUNT 14U

// Mã AWUPSC 0x2..0xF tương ứng hệ số chia 2, 4, ..., 4096, 10240, 61440
static const uint16_t awuPrescalers[POWER_AWU_PRESCALER_COUNT] = {
    2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 10240, 61440
};

/**
 * @brief Một lần hẹn giờ AWU đã tính sẵn: mã hệ số chia + giá trị cửa sổ.
 */
typedef struct
{
    uint8_t prescalerCode;
    uint8_t window;
} PowerAwuSetting_t;

/**
 * @brief Trạng thái nguồn đánh thức (toàn chip chỉ có một khối PWR nên dùng một bản duy nhất).
 */
typedef struct
{
    uint8_t pinLines; // Mặt nạ đường EXTI 0-7 của các chân đánh thức
    uint32_t gpioClocks; // Clock các Port chứa chân đánh thức (giữ lại trong lúc ngủ)
    uint32_t fullChunks; // Số lần hẹn giờ dài nhất (~30s) phải chạy trước
    PowerAwuSetting_t full; // Cấu hình cho mỗi lần dài nhất
    PowerAwuSetting_t rest; // Cấu hình cho phần lẻ còn lại (window = 0xFF nếu không có)
} PowerWakeupState_t;

static PowerWakeupState_t wakeupState = {.rest = {0, 0xFF}};

// ============================================================================
// TÍNH THỜI GIAN HẸN GIỜ AWU (chạy lúc cấu hình, clock đầy đủ)
// ============================================================================

// Chọn hệ số chia nhỏ nhất đủ chứa số nhịp LSI yêu cầu -> độ phân giải tốt nhất, không bao giờ ngủ thiếu
static PowerAwuSetting_t powerCalculateAwu(const uint32_t ticks)
{
    for (uint8_t i = 0; i < POWER_AWU_PRESCALER_COUNT; i++)
    {
        const uint32_t windowPlusOne = (ticks + awuPrescalers[i] - 1) / awuPrescalers[i];
        if (windowPlusOne <= POWER_AWU_WINDOW_MAX + 1)
        {
            const PowerAwuSetting_t setting = {(uint8_t)(i + 2), (uint8_t)(windowPlusOne ? windowPlusOne - 1 : 0)};
            return setting;
        }
    }
    const PowerAwuSetting_t longest = {POWER_AWU_PRESCALER_COUNT + 1, POWER_AWU_WINDOW_MAX};
    return longest;
}

static void powerPrepareTimer(const uint32_t timeoutMs)
{
    const uint32_t maxTicks = (POWER_AWU_WINDOW_MAX + 1) * (uint32_t)awuPrescalers[POWER_AWU_PRESCALER_COUNT - 1];
    const uint64_t ticks = ((uint64_t)timeoutMs * POWER_LSI_FREQUENCY_HZ + 999) / 1000;

    wakeupState.fullChunks = (uint32_t)(ticks / maxTicks);
    wakeupState.full = powerCalculateAwu(maxTicks);
    wakeupState.rest.window = 0xFF;
    const uint32_t restTicks = (uint32_t)(ticks % maxTicks);
    if (restTicks)
    {
        wakeupState.rest = powerCalculateAwu(restTicks);
    }

    RCC->APB1PCENR |= RCC_APB1Periph_PWR;
    RCC->RSTSCKR |= RCC_LSION;
    while (!(RCC->RSTSCKR & RCC_LSIRDY))
    {
    }
}

static uint8_t powerTimerEnabled(void)
{
    return wakeupState.fullChunks || wakeupState.rest.window != 0xFF;
}

// ============================================================================
// CẤU HÌNH NGUỒN ĐÁNH THỨC
// ============================================================================

void powerConfigureWakeup(const POWER_WakeupConfiguration_t* config)
{
    // 1. Xóa sạch cấu hình cũ (chỉ các đường EXTI thư viện từng dùng)
    const uint32_t oldLines = wakeupState.pinLines | POWER_EXTI_LINE_AWU;
    EXTI->EVENR &= ~oldLines;
    EXTI->INTENR &= ~oldLines;
    EXTI->FTENR &= ~oldLines;
    EXTI->RTENR &= ~oldLines;
    EXTI->INTFR = oldLines;
    PWR->AWUCSR &= ~PWR_AWUCSR_AWUEN;
    wakeupState.pinLines = 0;
    wakeupState.gpioClocks = 0;
    wakeupState.fullChunks = 0;
    wakeupState.rest.window = 0xFF;

    if (config == NULL) return;

    // 2. Chân đánh thức: Input kéo nội ngược chiều sườn, nối Port vào đường EXTI cùng số chân
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO;
    for (uint8_t i = 0; i < config->pinCount; i++)
    {
        const uint8_t mcuPin = config->pins[i].mcuPin;
        if (!MCU_PIN_IS_GPIO(mcuPin)) continue;

        const uint8_t port = MCU_PIN_GET_PORT(mcuPin); // 0 = A, 1 = C, 2 = D
        const uint8_t line = MCU_PIN_GET_NUM(mcuPin);
        const uint32_t mask = 1UL << line;
        const POWER_WakeupTrigger_t trigger = config->pins[i].trigger;

        pinMode(mcuPin, trigger == POWER_WAKEUP_FALLING ? INPUT_PULLUP : INPUT_PULLDOWN);
        wakeupState.gpioClocks |= RCC_APB2Periph_GPIOA << (port ? port + 1 : 0);

        // EXTICR: 2 bit mỗi đường, mã Port A = 0, C = 2, D = 3
        AFIO->EXTICR = (AFIO->EXTICR & ~(0x3UL << (line * 2))) | ((uint32_t)(port ? port + 1 : 0) << (line * 2));

        if (trigger != POWER_WAKEUP_RISING) EXTI->FTENR |= mask;
        if (trigger != POWER_WAKEUP_FALLING) EXTI->RTENR |= mask;
        EXTI->EVENR |= mask; // Sự kiện đánh thức WFE
        EXTI->INTENR |= mask; // Chỉ để có cờ INTFR phân biệt chân với hẹn giờ (thư viện không bật NVIC)
        wakeupState.pinLines |= mask;
    }

    // 3. Hẹn giờ: tính sẵn cấu hình AWU, bật LSI
    if (config->timeoutMs)
    {
        powerPrepareTimer(config->timeoutMs);
    }
}

// ============================================================================
// VÀO / RA CHẾ ĐỘ NGỦ
// ============================================================================

// Chờ một lần hẹn giờ AWU. Trả về 1 nếu bị chân đánh thức cắt ngang
static uint8_t powerWaitTimer(const PowerAwuSetting_t setting)
{
    PWR->AWUCSR &= ~PWR_AWUCSR_AWUEN; // Tắt rồi bật lại để bộ đếm AWU bắt đầu từ 0
    PWR->AWUPSC = (PWR->AWUPSC & ~0xFUL) | setting.prescalerCode;
    PWR->AWUWR = (PWR->AWUWR & ~0x3FUL) | setting.window;
    EXTI->EVENR |= POWER_EXTI_LINE_AWU;
    EXTI->FTENR |= POWER_EXTI_LINE_AWU;
    EXTI->INTFR = POWER_EXTI_LINE_AWU;
    PWR->AWUCSR |= PWR_AWUCSR_AWUEN;

    __WFE();
    return (EXTI->INTFR & wakeupState.pinLines) != 0;
}

static void powerWaitForWakeup(void)
{
    if (!powerTimerEnabled())
    {
        // Chỉ có chân: bỏ qua các lần thức không do chân (sự kiện lạ) và ngủ tiếp
        do
        {
            __WFE();
        } while (!(EXTI->INTFR & wakeupState.pinLines));
        return;
    }

    uint8_t interrupted = 0;
    for (uint32_t i = 0; i < wakeupState.fullChunks && !interrupted; i++)
    {
        interrupted = powerWaitTimer(wakeupState.full);
    }
    if (!interrupted && wakeupState.rest.window != 0xFF)
    {
        powerWaitTimer(wakeupState.rest);
    }

    PWR->AWUCSR &= ~PWR_AWUCSR_AWUEN;
    EXTI->EVENR &= ~POWER_EXTI_LINE_AWU;
    EXTI->FTENR &= ~POWER_EXTI_LINE_AWU;
}

static uint8_t powerEnterLowPower(const uint8_t deep)
{
    if (!wakeupState.pinLines && !powerTimerEnabled()) return 0;

    // 1. Lưu trạng thái cần khôi phục
    const uint32_t apb1Clocks = RCC->APB1PCENR;
    const uint32_t apb2Clocks = RCC->APB2PCENR;
    const uint32_t clockConfig = RCC->CFGR0;
    const uint32_t pllOn = RCC->CTLR & RCC_PLLON;
    const uint32_t sysTickControl = SysTick->CTLR;
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO;
    const uint32_t swdDisabled = AFIO->PCFR1 & POWER_AFIO_SWD_DISABLE;

#if POWER_SLEEP_DISABLE_SWD
    AFIO->PCFR1 |= POWER_AFIO_SWD_DISABLE;
#endif

    // 2. Dừng SysTick (ngắt 1ms của ch32v003_timer sẽ đánh thức CPU liên tục), chỉ giữ clock cần cho việc thức
    SysTick->CTLR &= ~1UL;
    RCC->APB1PCENR = RCC_APB1Periph_PWR;
    RCC->APB2PCENR = wakeupState.gpioClocks | RCC_APB2Periph_AFIO;

    if (deep)
    {
        PWR->CTLR |= PWR_CTLR_PDDS;
        PFIC->SCTLR |= (1 << 2); // SLEEPDEEP
    }
    else
    {
        // Sleep: chạy HSI / 256 (~94kHz), tắt PLL
        RCC->CFGR0 = (RCC->CFGR0 & ~RCC_SW) | RCC_SW_HSI;
        while ((RCC->CFGR0 & RCC_SWS) != RCC_SWS_HSI)
        {
        }
        RCC->CTLR &= ~RCC_PLLON;
        RCC->CFGR0 |= RCC_HPRE;
        PWR->CTLR &= ~PWR_CTLR_PDDS;
        PFIC->SCTLR &= ~(1 << 2);
    }

    // 3. Ngủ tới khi có nguồn đánh thức
    EXTI->INTFR = wakeupState.pinLines | POWER_EXTI_LINE_AWU;
    powerWaitForWakeup();
    EXTI->INTFR = wakeupState.pinLines | POWER_EXTI_LINE_AWU;

    // 4. Khôi phục: bỏ deep sleep, bật lại PLL nếu trước đó có dùng, rồi trả nguyên cấu hình clock
    PFIC->SCTLR &= ~(1 << 2);
    PWR->CTLR &= ~PWR_CTLR_PDDS;
    RCC->CFGR0 = (RCC->CFGR0 & ~RCC_HPRE) | (clockConfig & RCC_HPRE);
    if (pllOn)
    {
        RCC->CTLR |= RCC_PLLON;
        while (!(RCC->CTLR & RCC_PLLRDY))
        {
        }
    }
    RCC->CFGR0 = clockConfig;
    while ((RCC->CFGR0 & RCC_SWS) != ((clockConfig & RCC_SW) << 2))
    {
    }

    RCC->APB1PCENR = apb1Clocks;
    RCC->APB2PCENR = apb2Clocks | RCC_APB2Periph_AFIO;
    AFIO->PCFR1 = (AFIO->PCFR1 & ~POWER_AFIO_SWD_DISABLE) | swdDisabled;
    RCC->APB2PCENR = apb2Clocks;
    SysTick->CTLR = sysTickControl;
    return 1;
}

uint8_t powerSleep(void)
{
    return powerEnterLowPower(0);
}

uint8_t powerStandby(void)
{
    return powerEnterLowPower(1);
}
