/**
 * @file pot_monitor.c
 * @brief Relative potentiometer movement monitor implementation.
 */

#include "pot_monitor.h"
#include "../../pot/pot_adc.h"
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "POT_MONITOR";
static const uint32_t SAMPLE_PERIOD_MS = 50;
static const int STEP_THRESHOLD = 300;
static const int MAX_STEPS_PER_SAMPLE = 4;

static pot_device_handle_t s_pot_handle = NULL;
static pot_monitor_callback_t s_callback = NULL;

static void pot_monitor_task(void *arg)
{
    (void)arg;
    int previous_raw = 0;
    int accumulated_delta = 0;
    bool has_baseline = false;

    while (true) {
        int current_raw = 0;
        esp_err_t err = pot_adc_read_raw(s_pot_handle, &current_raw);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Potentiometer read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
            continue;
        }

        if (!has_baseline) {
            previous_raw = current_raw;
            has_baseline = true;
        } else {
            accumulated_delta += current_raw - previous_raw;
            previous_raw = current_raw;

            int steps = 0;
            while (accumulated_delta >= STEP_THRESHOLD &&
                   steps < MAX_STEPS_PER_SAMPLE) {
                accumulated_delta -= STEP_THRESHOLD;
                steps++;
            }
            while (accumulated_delta <= -STEP_THRESHOLD &&
                   steps > -MAX_STEPS_PER_SAMPLE) {
                accumulated_delta += STEP_THRESHOLD;
                steps--;
            }

            if (steps != 0 && s_callback != NULL) {
                s_callback((int8_t)steps);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}

esp_err_t pot_monitor_init(pot_monitor_callback_t callback)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_oneshot_unit_handle_t adc_unit = NULL;
    const adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    esp_err_t err = adc_oneshot_new_unit(&adc_config, &adc_unit);
    if (err != ESP_OK) {
        return err;
    }

    const pot_config_t pot_config = {
        .adc_unit_handle = adc_unit,
        .adc_gpio = CONFIG_ESPOUT_POT_ADC_GPIO,
        .vcc_gpio = CONFIG_ESPOUT_POT_VCC_GPIO,
        .gnd_gpio = CONFIG_ESPOUT_POT_GND_GPIO,
        .attenuation = ADC_ATTEN_DB_12,
        .min_raw_value = 0,
        .max_raw_value = 4095
    };
    err = pot_adc_init(&s_pot_handle, &pot_config);
    if (err != ESP_OK) {
        adc_oneshot_del_unit(adc_unit);
        return err;
    }

    s_callback = callback;
    if (xTaskCreate(pot_monitor_task, "pot_monitor", 3072, NULL, 4, NULL) !=
        pdPASS) {
        s_callback = NULL;
        pot_adc_deinit(s_pot_handle);
        s_pot_handle = NULL;
        adc_oneshot_del_unit(adc_unit);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Potentiometer monitor initialized");
    return ESP_OK;
}