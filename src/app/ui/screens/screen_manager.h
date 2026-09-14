/**
 * @file screen_manager.h
 * @brief Screen manager interface for managing multiple UI screens.
 * 
 * The screen manager handles registration, activation, deactivation,
 * and lifecycle management of all application screens.
 */

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "esp_err.h"
#include "screen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of screens that can be registered.
 */
#define SCREEN_MANAGER_MAX_SCREENS 16

/**
 * @brief Initialize the screen manager.
 * 
 * Must be called before any screens are registered.
 * 
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t screen_manager_init(void);

/**
 * @brief Deinitialize the screen manager and all registered screens.
 * 
 * Cleans up all screen resources.
 */
void screen_manager_deinit(void);

/**
 * @brief Register a screen with the screen manager.
 * 
 * Screens must be registered before they can be activated.
 * The screen's on_create callback will be called immediately.
 * 
 * @param screen Pointer to the screen structure to register.
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t screen_manager_register(screen_t *screen);

/**
 * @brief Unregister a screen from the screen manager.
 * 
 * The screen's on_destroy callback will be called.
 * 
 * @param id The screen identifier to unregister.
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t screen_manager_unregister(screen_id_t id);

/**
 * @brief Activate a screen (make it visible).
 * 
 * The screen's on_activate callback will be called.
 * The previously active screen's on_deactivate callback will be called.
 * 
 * @param id The screen identifier to activate.
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t screen_manager_activate(screen_id_t id);

/**
 * @brief Deactivate the currently active screen.
 * 
 * The screen's on_deactivate callback will be called.
 * 
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t screen_manager_deactivate_current(void);

/**
 * @brief Get the currently active screen.
 * 
 * @return screen_id_t The identifier of the currently active screen, or SCREEN_NONE.
 */
screen_id_t screen_manager_get_current(void);

/**
 * @brief Get a registered screen by identifier.
 * 
 * @param id The screen identifier.
 * @return screen_t* Pointer to the screen structure, or NULL if not found.
 */
screen_t *screen_manager_get_screen(screen_id_t id);

/**
 * @brief Check if a screen is registered.
 * 
 * @param id The screen identifier.
 * @return bool true if registered, false otherwise.
 */
bool screen_manager_is_registered(screen_id_t id);

/**
 * @brief Set user data for a screen.
 * 
 * @param id The screen identifier.
 * @param data Pointer to user data.
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
esp_err_t screen_manager_set_user_data(screen_id_t id, void *data);

/**
 * @brief Get user data from a screen.
 * 
 * @param id The screen identifier.
 * @return void* Pointer to user data, or NULL if not found.
 */
void *screen_manager_get_user_data(screen_id_t id);

/**
 * @brief Process periodic updates for the current screen.
 * 
 * Calls the on_update callback of the currently active screen if it exists.
 * Should be called from the UI timer task.
 */
void screen_manager_process_updates(void);

/**
 * @brief Distribute an event to all registered screens.
 * 
 * Calls the on_event callback of each screen that has one.
 * 
 * @param event Pointer to the event to distribute.
 */
void screen_manager_distribute_event(const app_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_MANAGER_H */
