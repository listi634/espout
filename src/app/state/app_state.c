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
    s_state.power_on = false;
    s_state.brightness = 0;
    s_state.led_red = 0;
    s_state.led_green = 0;
    s_state.led_blue = 0;
    s_state.mode = MODE_MANUAL;

    ESP_LOGI(TAG, "State initialized");
}

const app_state_t *app_state_get(void)
{
    return &s_state;
}

bool app_state_set_power(bool is_on)
{
    if (s_state.power_on == is_on) {
        return false;
    }

    s_state.power_on = is_on;

    if (!is_on) {
        s_state.brightness = 0;
        s_state.led_red = 0;
        s_state.led_green = 0;
        s_state.led_blue = 0;
    }

    ESP_LOGI(TAG, "Power: %s", is_on ? "ON" : "OFF");
    return true;
}

bool app_state_set_brightness(int value)
{
    if (value < 0) {
        value = 0;
    } else if (value > 100) {
        value = 100;
    }

    if (s_state.brightness == value) {
        return false;
    }

    s_state.brightness = value;
    ESP_LOGI(TAG, "Brightness: %d%%", value);
    return true;
}

bool app_state_set_led_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (s_state.led_red == red &&
        s_state.led_green == green &&
        s_state.led_blue == blue) {
        return false;
    }

    s_state.led_red = red;
    s_state.led_green = green;
    s_state.led_blue = blue;
    ESP_LOGI(TAG, "LED color: (%d, %d, %d)", red, green, blue);
    return true;
}

bool app_state_set_mode(system_mode_t mode)
{
    if (s_state.mode == mode) {
        return false;
    }

    s_state.mode = mode;
    ESP_LOGI(TAG, "Mode: %d", mode);
    return true;
}
