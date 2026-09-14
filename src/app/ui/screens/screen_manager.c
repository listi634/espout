/**
 * @file screen_manager.c
 * @brief Screen manager implementation for managing multiple UI screens.
 */

#include "screen_manager.h"
#include "screen.h"
#include "esp_log.h"

static const char *TAG = "SCREEN_MANAGER";

/**
 * @brief Screen manager state structure.
 */
typedef struct {
    screen_t *screens[SCREEN_MANAGER_MAX_SCREENS];
    screen_id_t current_screen;
    bool initialized;
} screen_manager_state_t;

static screen_manager_state_t s_state = {0};

esp_err_t screen_manager_init(void)
{
    if (s_state.initialized) {
        ESP_LOGW(TAG, "Screen manager already initialized");
        return ESP_OK;
    }

    for (int i = 0; i < SCREEN_MANAGER_MAX_SCREENS; i++) {
        s_state.screens[i] = NULL;
    }
    s_state.current_screen = SCREEN_NONE;
    s_state.initialized = true;

    ESP_LOGI(TAG, "Screen manager initialized");
    return ESP_OK;
}

void screen_manager_deinit(void)
{
    if (!s_state.initialized) {
        return;
    }

    // Deactivate and destroy all registered screens
    for (int i = 0; i < SCREEN_MANAGER_MAX_SCREENS; i++) {
        if (s_state.screens[i] != NULL) {
            screen_t *screen = s_state.screens[i];
            
            // Deactivate if this is the current screen
            if (screen->id == s_state.current_screen) {
                if (screen->on_deactivate != NULL) {
                    screen->on_deactivate();
                }
            }
            
            // Destroy the screen
            if (screen->on_destroy != NULL) {
                screen->on_destroy();
            }
            
            s_state.screens[i] = NULL;
        }
    }

    s_state.current_screen = SCREEN_NONE;
    s_state.initialized = false;

    ESP_LOGI(TAG, "Screen manager deinitialized");
}

