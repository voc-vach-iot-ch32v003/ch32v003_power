# ⚡ ch32v003_power

Thư viện quản lý năng lượng thông minh và cấu hình ngắt đánh thức (Wakeup) cho các dòng vi điều khiển **CH32V003** (J4M6, A4M6, F4P6) trên nền framework **ch32v003fun**.
Thư viện hỗ trợ đưa vi điều khiển vào chế độ ngủ tiết kiệm điện cực hạ dòng (Sleep / Standby), tự động hạ tần số hệ thống xuống $31.25\text{ kHz}$, quản lý clock AFIO/GPIO thông minh và tự động khôi phục xung nhịp cao (PLL 48MHz) sau khi thức giấc.

---

## 🚀 Tính năng nổi bật

- **Ngủ nhẹ giữ bộ nhớ (Ultra Low Power Sleep):** Hạ xung nhịp CPU, ngắt xung clock ngoại vi không sử dụng và đi vào chế độ `wfi`. Khi có ngắt, chip tỉnh giấc và tiếp tục thực thi ngay dòng lệnh phía dưới mà không bị mất RAM.
- **Ngủ sâu cực hạn (Standby Mode):** Ép dòng tiêu thụ xuống mức $\approx 10\mu\text{A}$, ngắt toàn bộ lõi CPU và RAM. Khi kích hoạt chân Wakeup phần cứng (PD0), chip sẽ tự động **Hardware Reset** và khởi chạy lại từ hàm `main()`. _(Dành cho A4M6 và F4P6)_.
- **Cấu hình Wakeup linh hoạt:** Hỗ trợ kích hoạt đánh thức qua bất kỳ chân GPIO nào có ngắt EXTI (Line 0–7) với kiểu kích hoạt cạnh lên (`RISING`), cạnh xuống (`FALLING`), hoặc cả hai (`CHANGE`).
- **Bảo vệ mạch nạp SWD:** Tự động ngắt chân Debug SWIO (PD1) trước khi ngủ để tránh rò dòng qua WCH-LinkE và khôi phục ngay sau khi thức dậy.

---

## 📑 Cấu trúc hàm (API Reference)

### 1. Hàm cấu hình chân đánh thức (Wakeup Pin)

| Hàm xử lý                                            | Tham số                                                                     | Giá trị trả về | Mô tả                                                                                                           |
| ---------------------------------------------------- | --------------------------------------------------------------------------- | -------------- | --------------------------------------------------------------------------------------------------------------- |
| **`enableWakeupPin(mcuPin, trigger)`**               | `uint8_t mcuPin`, `WakeupTrigger_t trigger`                                 | `void`         | Cấu hình chân vật lý trên board làm chân Wakeup. `trigger`: `WAKEUP_FALLING`, `WAKEUP_RISING`, `WAKEUP_CHANGE`. |
| **`enableWakeupPinPort(GPIOx, pinNumber, trigger)`** | `const GPIO_TypeDef* GPIOx`, `uint8_t pinNumber`, `WakeupTrigger_t trigger` | `void`         | Cấu hình trực tiếp qua cổng `GPIOA/C/D` và chỉ số pin (0–7) tương ứng với EXTI Line.                            |

### 2. Hàm chuyển chế độ tiết kiệm điện

| Hàm xử lý                  | Tham số | Giá trị trả về | Mô tả                                                                                                     |
| -------------------------- | ------- | -------------- | --------------------------------------------------------------------------------------------------------- |
| **`sleepUltraLowPower()`** | `void`  | `void`         | Hạ xung hệ thống, đưa CPU vào Sleep Mode (`wfi`). Thức dậy tiếp tục chạy tiếp code bên dưới.              |
| **`enterStandbyMode()`**   | `void`  | `void`         | Đưa chip vào Standby Mode. Thức dậy qua chân PD0 sẽ Reset chip từ đầu main. _(Không khả dụng trên J4M6)_. |

---

## ⚙️ Các chế độ ngắt Wakeup (`WakeupTrigger_t`)

```c
typedef enum {
    WAKEUP_FALLING = 0, // Kích hoạt khi chân chuyển từ HIGH xuống LOW (Dùng PULLUP nội)
    WAKEUP_RISING  = 1, // Kích hoạt khi chân chuyển từ LOW lên HIGH (Dùng PULLDOWN nội)
    WAKEUP_CHANGE  = 2  // Kích hoạt khi có bất kỳ sự thay đổi trạng thái nào
} WakeupTrigger_t;

```

---

## 📝 Code mẫu sử dụng

### 1. Sử dụng Chế độ Ngủ nhẹ (Sleep Ultra Low Power)

```c
#include "ch32fun.h"
#include <ch32v003_gpio.h>
#include <ch32v003_delay.h>
#include <ch32v003_power.h>

#define BUTTON_PIN MCU_PIN3 // Chân nút nhấn đánh thức
#define LED_PIN    MCU_PIN4 // Chân LED hiển thị

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // Cấu hình BUTTON_PIN làm chân Wakeup khi nhấn (Cạnh xuống FALLING)
    enableWakeupPin(BUTTON_PIN, WAKEUP_FALLING);
}

void loop() {
    // Chớp LED báo hiệu trước khi đi ngủ
    digitalWrite(LED_PIN, HIGH);
    delayMs(200);
    digitalWrite(LED_PIN, LOW);

    // Đưa CPU vào chế độ ngủ tiết kiệm điện
    sleepUltraLowPower();

    // --- CPU ĐÃ THỨC DẬY ---
    // Code chạy tiếp ngay tại đây sau khi có nút nhấn trên BUTTON_PIN
    for (uint8_t i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delayMs(50);
        digitalWrite(LED_PIN, LOW);
        delayMs(50);
    }
}

int main() {
    SystemInit();
    setup();

    while (1) {
        loop();
    }
}

```

### 2. Sử dụng Chế độ Ngủ sâu cực hạn (Standby Mode)

```c
#include "ch32fun.h"
#include <ch32v003_gpio.h>
#include <ch32v003_delay.h>
#include <ch32v003_power.h>

#define LED_PIN MCU_PIN4

int main() {
    SystemInit();

    pinMode(LED_PIN, OUTPUT);

    // Báo hiệu chip mới khởi động lại / vừa thức dậy từ Standby
    digitalWrite(LED_PIN, HIGH);
    delayMs(1000);
    digitalWrite(LED_PIN, LOW);

    // Đưa chip vào Standby (Cần tác động mức HIGH vào chân PD0 để Reset thức giấc)
#ifndef CH32V003_J4M6
    enterStandbyMode();
#endif

    while (1) {
        // Không bao giờ chạy tới đây vì Standby sẽ Reset chip về main()
    }
}

```

---
