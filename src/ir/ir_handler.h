/**
 * @file ir_handler.h
 * @brief Generic hardware driver interface for NEC IR Remote receivers.
 */

#ifndef IR_HANDLER_H
#define IR_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "ir_config.h"

/**
 * @brief Function pointer callback type triggered when a valid key is pressed.
 * @param key The identified global key action enum value.
 * @param entry Pointer to the full config entry matching the code layout.
 */
typedef void (*ir_handler_callback_t)(ir_key_t key, const ir_lookup_entry_t *entry);

/**
 * @brief Initializes the RMT peripheral channel using a decoupled layout profile.
 * @param profile Pointer to the global lookup table array to use for matching.
 * @param profile_size Number of elements inside the chosen lookup table array.
 * @param callback User callback function executed upon successful frame decoding.
 * @return esp_err_t ESP_OK on success, or an appropriate driver error code.
 */
esp_err_t ir_handler_init(const ir_lookup_entry_t *profile, 
                          size_t profile_size, 
                          ir_handler_callback_t callback);

#endif /* IR_HANDLER_H */