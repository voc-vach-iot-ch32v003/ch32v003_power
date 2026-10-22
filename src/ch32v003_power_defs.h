/**
 * @file ch32v003_power_defs.h
 * @author Vọc Vạch IoT
 * @brief Macro cấu hình mặc định của thư viện quản lý năng lượng (ghi đè được qua sys_config.h).
 */

#ifndef CH32V003_POWER_DEFS_H
#define CH32V003_POWER_DEFS_H

// Cho phép người dùng ghi đè cấu hình qua include/sys_config.h của dự án
#if defined(__has_include)
#if __has_include("sys_config.h")
#include "sys_config.h"
#endif
#endif

#ifndef POWER_LSI_FREQUENCY_HZ
#define POWER_LSI_FREQUENCY_HZ 128000UL /**< Tần số dao động LSI dùng để tính thời gian tự thức dậy (danh định 128kHz, thực tế lệch theo từng chip) */
#endif

#ifndef POWER_SLEEP_DISABLE_SWD
#define POWER_SLEEP_DISABLE_SWD 1 /**< 1 = Tắt SWD trong lúc ngủ để không rò dòng qua mạch nạp (khôi phục sau khi thức), 0 = giữ nguyên */
#endif

#endif // CH32V003_POWER_DEFS_H
