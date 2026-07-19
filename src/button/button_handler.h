/**
 * @file button_handler.h
 * @brief Generalized reusable button component driver with debouncing and event processing.
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "esp_err.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle representing a button instance.
 */
typedef struct button_device *button_handle_t;

/**
 * @brief Supported button interaction events dispatched to the application.
 */
typedef enum {
    BUTTON_EVENT_SINGLE_PRESS, /**< Fired when button is pressed and released quickly */
    BUTTON_EVENT_LONG_PRESS,   /**< Fired once the button passes the hold-time threshold */
    BUTTON_EVENT_RELEASED      /**< Fired when the button is physically released */
} button_event_t;

/**
 * @brief Function pointer signature for receiving button events.
 * @param[in] event     The detected button event type.
 * @param[in] user_data Optional user context pointer passed during initialization.
 */
typedef void (*button_callback_t)(button_event_t event, void *user_data);

/**
 * @brief Configuration structure for initializing a generalized button instance.
 */
typedef struct {
    gpio_num_t gpio_num;         /**< Physical GPIO pin number */
    uint32_t long_press_ms;      /**< Duration threshold for a long press event (e.g., 1000ms) */
    button_callback_t callback;  /**< Application callback function to execute on events */
    void *user_data;             /**< Optional user context passed back into the callback */
    bool active_low;             /**< True if button pulls to GND (internal pull-up enabled) */
} button_config_t;

/**
 * @brief Initializes a button device instance and spawns its background processing task.
 * @param[out] out_handle Pointer to store the allocated button device handle.
 * @param[in]  config     Pointer to the configuration structure parameters.
 * @return esp_err_t      ESP_OK on success, or structural framework error codes.
 */
esp_err_t button_handler_init(button_handle_t *out_handle, const button_config_t *config);

/**
 * @brief Terminates processing, deletes the task, and frees allocated button resources.
 * @param[in] handle Button device handle to destroy.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t button_handler_deinit(button_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_HANDLER_H */