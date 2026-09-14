/**
 * @file screen.h
 * @brief Screen interface definition for modular UI architecture.
 * 
 * This header defines the interface that all screens must implement
 * to be managed by the screen manager.
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "esp_err.h"
#include "lvgl.h"
#include "app/event_bus/event_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Screen identifier type.
 */
typedef enum {
    SCREEN_NONE = -1,
    SCREEN_MAIN,
    SCREEN_LOADING,
    // Future screens will be added here
    SCREEN_COUNT
} screen_id_t;

/**
 * @brief Screen structure containing all screen-related data and callbacks.
 * 
 * All screens must implement this interface to be managed by the screen manager.
 */
typedef struct screen_t {
    /** @brief Screen identifier. */
    screen_id_t id;
    
    /** @brief Screen name for debugging. */
    const char *name;
    
    /** @brief The LVGL screen object (lv_obj_t*). */
    lv_obj_t *view;
    
    /**
     * @brief Callback to create the screen UI.
     * 
     * Called once when the screen is first registered.
     * Should create all LVGL widgets and store references.
     * 
     * @param parent The parent LVGL object to create the screen under.
     * @return esp_err_t ESP_OK on success, error code on failure.
     */
    esp_err_t (*on_create)(lv_obj_t *parent);
    
    /**
     * @brief Callback when screen is activated (shown).
     * 
     * Called every time the screen becomes visible.
     * Should refresh UI state from current application state.
     * 
     * @return esp_err_t ESP_OK on success, error code on failure.
     */
    esp_err_t (*on_activate)(void);
    
    /**
     * @brief Callback when screen is deactivated (hidden).
     * 
     * Called every time the screen is hidden.
     * Should clean up any temporary resources.
     * 
     * @return esp_err_t ESP_OK on success, error code on failure.
     */
    esp_err_t (*on_deactivate)(void);
    
    /**
     * @brief Callback to destroy the screen and free resources.
     * 
     * Called when the screen is unregistered or application shuts down.
     * Should free all LVGL objects and any allocated memory.
     * 
     * @return esp_err_t ESP_OK on success, error code on failure.
     */
    esp_err_t (*on_destroy)(void);
    
    /**
     * @brief Optional callback to handle events directed to this screen.
     * 
     * If NULL, events are not processed by this screen.
     * 
     * @param event The event to handle.
     */
    void (*on_event)(const app_event_t *event);
    
    /**
     * @brief Optional callback for periodic updates (e.g., animations).
     * 
     * Called from the UI timer task if screen is active.
     * If NULL, no periodic updates are performed.
     */
    void (*on_update)(void);
    
    /**
     * @brief User data pointer for screen-specific context.
     * 
     * Can be used to store screen-specific data.
     */
    void *user_data;
} screen_t;

/**
 * @brief Initialize a screen structure with default values.
 * 
 * @param screen Pointer to the screen structure to initialize.
 * @param id Screen identifier.
 * @param name Screen name for debugging.
 */
void screen_init(screen_t *screen, screen_id_t id, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_H */
