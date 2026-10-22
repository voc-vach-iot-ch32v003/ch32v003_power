/**
 * @file ch32v003_power.h
 * @author Vọc Vạch IoT
 * @brief Thư viện quản lý năng lượng cho CH32V003 (J4M6, A4M6, F4P6): Sleep / Standby, đánh thức bằng chân hoặc hẹn giờ.
 */

#ifndef CH32V003_POWER_H
#define CH32V003_POWER_H

#include "ch32fun.h"
#include "ch32v003_power_defs.h"
#include "ch32v003_power_types.h"

/**
 * @brief Khai báo nguồn đánh thức cho cả powerSleep() và powerStandby().
 * @details Lần gọi sau thay thế hoàn toàn cấu hình lần trước (không cộng dồn). Chân đánh thức được đặt
 *          về Input kèm điện trở kéo nội và nối vào đường sự kiện EXTI tương ứng. Thời gian hẹn giờ được
 *          tính sẵn tại đây (không tính lúc chip đang chạy clock thấp).
 * @param config Cấu hình mong muốn, hoặc NULL để xóa toàn bộ nguồn đánh thức.
 * @note Thư viện chỉ dùng chế độ sự kiện EXTI (EVENR), không chiếm hàm ngắt EXTI7_0_IRQHandler.
 */
void powerConfigureWakeup(const POWER_WakeupConfiguration_t* config);

/**
 * @brief Chế độ Sleep: hạ clock hệ thống (HSI / 256), tắt clock ngoại vi, CPU dừng tới khi có nguồn đánh thức
 *        (đo thực tế 381µA trên J4M6 ở 3.3V; Standby ~8–10µA).
 * @details Thức dậy chạy tiếp lệnh ngay sau hàm này, RAM và trạng thái ngoại vi giữ nguyên, clock được
 *          khôi phục như trước khi ngủ.
 * @return uint8_t 1 nếu đã ngủ, 0 nếu chưa khai báo nguồn đánh thức nào (từ chối ngủ vô thời hạn).
 * @note SysTick dừng trong lúc ngủ: millis() / micros() không tính phần thời gian ngủ.
 */
uint8_t powerSleep(void);

/**
 * @brief Chế độ Standby: tắt mọi clock trừ LSI (đo thực tế ~8–10µA trên J4M6 ở 3.3V), CPU dừng tới khi có nguồn đánh thức.
 * @details CH32V003 thức dậy từ Standby vẫn giữ RAM và chạy tiếp lệnh ngay sau hàm này (KHÔNG reset về
 *          đầu main()). Clock được khôi phục như trước khi ngủ.
 * @return uint8_t 1 nếu đã ngủ, 0 nếu chưa khai báo nguồn đánh thức nào (từ chối ngủ vô thời hạn).
 * @note SysTick dừng trong lúc ngủ: millis() / micros() không tính phần thời gian ngủ.
 * @warning Đặt các chân không dùng về ANALOG trước khi gọi: chân Input thả nổi làm dòng Standby tăng
 *          lên hàng chục µA.
 */
uint8_t powerStandby(void);

#endif // CH32V003_POWER_H
