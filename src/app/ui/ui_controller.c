/**
 * @file ui_controller.c
 * @brief UI controller implementation.
 * 
 * The UI controller manages the screen system and handles state-to-UI updates.
 */

#include "ui_controller.h"
#include "screens/screen_manager.h"
#include "screens/main_screen.h"
#include "../state/app_state.h"
#include "../event_bus/event_bus.h"
#include "../event_bus/event_types.h"
#include "esp_log.h"

static const char *TAG = "UI_CONTROLLER";

/**
 * @brief State change handler - updates UI when application state changes.
 */
static void state_change_handler(const app_event_t *event)
{
    (void)event;
    const app_state_t *state = app_state_get();

    main_screen_update_selection(state->selected_function);
}

/**
 * @brief Event handler for screen navigation and UI events.
 */
static void ui_event_handler(const app_event_t *event)
{
    if (event == NULL) {
        return;
    }

    // Distribute event to all screens
    screen_manager_distribute_event(event);
}

esp_err_t ui_controller_init(buzzer_handle_t buzzer)
{
    if (buzzer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize screen manager first
    esp_err_t ret = screen_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize screen manager: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize main screen
    ret = main_screen_init(buzzer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize main screen: %s", esp_err_to_name(ret));
        screen_manager_deinit();
        return ret;
    }

    // Subscribe to state change events
    app_event_type_t ui_events[] = {EVENT_FUNCTION_CHANGED};

    ret = event_bus_subscribe(
        ui_events, sizeof(ui_events) / sizeof(ui_events[0]),
        state_change_handler);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to state events");
        return ret;
    }

    // Also subscribe to all events for screen distribution
    // For now, just subscribe to the same events
    ret = event_bus_subscribe(
        ui_events, sizeof(ui_events) / sizeof(ui_events[0]),
        ui_event_handler);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to UI events");
        event_bus_unsubscribe(state_change_handler);
        return ret;
    }

    ESP_LOGI(TAG, "UI controller initialized");
    return ESP_OK;
}

void ui_controller_deinit(void)
{
    screen_manager_deinit();
    event_bus_unsubscribe(state_change_handler);
    event_bus_unsubscribe(ui_event_handler);
    ESP_LOGI(TAG, "UI controller deinitialized");
}
