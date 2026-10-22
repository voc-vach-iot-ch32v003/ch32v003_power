#include "ch32fun.h"
#include "ch32v003_debug.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_power.h"
#include "user_config.h"

// ============================================================================
// VÍ DỤ: Standby (~10µA), thức dậy bằng nút nhấn HOẶC tự thức sau WAKEUP_PERIOD_MS.
// CH32V003 thức dậy từ Standby vẫn giữ RAM và chạy tiếp ngay sau powerStandby()
// (không reset): bộ đếm số lần thức được in ra qua SWIO tăng dần.
// ============================================================================

static const POWER_WakeupPin_t wakeupPins[] = {
    {WAKEUP_BUTTON_PIN, POWER_WAKEUP_FALLING},
};

static const POWER_WakeupConfiguration_t wakeupConfig = {
    .pins = wakeupPins,
    .pinCount = 1,
    .timeoutMs = WAKEUP_PERIOD_MS, // Nguồn nào xảy ra trước thì thức trước
};

static uint16_t wakeCount = 0;

static void setup(void)
{
    pinMode(LED_PIN, OUTPUT);
    powerConfigureWakeup(&wakeupConfig);
    printf("Standby demo: nhan nut hoac doi %u ms\r\n", WAKEUP_PERIOD_MS);
}

static void loop(void)
{
    digitalWrite(LED_PIN, HIGH);
    delayMs(200);
    digitalWrite(LED_PIN, LOW);

    printf("Ngu lan %u...\r\n", wakeCount);
    delayMs(10); // Cho dòng printf truyền xong qua SWIO trước khi SWD tạm tắt trong lúc ngủ

    powerStandby(); // Thức dậy chạy tiếp ngay dòng dưới, RAM giữ nguyên
    wakeCount++;
    printf("Thuc day! wakeCount = %u\r\n", wakeCount);
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Khoảng chờ an toàn tránh brick chip

    setup();
    while (1)
    {
        loop();
    }
}
