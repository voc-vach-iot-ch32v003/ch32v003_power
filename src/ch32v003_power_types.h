/**
 * @file ch32v003_power_types.h
 * @author Vọc Vạch IoT
 * @brief Định nghĩa enum và cấu trúc cấu hình nguồn đánh thức cho thư viện quản lý năng lượng CH32V003.
 */

#ifndef CH32V003_POWER_TYPES_H
#define CH32V003_POWER_TYPES_H

#include <stdint.h>

/**
 * @brief Sườn tín hiệu trên chân đánh thức chip.
 * @details Thư viện tự bật điện trở kéo nội ngược chiều sườn: FALLING bật Pull-up, RISING / CHANGE bật Pull-down.
 */
typedef enum
{
    POWER_WAKEUP_FALLING = 0, /**< Sườn xuống: chân chuyển từ HIGH sang LOW (VD: nút nhấn nối GND) */
    POWER_WAKEUP_RISING = 1, /**< Sườn lên: chân chuyển từ LOW sang HIGH */
    POWER_WAKEUP_CHANGE = 2 /**< Cả hai sườn */
} POWER_WakeupTrigger_t;

/**
 * @brief Một chân đánh thức, phần tử của mảng POWER_WakeupConfiguration_t.pins.
 * @note Mỗi số chân (0-7) chỉ dùng được cho MỘT Port (VD: PC3 và PD3 dùng chung đường EXTI3).
 */
typedef struct
{
    uint8_t mcuPin; /**< Chân vật lý (MCU_Pin_t / MCU_SpecialPin_t) */
    POWER_WakeupTrigger_t trigger; /**< Sườn tín hiệu đánh thức */
} POWER_WakeupPin_t;

/**
 * @brief Toàn bộ nguồn đánh thức dùng chung cho powerSleep() và powerStandby().
 * @details Chip thức dậy ngay khi MỘT trong các nguồn xảy ra trước: sườn trên một chân bất kỳ trong
 *          `pins`, hoặc hết `timeoutMs` (nếu > 0).
 */
typedef struct
{
    const POWER_WakeupPin_t* pins; /**< Mảng chân đánh thức, NULL nếu không dùng chân nào */
    uint8_t pinCount; /**< Số phần tử trong mảng pins */
    uint32_t timeoutMs; /**< Tự thức dậy sau khoảng thời gian này (ms, theo dao động LSI), 0 = tắt */
} POWER_WakeupConfiguration_t;

#endif // CH32V003_POWER_TYPES_H
