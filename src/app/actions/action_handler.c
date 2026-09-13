/**
 * @file action_handler.c
 * @brief Application action handler implementation.
 */

#include "action_handler.h"
#include "../state/app_state.h"
#include "../event_bus/event_types.h"
#include "../event_bus/event_bus.h"
#include "esp_log.h"
#include "../../led/rgb_led.h"

static const char *TAG = "ACTION_HANDLER";

static const app_state_t *s_state = NULL;

static void handle_ir_key_event(const app_event_t *event,
                                 bool *power_changed,
                                 bool *brightness_changed);
static void handle_button_event(const app_event_t *event,
                                  bool *power_changed,
                                  bool *brightness_changed);
static void handle_potentiometer_event(const app_event_t *event,
                                        bool *brightness_changed);
static void update_led_from_state(void);
static void publish_state_events(bool power_changed, bool brightness_changed);

esp_err_t action_handler_init(void)
{
    s_state = app_state_get();
    ESP_LOGI(TAG, "Action handler initialized");
    return ESP_OK;
}

void action_handler_process_event(const app_event_t *event)
{
    if (event == NULL) {
        return;
    }

    bool power_changed = false;
    bool brightness_changed = false;

    switch (event->type) {
        case EVENT_IR_KEY_PRESSED:
            handle_ir_key_event(event, &power_changed, &brightness_changed);
            break;
        case EVENT_BUTTON_PRESSED:
        case EVENT_BUTTON_LONG_PRESSED:
        case EVENT_BUTTON_RELEASED:
            handle_button_event(event, &power_changed, &brightness_changed);
            break;
        case EVENT_POTENTIOMETER_CHANGED:
            handle_potentiometer_event(event, &brightness_changed);
            break;
        default:
            ESP_LOGD(TAG, "Unhandled event type: %d", event->type);
            break;
    }

    publish_state_events(power_changed, brightness_changed);
    update_led_from_state();
}

static void publish_state_events(bool power_changed, bool brightness_changed)
{
    if (!power_changed && !brightness_changed) {
        return;
    }

    if (power_changed) {
        app_event_t event = {.type = EVENT_POWER_CHANGED};
        event_bus_publish(&event);
    }

    if (brightness_changed) {
        app_event_t event = {.type = EVENT_BRIGHTNESS_CHANGED};
        event_bus_publish(&event);
    }
}

static void handle_ir_key_event(const app_event_t *event,
                                 bool *power_changed,
                                 bool *brightness_changed)
{
    switch (event->data.ir.key) {
        case IR_KEY_ON:
            if (app_state_set_power(true)) {
                *power_changed = true;
            }
            if (app_state_set_brightness(50)) {
                *brightness_changed = true;
            }
            break;
        case IR_KEY_OFF:
            if (app_state_set_power(false)) {
                *power_changed = true;
            }
            break;
        case IR_KEY_ZOOM_UP:
            if (s_state->power_on) {
                int new_brightness = s_state->brightness + 10;
                if (app_state_set_brightness(new_brightness)) {
                    *brightness_changed = true;
                }
            }
            break;
        case IR_KEY_ZOOM_DOWN:
            if (s_state->power_on) {
                int new_brightness = s_state->brightness - 10;
                if (app_state_set_brightness(new_brightness)) {
                    *brightness_changed = true;
                }
            }
            break;
        case IR_KEY_MENU:
            {
                system_mode_t next_mode = (s_state->mode + 1) % MODE_COUNT;
                app_state_set_mode(next_mode);
            }
            break;
        default:
            ESP_LOGD(TAG, "Unhandled IR key: %d", event->data.ir.key);
            break;
    }
}

static void handle_button_event(const app_event_t *event,
                                  bool *power_changed,
                                  bool *brightness_changed)
{
    switch (event->type) {
        case EVENT_BUTTON_PRESSED:
            if (s_state->power_on) {
                if (app_state_set_power(false)) {
                    *power_changed = true;
                }
            } else {
                if (app_state_set_power(true)) {
                    *power_changed = true;
                }
                if (app_state_set_brightness(50)) {
                    *brightness_changed = true;
                }
            }
            break;
        case EVENT_BUTTON_LONG_PRESSED:
            {
                system_mode_t next_mode = (s_state->mode + 1) % MODE_COUNT;
                app_state_set_mode(next_mode);
            }
            break;
        case EVENT_BUTTON_RELEASED:
            break;
        default:
            break;
    }
}

static void handle_potentiometer_event(const app_event_t *event,
                                        bool *brightness_changed)
{
    if (s_state->power_on) {
        if (app_state_set_brightness(event->data.potentiometer.value)) {
            *brightness_changed = true;
        }
    }
}

static void update_led_from_state(void)
{
    if (!s_state->power_on) {
        rgb_led_clear();
        return;
    }

    uint8_t led_value = (s_state->brightness * 255) / 100;
    rgb_led_set_color(led_value, led_value, led_value);
}
