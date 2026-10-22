/**
 * @file user_config.h
 * @author Vọc Vạch IoT
 * @brief Chân nút nhấn đánh thức và LED báo hiệu của ví dụ Sleep.
 */

#ifndef USER_CONFIG_H
#define USER_CONFIG_H

/** @brief Nút nhấn nối GND, đánh thức bằng sườn xuống (chân vật lý số 5: J4M6 PC1, A4M6 PC6, F4P6 PA1). */
#define WAKEUP_BUTTON_PIN MCU_PIN5

/** @brief LED qua trở 470Ω xuống GND (chân vật lý số 3: J4M6 PA2, A4M6 PC3, F4P6 PD6). */
#define LED_PIN MCU_PIN3

#endif // USER_CONFIG_H
