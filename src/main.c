/**
 * @file main.c
 * @brief Application entry orchestration logic stitching components together.
 */

#include "sdkconfig.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "pot_adc.h"
#include "rgb_led.h"

static const char *TAG = "MAIN_APP";

/**
 * @brief Maps a percentage value (0-100) to smooth RGB color transitions.
 * @param percentage The scaled percentage from the potentiometer.
 * @param r Pointer to store the output red intensity.
 * @param g Pointer to store the output green intensity.
 * @param b Pointer to store the output blue intensity.
 */
static void map_percentage_to_rgb(int percentage, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (percentage < 0) {
        percentage = 0;
    }
    if (percentage > 100) {
        percentage = 100;
    }

    /* Divide into 4 color transition zones */
    if (percentage < 25) {
        /* Zone 1: Red to Yellow (Green increases) */
        *r = 255;
        *g = (uint8_t)((percentage * 4) * 255 / 100);
        *b = 0;
    } else if (percentage < 50) {
        /* Zone 2: Yellow to Green (Red decreases) */
        int local_pct = percentage - 25;
        *r = (uint8_t)(255 - ((local_pct * 4) * 255 / 100));
        *g = 255;
        *b = 0;
    } else if (percentage < 75) {
        /* Zone 3: Green to Cyan (Blue increases) */
        int local_pct = percentage - 50;
        *r = 0;
        *g = 255;
        *b = (uint8_t)((local_pct * 4) * 255 / 100);
    } else {
        /* Zone 4: Cyan to Blue (Green decreases) */
        int local_pct = percentage - 75;
        *r = 0;
        *g = (uint8_t)(255 - ((local_pct * 4) * 255 / 100));
        *b = 255;
    }
}

void app_main(void) {
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

    /* 4. Initialize the on-board RGB LED peripheral driver framework */
    err = rgb_led_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RGB LED driver core initialization failed!");
        return;
    }

    /* 5. Infinite application processing poll event execution loop */
    while (1) {
        int scaled_percentage = 0;
        int raw_value = 0;
        
        if (pot_adc_read_scaled(navigation_pot, 100, &scaled_percentage) == ESP_OK) {
            ESP_LOGI(TAG, "Potentiometer Scale Position: %d%%", scaled_percentage);
            
            /* Calculate and apply corresponding RGB colors */
            uint8_t red = 0;
            uint8_t green = 0;
            uint8_t blue = 0;
            map_percentage_to_rgb(scaled_percentage, &red, &green, &blue);
            
            rgb_led_set_color(red, green, blue);
        }

        if (pot_adc_read_raw(navigation_pot, &raw_value) == ESP_OK) {
            ESP_LOGD(TAG, "Potentiometer Raw Position: %d", raw_value);
        }

        /* Short delay slightly for responsive LED updating during twists */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}