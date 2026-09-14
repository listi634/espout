/**
 * @file action_handler.c
 * @brief Application action handler implementation.
 */

#include "action_handler.h"
#include "../state/app_state.h"
#include "../event_bus/event_types.h"
#include "../event_bus/event_bus.h"
#include "buzzer/buzzer_notes.h"
#include "esp_log.h"

static const char *TAG = "ACTION_HANDLER";
static buzzer_handle_t s_buzzer = NULL;

static void handle_ir_key_event(const app_event_t *event);

esp_err_t action_handler_init(buzzer_handle_t buzzer)
{
    if (buzzer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_buzzer = buzzer;
    ESP_LOGI(TAG, "Action handler initialized");
    return ESP_OK;
}

void action_handler_process_event(const app_event_t *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->type) {
        case EVENT_IR_KEY_PRESSED:
            handle_ir_key_event(event);
            break;
        default:
            ESP_LOGD(TAG, "Unhandled event type: %d", event->type);
            break;
    }

}

static void handle_ir_key_event(const app_event_t *event)
{
    const app_state_t *state = app_state_get();
    app_function_t next_function = state->selected_function;

    switch (event->data.ir.key) {
        case IR_KEY_UP:
            if (buzzer_beep(s_buzzer, NOTE_FS4, 80) != ESP_OK) {
                ESP_LOGW(TAG, "Failed to play IR key feedback");
            }
            next_function = (next_function + APP_FUNCTION_COUNT - 1) %
                            APP_FUNCTION_COUNT;
            break;
        case IR_KEY_DOWN:
            if (buzzer_beep(s_buzzer, NOTE_G4, 80) != ESP_OK) {
                ESP_LOGW(TAG, "Failed to play IR key feedback");
            }
            next_function = (next_function + 1) % APP_FUNCTION_COUNT;
            break;
        default:
            break;
    }

    if (next_function != state->selected_function &&
        app_state_set_selected_function(next_function)) {
        app_event_t changed_event = {.type = EVENT_FUNCTION_CHANGED};
        event_bus_publish(&changed_event);
    }
}
