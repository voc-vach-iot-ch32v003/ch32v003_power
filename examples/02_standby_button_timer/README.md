# 🌙 Ví dụ 02: Standby, đánh thức bằng nút nhấn hoặc hẹn giờ

## Mục tiêu

Đưa chip vào chế độ **Standby** (~10µA theo datasheet) bằng `powerStandby()` với **hai nguồn đánh thức cùng lúc**: nút nhấn (sườn xuống) và tự thức sau `WAKEUP_PERIOD_MS` (10 giây). Chạy được trên cả 3 gói vỏ.

CH32V003 thức dậy từ Standby **vẫn giữ RAM và chạy tiếp** ngay sau `powerStandby()` (khác STM32, không reset về đầu `main()`): `wakeCount` in ra qua SWIO tăng dần.

## Sơ đồ nối dây (Pinout)

| Tín hiệu | Hằng số | J4M6 (SOP8) | A4M6 (SOP16) | F4P6 (TSSOP20) | Ghi chú |
| --- | --- | --- | --- | --- | --- |
| Nút nhấn | `WAKEUP_BUTTON_PIN` (`MCU_PIN5`) | PC1 | PC6 | PA1 | Một chân nút nối GND, Pull-up nội do thư viện bật |
| LED | `LED_PIN` (`MCU_PIN3`) | PA2 | PC3 | PD6 | Nối LED qua trở 470Ω xuống GND |

## Build & Nạp

```bash
pio run -e CH32V003J4M6 -t upload_and_sdi_monitor   # hoặc CH32V003A4M6 / CH32V003F4P6
```

## Kết quả mong đợi

```text
Standby demo: nhan nut hoac doi 10000 ms
Ngu lan 0...
Thuc day! wakeCount = 1
Ngu lan 1...
```

- Không nhấn nút: chip tự thức khoảng mỗi 10 giây (dài hơn một chút nếu LSI của chip thấp hơn 128kHz, hiệu chỉnh bằng `POWER_LSI_FREQUENCY_HZ` trong `include/sys_config.h`).
- Nhấn nút: chip thức ngay.
- Trong lúc ngủ, SWD tạm tắt (`POWER_SLEEP_DISABLE_SWD = 1`) nên terminal có thể hiện vài ký tự rác. Chip vẫn nạp lại được vì luôn có 1 giây chờ đầu `main()`.
