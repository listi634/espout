#include "sdkconfig.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "utils/app_config.h"
#include "button_handler.h"

static const char *TAG = "MAIN_APP";

/**
 * @brief Application callback that handles button events asynchronously.
 */
static void on_main_button_event(button_event_t event, void *user_data) {
    switch (event) {
        case BUTTON_EVENT_SINGLE_PRESS:
            ESP_LOGI(TAG, "Button Action: Single Click detected! (Trigger Menu Confirm)");
            break;
        case BUTTON_EVENT_LONG_PRESS:
            ESP_LOGI(TAG, "Button Action: Long Press detected! (Trigger Back / Mode Change)");
            break;
        case BUTTON_EVENT_RELEASED:
            ESP_LOGD(TAG, "Button physically released.");
            break;
        default:
            break;
    }
}

void app_main(void) {
    /* Initialize dynamic logging whitelists first */
    ESP_ERROR_CHECK(app_config_init());

    /* Configure the standalone button config structure using Kconfig targets */
    const button_config_t btn_cfg = {
        .gpio_num = CONFIG_ESPOUT_BUTTON_GPIO, /* Gezogen aus deiner Kconfig.projbuild */
        .long_press_ms = 1000,                 /* 1 Sekunde gedrückt halten */
        .callback = on_main_button_event,      /* Unser Event-Verteiler */
        .user_data = NULL,
        .active_low = true                     /* Schaltet sauber gegen GND */
    };

    button_handle_t user_button = NULL;
    if (button_handler_init(&user_button, &btn_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to instantiate user action button framework!");
        return;
    }

    /* main.c task can now sleep or process other components peacefully */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}