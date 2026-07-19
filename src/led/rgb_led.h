/**
 * @file rgb_led.h
 * @brief Driver interface for the on-board WS2812B RGB LED using ESP-IDF RMT.
 */

#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initializes the on-board RGB LED peripheral.
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t rgb_led_init(void);

/**
 * @brief Sets the color of the on-board RGB LED.
 * @param red Red component intensity (0-255).
 * @param green Green component intensity (0-255).
 * @param blue Blue component intensity (0-255).
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Clears (turns off) the on-board RGB LED.
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t rgb_led_clear(void);

#endif /* RGB_LED_H */