# 🔋 Ví dụ 01: Sleep giữ RAM, đánh thức bằng nút nhấn

## Mục tiêu

Đưa chip vào chế độ **Sleep** bằng `powerSleep()` và đánh thức bằng nút nhấn (sườn xuống). Chip thức dậy **chạy tiếp ngay sau `powerSleep()`**: biến đếm `wakeCount` trong RAM tăng qua mỗi lần thức, LED chớp số lần tương ứng.

## Sơ đồ nối dây (Pinout)

`MCU_PIN<n>` là số chân vật lý trên vỏ chip, nên chân port tương ứng khác nhau theo gói vỏ:

| Tín hiệu | Hằng số | J4M6 (SOP8) | A4M6 (SOP16) | F4P6 (TSSOP20) | Ghi chú |
| --- | --- | --- | --- | --- | --- |
| Nút nhấn | `WAKEUP_BUTTON_PIN` (`MCU_PIN5`) | PC1 | PC6 | PA1 | Một chân nút nối GND, Pull-up nội do thư viện bật |
| LED | `LED_PIN` (`MCU_PIN3`) | PA2 | PC3 | PD6 | Nối LED qua trở 470Ω xuống GND |

## Build & Nạp

```bash
pio run -e CH32V003J4M6 -t upload   # hoặc CH32V003A4M6 / CH32V003F4P6
```

## Kết quả mong đợi

- Khởi động: LED chớp 1 lần rồi chip ngủ.
- Mỗi lần nhấn nút: chip thức, LED chớp 2, 3, 4, 5 lần (quay vòng), rồi ngủ lại.
- Trong lúc ngủ chip chạy clock HSI/256 và tắt clock ngoại vi. Muốn dòng tiêu thụ thấp nhất (~10µA) thì xem ví dụ `02_standby_button_timer`.
