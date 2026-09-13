/**
 * @file ui_controller.c
 * @brief UI controller implementation.
 */

#include "ui_controller.h"
#include "../state/app_state.h"
#include "../event_bus/event_bus.h"
#include "../event_bus/event_types.h"
#include "esp_log.h"
#include "../../lvgl/lvgl_ui.h"

static const char *TAG = "UI_CONTROLLER";

static const app_state_t *s_state = NULL;

static void state_change_handler(const app_event_t *event)
{
    (void)event;
    const app_state_t *state = app_state_get();

    lvgl_ui_update_power_status(state->power_on);
    lvgl_ui_update_brightness(state->brightness);
}

esp_err_t ui_controller_init(void)
{
    s_state = app_state_get();

    app_event_type_t ui_events[] = {
        EVENT_POWER_CHANGED,
        EVENT_BRIGHTNESS_CHANGED
    };

    esp_err_t ret = event_bus_subscribe(
        ui_events, sizeof(ui_events) / sizeof(ui_events[0]),
        state_change_handler);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to events");
        return ret;
    }

    ESP_LOGI(TAG, "UI controller initialized");
    return ESP_OK;
}