esp_err_t screen_manager_register(screen_t *screen)
{
    if (!s_state.initialized) {
        ESP_LOGE(TAG, "Screen manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (screen == NULL) {
        ESP_LOGE(TAG, "Screen pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (screen->id < 0 || screen->id >= SCREEN_COUNT) {
        ESP_LOGE(TAG, "Invalid screen ID: %d", screen->id);
        return ESP_ERR_INVALID_ARG;
    }

    if (s_state.screens[screen->id] != NULL) {
        ESP_LOGE(TAG, "Screen ID %d already registered", screen->id);
        return ESP_ERR_INVALID_STATE;
    }

    // Store the screen
    s_state.screens[screen->id] = screen;

    // Initialize the screen view if not already created
    if (screen->view == NULL && screen->on_create != NULL) {
        esp_err_t ret = screen->on_create(NULL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to create screen %s: %s", 
                    screen->name, esp_err_to_name(ret));
            s_state.screens[screen->id] = NULL;
            return ret;
        }
    }

    // If this is the first screen, activate it
    if (s_state.current_screen == SCREEN_NONE) {
        s_state.current_screen = screen->id;
        if (screen->on_activate != NULL) {
            screen->on_activate();
        }
    }

    ESP_LOGI(TAG, "Screen registered: %s (ID: %d)", screen->name, screen->id);
    return ESP_OK;
}

esp_err_t screen_manager_unregister(screen_id_t id)
{
    if (!s_state.initialized) {
        ESP_LOGE(TAG, "Screen manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (id < 0 || id >= SCREEN_COUNT) {
        ESP_LOGE(TAG, "Invalid screen ID: %d", id);
        return ESP_ERR_INVALID_ARG;
    }

    screen_t *screen = s_state.screens[id];
    if (screen == NULL) {
        ESP_LOGW(TAG, "Screen ID %d not registered", id);
        return ESP_ERR_NOT_FOUND;
    }

    // Deactivate if this is the current screen
    if (id == s_state.current_screen) {
        if (screen->on_deactivate != NULL) {
            screen->on_deactivate();
        }
        s_state.current_screen = SCREEN_NONE;
    }

    // Destroy the screen
    if (screen->on_destroy != NULL) {
        screen->on_destroy();
    }

    s_state.screens[id] = NULL;

    ESP_LOGI(TAG, "Screen unregistered: %s (ID: %d)", screen->name, id);
    return ESP_OK;
}

esp_err_t screen_manager_activate(screen_id_t id)
{
    if (!s_state.initialized) {
        ESP_LOGE(TAG, "Screen manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (id < 0 || id >= SCREEN_COUNT) {
        ESP_LOGE(TAG, "Invalid screen ID: %d", id);
        return ESP_ERR_INVALID_ARG;
    }

    screen_t *new_screen = s_state.screens[id];
    if (new_screen == NULL) {
        ESP_LOGE(TAG, "Screen ID %d not registered", id);
        return ESP_ERR_NOT_FOUND;
    }

    // Deactivate current screen if different
    if (s_state.current_screen != SCREEN_NONE && 
        s_state.current_screen != id) {
        screen_t *current = s_state.screens[s_state.current_screen];
        if (current != NULL && current->on_deactivate != NULL) {
            current->on_deactivate();
        }
    }

    // Activate new screen
    s_state.current_screen = id;
    if (new_screen->on_activate != NULL) {
        new_screen->on_activate();
    }

    ESP_LOGI(TAG, "Screen activated: %s (ID: %d)", new_screen->name, id);
    return ESP_OK;
}

esp_err_t screen_manager_deactivate_current(void)
{
    if (!s_state.initialized) {
        ESP_LOGE(TAG, "Screen manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_state.current_screen == SCREEN_NONE) {
        ESP_LOGW(TAG, "No screen is currently active");
        return ESP_ERR_INVALID_STATE;
    }

    screen_t *screen = s_state.screens[s_state.current_screen];
    if (screen == NULL || screen->on_deactivate == NULL) {
        return ESP_OK;
    }

    screen->on_deactivate();
    ESP_LOGI(TAG, "Screen deactivated: %s (ID: %d)", 
            screen->name, s_state.current_screen);
    return ESP_OK;
}

screen_id_t screen_manager_get_current(void)
{
    if (!s_state.initialized) {
        return SCREEN_NONE;
    }
    return s_state.current_screen;
}

screen_t *screen_manager_get_screen(screen_id_t id)
{
    if (!s_state.initialized || id < 0 || id >= SCREEN_COUNT) {
        return NULL;
    }
    return s_state.screens[id];
}

bool screen_manager_is_registered(screen_id_t id)
{
    if (!s_state.initialized || id < 0 || id >= SCREEN_COUNT) {
        return false;
    }
    return s_state.screens[id] != NULL;
}

esp_err_t screen_manager_set_user_data(screen_id_t id, void *data)
{
    screen_t *screen = screen_manager_get_screen(id);
    if (screen == NULL) {
        ESP_LOGE(TAG, "Screen ID %d not registered", id);
        return ESP_ERR_NOT_FOUND;
    }
    screen->user_data = data;
    return ESP_OK;
}

void *screen_manager_get_user_data(screen_id_t id)
{
    screen_t *screen = screen_manager_get_screen(id);
    if (screen == NULL) {
        return NULL;
    }
    return screen->user_data;
}

void screen_manager_process_updates(void)
{
    if (!s_state.initialized) {
        return;
    }

    screen_t *screen = screen_manager_get_screen(s_state.current_screen);
    if (screen != NULL && screen->on_update != NULL) {
        screen->on_update();
    }
}

void screen_manager_distribute_event(const app_event_t *event)
{
    if (!s_state.initialized || event == NULL) {
        return;
    }

    for (int i = 0; i < SCREEN_MANAGER_MAX_SCREENS; i++) {
        screen_t *screen = s_state.screens[i];
        if (screen != NULL && screen->on_event != NULL) {
            screen->on_event(event);
        }
    }
}

void screen_manager_dispatch_input(const app_input_event_t *event)
{
    if (!s_state.initialized || event == NULL ||
        s_state.current_screen == SCREEN_NONE) {
        return;
    }

    screen_t *screen = s_state.screens[s_state.current_screen];
    if (screen != NULL && screen->on_input != NULL) {
        screen->on_input(event);
    }
}
