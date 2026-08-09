/**
 * @file lvgl_ui.h
 * @brief LVGL UI initialization and management.
 */

#ifndef LVGL_UI_H
#define LVGL_UI_H

#include "esp_err.h"
#include "lvgl.h"
#include "ir_config.h"

/**
 * @brief Initializes LVGL and creates the main UI.
 *
 * @param[in] disp LVGL display driver instance.
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t lvgl_ui_init(lv_display_t *disp);

/**
 * @brief Updates the UI based on an IR key event.
 *
 * @param[in] key The IR key that was pressed.
 * @param[in] entry The IR lookup entry for the key.
 */
void lvgl_ui_handle_ir_event(ir_key_t key, const ir_lookup_entry_t *entry);

/**
 * @brief Updates the brightness display value.
 *
 * @param[in] value The new brightness value (0-100).
 */
void lvgl_ui_update_brightness(int value);

/**
 * @brief Updates the power status display.
 *
 * @param[in] is_on True if power is on, false if off.
 */
void lvgl_ui_update_power_status(bool is_on);

/**
 * @brief Tick handler for LVGL (should be called periodically with elapsed ms).
 * 
 * @param[in] tick_period_ms Time elapsed since last call in milliseconds.
 */
void lvgl_tick_handler(uint32_t tick_period_ms);

/**
 * @brief Deinitializes LVGL UI and frees resources.
 */
void lvgl_ui_deinit(void);

#endif /* LVGL_UI_H */

