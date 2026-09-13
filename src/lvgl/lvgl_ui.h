/**
 * @file lvgl_ui.h
 * @brief LVGL UI rendering interface.
 */

#ifndef LVGL_UI_H
#define LVGL_UI_H

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL UI widgets.
 *
 * @param disp LVGL display driver instance.
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t lvgl_ui_init(lv_display_t *disp);

/**
 * @brief Update the brightness display.
 *
 * @param value Brightness percentage (0-100).
 */
void lvgl_ui_update_brightness(int value);

/**
 * @brief Update the power status display.
 *
 * @param is_on true if power is on, false if off.
 */
void lvgl_ui_update_power_status(bool is_on);

/**
 * @brief Deinitialize LVGL UI and free resources.
 */
void lvgl_ui_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_UI_H */
