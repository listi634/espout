/**
 * @file main.c
 * @brief Application entry orchestration logic stitching components together.
 */

#include "sdkconfig.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "utils/app_config.h"
#include "pot_adc.h"

static const char *TAG = "MAIN_APP";

void app_main(void) {
    ESP_ERROR_CHECK(app_config_init());

    ESP_LOGI(TAG, "Booting Smarte Steuerzentrale Master Application Hub...");

    /* 1. Allocate shared ADC hardware peripheral instance unit safely at top layer */
    adc_oneshot_unit_handle_t adc1_handle = NULL;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    /* 2. Map dynamic settings out from generated Kconfig entries */
    const pot_config_t main_pot_cfg = {
        .adc_unit_handle = adc1_handle,
        .adc_gpio        = CONFIG_ESPOUT_POT_ADC_GPIO,
        .vcc_gpio        = CONFIG_ESPOUT_POT_VCC_GPIO,
        .gnd_gpio        = CONFIG_ESPOUT_POT_GND_GPIO,
        .attenuation     = ADC_ATTEN_DB_12,
        .min_raw_value   = 96,
        .max_raw_value   = 3312
    };

    /* 3. Initialize independent instance handle tracking runtime execution state */
    pot_device_handle_t navigation_pot = NULL;
    esp_err_t err = pot_adc_init(&navigation_pot, &main_pot_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Potentiometer runtime activation instantiation failed!");
        return;
    }

    /* 4. Infinite application processing poll event execution loop */
    while (1) {
        int scaled_percentage = 0;
        int raw_value = 0;
        
        if (pot_adc_read_scaled(navigation_pot, 100, &scaled_percentage) == ESP_OK) {
            ESP_LOGI(TAG, "Potentiometer Scale Position: %d%%", scaled_percentage);
        }

        if (pot_adc_read_raw(navigation_pot, &raw_value) == ESP_OK) {
            ESP_LOGI(TAG, "Potentiometer Raw Position: %d", raw_value);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}