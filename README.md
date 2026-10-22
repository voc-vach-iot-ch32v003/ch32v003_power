# 🔋 ch32v003_power

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Supported-orange.svg)
![Framework](https://img.shields.io/badge/Framework-ch32v003fun-green.svg)
![MCU](https://img.shields.io/badge/MCU-CH32V003-red.svg)

Thư viện quản lý năng lượng cho vi điều khiển CH32V003: đưa chip vào **Sleep** hoặc **Standby**, đánh thức bằng **sườn tín hiệu trên chân bất kỳ** và/hoặc **hẹn giờ**. Chạy được trên cả 3 gói vỏ J4M6, A4M6, F4P6.

---

## 🚀 Tính năng nổi bật

- **Hai mức ngủ, cùng cách dùng:** `powerSleep()` (hạ clock HSI/256, tắt clock ngoại vi; đo thực tế **371µA**) và `powerStandby()` (tắt mọi clock trừ LSI; đo thực tế **6.7µA**). Số đo trên J4M6, 3.3V, chân không dùng đặt `ANALOG`.
- **Thức dậy chạy tiếp, giữ RAM:** CH32V003 thức từ cả Sleep lẫn Standby đều chạy tiếp lệnh ngay sau hàm ngủ, không reset về đầu `main()`. Clock hệ thống, clock ngoại vi, SysTick và chế độ SWIO được khôi phục như trước khi ngủ.
- **Nhiều nguồn đánh thức cùng lúc:** nhiều chân (sườn xuống / lên / cả hai) cộng thêm hẹn giờ. Nguồn nào xảy ra trước thì chip thức trước.
- **Hẹn giờ từ vài ms tới hàng giờ:** tự chia thành nhiều lần AWU (mỗi lần tối đa ~30 giây). Cấu hình được tính sẵn trong `powerConfigureWakeup()`, không tính lúc clock thấp.
- **Không chiếm ngắt:** dùng sự kiện EXTI (WFE), `EXTI7_0_IRQHandler` vẫn để dành cho chương trình.

---

## 📦 Cài đặt (Installation)

```ini
lib_deps =
    https://github.com/voc-vach-iot-ch32v003/ch32v003_power.git
```

Thư viện tự kéo theo `ch32v003_gpio`.

---

## ⚙️ Cấu hình môi trường

```ini
build_flags = -DCH32V003_J4M6 ; hoặc -DCH32V003_A4M6 / -DCH32V003_F4P6
```

Các macro sau có giá trị mặc định, ghi đè được trong `include/sys_config.h` của dự án:

| Macro | Mặc định | Ý nghĩa |
| :-- | :-- | :-- |
| `POWER_LSI_FREQUENCY_HZ` | `128000` | Tần số LSI để tính thời gian hẹn giờ. LSI lệch theo từng chip: bàn test đo được 109kHz, 112kHz, 123kHz (chip ngủ dài hơn yêu cầu 4–17%). Hiệu chỉnh: ngủ 10000ms, bấm giờ thực tế `T` ms, đặt `128000 * 10000 / T`. |
| `POWER_SLEEP_DISABLE_SWD` | `1` | `1` = tắt SWD trong lúc ngủ để không rò dòng qua mạch nạp (tự khôi phục khi thức). Terminal SWIO có thể hiện vài ký tự rác quanh lúc ngủ; nếu cần log qua `printf` ngay sau khi thức, chờ ~50ms để mạch nạp đồng bộ lại. |

> ⚠️ **Chân đánh thức:** mỗi số chân 0–7 chỉ dùng được cho một Port (VD: PC3 và PD3 dùng chung đường EXTI3). Thư viện tự bật điện trở kéo nội ngược chiều sườn: `FALLING` bật Pull-up, `RISING` / `CHANGE` bật Pull-down.
>
> ⚠️ **SysTick dừng trong lúc ngủ:** `millis()` / `micros()` không tính phần thời gian ngủ.
>
> ⚠️ **Chân không dùng phải đặt `ANALOG` trước khi ngủ**, đừng để Input thả nổi: chân trôi ở mức lưng chừng làm bộ đệm ngõ vào CMOS dẫn xuyên. Đo thực tế trên J4M6 ở Standby: chân thả nổi **≥ 50µA**, chân `ANALOG` **6.7µA**. Thư viện không tự đổi chân (để không làm mất trạng thái output của chương trình).
>
> ```c
> for (uint8_t pin = 0; pin < 8; pin++)
> {
>     pinModePort(GPIOA, pin, ANALOG);
>     pinModePort(GPIOC, pin, ANALOG);
>     pinModePort(GPIOD, pin, ANALOG); // PD1 ở ANALOG vẫn nạp được khi SWD bật (đã thử trên J4M6)
> } // Sau đó cấu hình lại các chân đang dùng
> ```
>
> ⚠️ **Chip đang ngủ thì không nạp được** (SWD tắt trong lúc ngủ khi `POWER_SLEEP_DISABLE_SWD = 1`). Muốn nạp lại: tắt rồi cấp lại nguồn để chip vào khoảng chờ `delayMs(1000)` đầu `main()`. Nếu không chủ động tắt/bật nguồn được (VD: nguồn cố định), dùng `rvprog -e` hoặc WCH-LinkUtility: rút nguồn chip, bấm xóa, cấp nguồn ngay.

---

## 📝 Code mẫu sử dụng (Quick Start)

Chớp LED rồi ngủ Standby, thức dậy sau 2 giây hoặc khi nhấn nút (nối GND):

```c
#include "ch32fun.h"
#include "ch32v003_delay.h"
#include "ch32v003_gpio.h"
#include "ch32v003_power.h"

#define LED_PIN    MCU_PIN3 // Chân vật lý số 3 (J4M6: PA2, A4M6: PC3, F4P6: PD6)
#define BUTTON_PIN MCU_PIN5 // Chân vật lý số 5 (J4M6: PC1, A4M6: PC6, F4P6: PA1)

static const POWER_WakeupPin_t wakeupPins[] = {
    {BUTTON_PIN, POWER_WAKEUP_FALLING},
};

static const POWER_WakeupConfiguration_t wakeupConfig = {
    .pins = wakeupPins,
    .pinCount = 1,
    .timeoutMs = 2000,
};

static void setup(void)
{
    pinMode(LED_PIN, OUTPUT);
    powerConfigureWakeup(&wakeupConfig);
}

static void loop(void)
{
    digitalWrite(LED_PIN, HIGH);
    delayMs(50);
    digitalWrite(LED_PIN, LOW);

    powerStandby(); // Thức dậy chạy tiếp ngay dòng dưới, RAM giữ nguyên
}

int main(void)
{
    SystemInit();
    delayMs(1000); // Khoảng chờ an toàn tránh brick chip (chip ngủ vẫn nạp lại được nhờ khoảng này)

    setup();
    while (1)
    {
        loop();
    }
}
```

---

## 📁 Danh sách Ví dụ mẫu (Examples)

| Ví dụ | Mô tả |
| :-- | :-- |
| [`01_sleep_button_wakeup`](examples/01_sleep_button_wakeup/) | `powerSleep()` đánh thức bằng nút nhấn; LED chớp theo số lần thức để thấy RAM được giữ. |
| [`02_standby_button_timer`](examples/02_standby_button_timer/) | `powerStandby()` đánh thức bằng nút nhấn **hoặc** sau 10 giây; in số lần thức qua SWIO, hướng dẫn hiệu chỉnh LSI. |

---

## 📑 Tra cứu API (API Reference)

### Hàm

| Hàm xử lý | Tham số | Giá trị trả về | Mô tả |
| :-- | :-- | :-- | :-- |
| `powerConfigureWakeup(config)` | `const POWER_WakeupConfiguration_t* config` | `void` | Khai báo nguồn đánh thức cho cả 2 chế độ ngủ. Lần gọi sau thay thế hoàn toàn lần trước. `NULL` = xóa hết. |
| `powerSleep()` | Không | `uint8_t` | Ngủ Sleep tới khi có nguồn đánh thức. Trả `0` (không ngủ) nếu chưa khai báo nguồn nào. |
| `powerStandby()` | Không | `uint8_t` | Ngủ Standby tới khi có nguồn đánh thức. Trả `0` (không ngủ) nếu chưa khai báo nguồn nào. |

### Kiểu dữ liệu

| Kiểu | Thành phần | Mô tả |
| :-- | :-- | :-- |
| `POWER_WakeupTrigger_t` | `POWER_WAKEUP_FALLING`, `POWER_WAKEUP_RISING`, `POWER_WAKEUP_CHANGE` | Sườn tín hiệu đánh thức. |
| `POWER_WakeupPin_t` | `uint8_t mcuPin`, `POWER_WakeupTrigger_t trigger` | Một chân đánh thức (`MCU_PIN<n>` hoặc chân chức năng). |
| `POWER_WakeupConfiguration_t` | `const POWER_WakeupPin_t* pins`, `uint8_t pinCount`, `uint32_t timeoutMs` | Toàn bộ nguồn đánh thức. `timeoutMs = 0` = không hẹn giờ. |

---

## 📄 Giấy phép (License) & Tác giả

- **Tác giả:** [Vọc Vạch IoT](https://github.com/voc-vach-iot)
- **Giấy phép:** Phát hành theo giấy phép [MIT License](LICENSE).
