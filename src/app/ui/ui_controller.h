/**
 * @file ui_controller.h
 * @brief UI controller interface.
 */

#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UI controller.
 *
 * Initializes the screen manager and all screens.
 * 
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t ui_controller_init(void);

/**
 * @brief Deinitialize the UI controller.
 *
 * Cleans up all screen resources.
 */
void ui_controller_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_CONTROLLER_H */
