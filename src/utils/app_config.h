/**
 * @file app_config.h
 * @brief Global application configuration and bootstrapping hooks interface.
 *
 * Kcapsulates initialization procedures for runtime logging whitelists, 
 * peripheral pin validations, and central hardware configuration hooks.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes system-wide runtime configurations and log whitelists.
 * * Loops through the developer playground table of component tags to apply
 * targeted runtime log verbosity dynamically without hardcoding settings
 * inside main blocks.
 *
 * @return esp_err_t ESP_OK on success, or appropriate structural error code.
 */
esp_err_t app_config_init(void);

#ifdef __cplusplus
}
#endif

#endif // APP_CONFIG_H