/**
 * @file rgb_led.c
 * @brief Driver implementation for the on-board WS2812B RGB LED.
 */

#include "rgb_led.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "RGB_LED";
static led_strip_handle_t led_strip_handle = NULL;

esp_err_t rgb_led_init(void)
{
    /* Configure the led_strip device framework */
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_ESPOUT_RGB_LED_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    /* Configure RMT peripheral settings backend */
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000, /* 10MHz standard resolution */
        .flags.with_dma = false,
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to instantiate RMT led_strip device driver");
        return ret;
    }

    ESP_LOGI(TAG, "RGB LED initialized successfully on GPIO %d", CONFIG_ESPOUT_RGB_LED_GPIO);
    return led_strip_clear(led_strip_handle);
}

esp_err_t rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (led_strip_handle == NULL) {
        ESP_LOGE(TAG, "Driver context is invalid or uninitialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = led_strip_set_pixel(led_strip_handle, 0, red, green, blue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to assign pixel values to internal buffer");
        return ret;
    }

    return led_strip_refresh(led_strip_handle);
}

esp_err_t rgb_led_clear(void)
{
    if (led_strip_handle == NULL) {
        ESP_LOGE(TAG, "Driver context is invalid or uninitialized");
        return ESP_ERR_INVALID_STATE;
    }

    return led_strip_clear(led_strip_handle);
}