/**
 * @file action_handler.h
 * @brief Application action handler interface.
 */

#ifndef ACTION_HANDLER_H
#define ACTION_HANDLER_H

#include "esp_err.h"
#include "event_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the action handler.
 *
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t action_handler_init(void);

/**
 * @brief Process an event and trigger appropriate actions.
 *
 * @param event The event to process.
 */
void action_handler_process_event(const app_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* ACTION_HANDLER_H */
