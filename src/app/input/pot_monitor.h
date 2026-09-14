/**
 * @file pot_monitor.h
 * @brief Relative potentiometer movement monitor.
 */

#ifndef APP_POT_MONITOR_H
#define APP_POT_MONITOR_H

#include <stdint.h>
#include "esp_err.h"

typedef void (*pot_monitor_callback_t)(int8_t delta);

/**
 * @brief Start monitoring relative potentiometer movement.
 * @param callback Callback invoked with signed movement steps.
 * @return esp_err_t ESP_OK on success, or an appropriate error code.
 */
esp_err_t pot_monitor_init(pot_monitor_callback_t callback);

#endif /* APP_POT_MONITOR_H */