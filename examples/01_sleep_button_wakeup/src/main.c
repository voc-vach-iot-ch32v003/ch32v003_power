#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_power.h"
#include "user_config.h"

// ============================================================================
// VÍ DỤ: Sleep giữ RAM, đánh thức bằng nút nhấn.
// Mỗi lần nhấn nút, chip thức dậy, chớp LED rồi ngủ lại. Chip chạy tiếp ngay sau
// powerSleep() nên biến đếm trong RAM tăng dần qua các lần thức.
// ============================================================================

static const POWER_WakeupPin_t wakeupPins[] = {
    {WAKEUP_BUTTON_PIN, POWER_WAKEUP_FALLING}, // Thư viện tự bật Pull-up nội cho sườn xuống
};

static const POWER_WakeupConfiguration_t wakeupConfig = {
    .pins = wakeupPins,
    .pinCount = 1,
    .timeoutMs = 0, // Chỉ thức bằng nút
};

static uint8_t wakeCount = 0;

static void setup(void)
{
    pinMode(LED_PIN, OUTPUT);
    powerConfigureWakeup(&wakeupConfig);
}

static void loop(void)
{
    // Chớp LED số lần bằng số lần đã thức (1, 2, 3... quay vòng sau 5) -> chứng minh RAM được giữ
    for (uint8_t i = 0; i <= wakeCount % 5; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delayMs(100);
        digitalWrite(LED_PIN, LOW);
        delayMs(150);
    }

    powerSleep(); // Ngủ tới khi nhấn nút, thức dậy chạy tiếp dòng dưới
    wakeCount++;
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
