/**
 * @file input_manager.h
 * @brief Application input source manager.
 */

#ifndef APP_INPUT_MANAGER_H
#define APP_INPUT_MANAGER_H

#include <stddef.h>
#include "esp_err.h"
#include "ir_config.h"

/**
 * @brief Initialize all application input sources and active-screen routing.
 * @param ir_profile IR lookup profile used by the IR hardware handler.
 * @param ir_profile_size Number of entries in the IR lookup profile.
 * @return esp_err_t ESP_OK on success, or an appropriate error code.
 */
esp_err_t input_manager_init(
	const ir_lookup_entry_t *ir_profile,
	size_t ir_profile_size);

/**
 * @brief Forward a decoded IR key to the active screen.
 * @param key Decoded IR key.
 * @param entry Matching IR profile entry.
 */
void input_manager_process_ir(
	ir_key_t key,
	const ir_lookup_entry_t *entry);

/**
 * @brief Forward a relative potentiometer step to the active screen.
 * @param delta Signed movement step count.
 */
void input_manager_process_pot(int8_t delta);

#endif /* APP_INPUT_MANAGER_H */