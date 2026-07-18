/**
 * @file pot_adc.c
 * @brief Potentiometer driver implementation encapsulating ESP-IDF oneshot APIs.
 */

#include "pot_adc.h"
#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "POT_ADC";

struct pot_device {
    adc_oneshot_unit_handle_t adc_handle;
    adc_channel_t adc_channel;
    gpio_num_t vcc_pin;
    gpio_num_t gnd_pin;
    int min_raw;
    int max_raw;
};

esp_err_t pot_adc_init(pot_device_handle_t *out_handle, const pot_config_t *config) {
    ESP_RETURN_ON_FALSE(out_handle && config, ESP_ERR_INVALID_ARG, TAG, "Invalid arguments");
    ESP_RETURN_ON_FALSE(config->adc_unit_handle, ESP_ERR_INVALID_ARG, TAG, "ADC unit handle required");

    /* Map physical GPIO to ESP32-C6 internal ADC Unit & Channel identifiers */
    adc_unit_t target_unit = ADC_UNIT_1;
    adc_channel_t target_channel = ADC_CHANNEL_0;
    esp_err_t err = adc_oneshot_io_to_channel(config->adc_gpio, &target_unit, &target_channel);
    ESP_RETURN_ON_ERROR(err, TAG, "GPIO %d is not a valid ADC pin", config->adc_gpio);

    /* Allocate structure container instance memory safely */
    struct pot_device *dev = malloc(sizeof(struct pot_device));
    if (!dev) {
        return ESP_ERR_NO_MEM;
    }

    dev->adc_handle = config->adc_unit_handle;
    dev->adc_channel = target_channel;
    dev->vcc_pin = config->vcc_gpio;
    dev->gnd_pin = config->gnd_gpio;
    dev->min_raw = config->min_raw_value;
    dev->max_raw = (config->max_raw_value == 0) ? 4095 : config->max_raw_value;

    /* Initialize VCC power rail pin if assigned */
    if (dev->vcc_pin >= 0) {
        gpio_config_t vcc_conf = {
            .pin_bit_mask = (1ULL << dev->vcc_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        err = gpio_config(&vcc_conf);
        if (err != ESP_OK) {
            free(dev);
            return err;
        }
        gpio_set_level(dev->vcc_pin, 1);
    }

    /* Initialize GND reference rail pin if assigned */
    if (dev->gnd_pin >= 0) {
        gpio_config_t gnd_conf = {
            .pin_bit_mask = (1ULL << dev->gnd_pin),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        err = gpio_config(&gnd_conf);
        if (err != ESP_OK) {
            free(dev);
            return err;
        }
        gpio_set_level(dev->gnd_pin, 0);
    }

    /* Configure allocated ADC hardware channel profile constraints */
    adc_oneshot_chan_cfg_t chan_conf = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = config->attenuation,
    };
    err = adc_oneshot_config_channel(dev->adc_handle, dev->adc_channel, &chan_conf);
    if (err != ESP_OK) {
        free(dev);
        return err;
    }

    *out_handle = dev;
    ESP_LOGI(TAG, "Initialized reusable pot instance on GPIO %d (Channel %d)", config->adc_gpio, dev->adc_channel);
    return ESP_OK;
}

esp_err_t pot_adc_read_raw(pot_device_handle_t handle, int *out_raw) {
    ESP_RETURN_ON_FALSE(handle && out_raw, ESP_ERR_INVALID_ARG, TAG, "Invalid handle state parameters");
    return adc_oneshot_read(handle->adc_handle, handle->adc_channel, out_raw);
}

esp_err_t pot_adc_read_scaled(pot_device_handle_t handle, int max_scale, int *out_scale) {
    ESP_RETURN_ON_FALSE(handle && out_scale && max_scale > 0, ESP_ERR_INVALID_ARG, TAG, "Invalid scaling parameter scale values");

    int raw_reading = 0;
    esp_err_t err = pot_adc_read_raw(handle, &raw_reading);
    if (err != ESP_OK) {
        return err;
    }

    /* Clamp values bounds within measured physical limits */
    if (raw_reading < handle->min_raw) {
        raw_reading = handle->min_raw;
    }
    if (raw_reading > handle->max_raw) {
        raw_reading = handle->max_raw;
    }

    /* Linear map scale mathematical evaluation formula */
    int raw_range = handle->max_raw - handle->min_raw;
    if (raw_range <= 0) {
        *out_scale = 0;
        return ESP_OK;
    }

    *out_scale = ((raw_reading - handle->min_raw) * max_scale) / raw_range;
    return ESP_OK;
}

esp_err_t pot_adc_deinit(pot_device_handle_t handle) {
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Null pointer handle deinit validation error");

    if (handle->vcc_pin >= 0) {
        gpio_set_level(handle->vcc_pin, 0);
    }
    free(handle);
    return ESP_OK;
}