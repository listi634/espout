/**
 * @file lvgl_display.h
 * @brief LVGL display driver interface for ST7789V2 LCD.
 */

#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include "esp_err.h"
#include "st7789.h"
#include "lvgl.h"

/**
 * @brief Initializes LVGL display driver using the existing ST7789 handle.
 *
 * @param[in] st7789_handle Initialized ST7789 display handle.
 * @return lv_display_t* Pointer to the LVGL display driver, or NULL on failure.
 */
lv_display_t *lvgl_display_init(st7789_handle_t st7789_handle);

/**
 * @brief Get the current LVGL display instance.
 *
 * @return lv_display_t* Pointer to the current LVGL display driver, or NULL.
 */
lv_display_t *lvgl_display_get_instance(void);

/**
 * @brief Deinitializes LVGL display driver.
 *
 * @param[in] disp LVGL display driver to deinitialize.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t lvgl_display_deinit(lv_display_t *disp);

#endif /* LVGL_DISPLAY_H */
