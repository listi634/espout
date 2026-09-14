/**
 * @file app_state.c
 * @brief Application state management implementation.
 */

#include "app_state.h"
#include "esp_log.h"

static const char *TAG = "APP_STATE";

static app_state_t s_state = {0};

void app_state_init(void)
{
    s_state.selected_function = APP_FUNCTION_SETTINGS;

    ESP_LOGI(TAG, "State initialized");
}

const app_state_t *app_state_get(void)
{
    return &s_state;
}

bool app_state_set_selected_function(app_function_t function)
{
    if (function < 0 || function >= APP_FUNCTION_COUNT) {
        return false;
    }

    if (s_state.selected_function == function) {
        return false;
    }

    s_state.selected_function = function;
    ESP_LOGI(TAG, "Selected function: %d", function);
    return true;
}
