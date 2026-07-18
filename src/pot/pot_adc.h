/**
 * @file pot_adc.h
 * @brief Generalized reusable Potentiometer component driver using ESP-IDF v5.x.
 */

#ifndef POT_ADC_H
#define POT_ADC_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle representing a potentiometer instance.
 */
typedef struct pot_device *pot_device_handle_t;

/**
 * @brief Configuration structure for initializing a generalized potentiometer.
 */
typedef struct {
    adc_oneshot_unit_handle_t adc_unit_handle; /**< Pre-allocated ADC unit handle */
    gpio_num_t adc_gpio;                       /**< GPIO pin for analog reading */
    gpio_num_t vcc_gpio;                       /**< Optional GPIO to drive VCC rail (set GPIO_NUM_NC if unused) */
    gpio_num_t gnd_gpio;                       /**< Optional GPIO to drive GND rail (set GPIO_NUM_NC if unused) */
    adc_atten_t attenuation;                   /**< ADC attenuation (e.g., ADC_ATTEN_DB_12) */
    int min_raw_value;                         /**< Low calibration bound (usually 0) */
    int max_raw_value;                         /**< High calibration bound (usually 4095) */
} pot_config_t;

/**
 * @brief Initializes a potentiometer device instance and its designated power pins.
 * @param[out] out_handle Pointer to store the allocated device handle.
 * @param[in]  config     Pointer to the configuration structure.
 * @return esp_err_t      ESP_OK on success, or appropriate ESP-IDF error code.
 */
esp_err_t pot_adc_init(pot_device_handle_t *out_handle, const pot_config_t *config);

/**
 * @brief Reads the raw digital conversion value from the potentiometer.
 * @param[in]  handle  Potentiometer device handle.
 * @param[out] out_raw Pointer to store the raw reading (0-4095).
 * @return esp_err_t   ESP_OK on success.
 */
esp_err_t pot_adc_read_raw(pot_device_handle_t handle, int *out_raw);

/**
 * @brief Reads the potentiometer value scaled linearly to a custom range (e.g., 0-100%).
 * @param[in]  handle    Potentiometer device handle.
 * @param[in]  max_scale The upper limit of your desired target scale.
 * @param[out] out_scale Pointer to store the scaled resulting integer.
 * @return esp_err_t     ESP_OK on success.
 */
esp_err_t pot_adc_read_scaled(pot_device_handle_t handle, int max_scale, int *out_scale);

/**
 * @brief Frees all allocated memory resources and releases power pins.
 * @param[in] handle Potentiometer device handle to destroy.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t pot_adc_deinit(pot_device_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // POT_ADC_H