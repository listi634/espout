/**
 * @file event_bus.h
 * @brief Application event bus interface.
 */

#ifndef APP_EVENT_BUS_H
#define APP_EVENT_BUS_H

#include "event_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the application event bus.
 *
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t event_bus_init(void);

/**
 * @brief Deinitialize the application event bus.
 */
void event_bus_deinit(void);

/**
 * @brief Publish an event to the bus.
 *
 * @param event Pointer to the event to publish.
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t event_bus_publish(const app_event_t *event);

/**
 * @brief Subscribe to specific event types.
 *
 * @param event_types Array of event types to subscribe to.
 * @param num_types Number of event types in the array.
 * @param callback Callback function invoked when matching events occur.
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t event_bus_subscribe(
    const app_event_type_t *event_types,
    size_t num_types,
    void (*callback)(const app_event_t *event));

/**
 * @brief Unsubscribe a callback from all event types.
 *
 * @param callback The callback function to remove.
 */
void event_bus_unsubscribe(void (*callback)(const app_event_t *event));

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENT_BUS_H */
