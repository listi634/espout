/**
 * @file main_screen.h
 * @brief Main screen interface for the application.
 * 
 * This screen displays the selectable application functions.
 */

#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

#include "esp_err.h"
#include "screen.h"
#include "../../state/app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the main screen.
 * 
 * Registers the main screen with the screen manager.
 * 
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t main_screen_init(void);

/**
 * @brief Update the selected function on the main screen.
 *
 * Thread-safe update via queue.
 *
 * @param function Function to display in the middle position.
 */
void main_screen_update_selection(app_function_t function);

/**
 * @brief Get the main screen's LVGL screen object.
 * 
 * @return lv_obj_t* Pointer to the LVGL screen object, or NULL if not created.
 */
lv_obj_t *main_screen_get_view(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_SCREEN_H */
