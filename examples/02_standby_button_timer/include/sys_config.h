/**
 * @file sys_config.h
 * @author Vọc Vạch IoT
 * @brief Cấu hình thư viện cho ví dụ Standby.
 */

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

/**
 * @brief Tần số LSI thực tế của chip (Hz) để thời gian tự thức dậy chính xác hơn.
 * @details Danh định 128kHz, nhưng mỗi chip lệch khác nhau (bàn test đo được 109–123kHz).
 *          Đo bằng cách ngủ 10000ms rồi bấm giờ thời gian thực T (ms): LSI ≈ 128000 * 10000 / T.
 */
#define POWER_LSI_FREQUENCY_HZ 128000UL

#endif // SYS_CONFIG_H
