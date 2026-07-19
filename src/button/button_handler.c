/**
 * @file button_handler.c
 * @brief Button driver implementation using an isolated FreeRTOS polling engine.
 */

#include "button_handler.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "BUTTON_DRIVER";

struct button_device {
    gpio_num_t gpio_num;
    uint32_t long_press_ms;
    button_callback_t callback;
    void *user_data;
    bool active_low;
    TaskHandle_t task_handle;
    bool is_running;
};

/**
 * @brief Internal FreeRTOS worker task evaluating physical debouncing logic loops.
 * @param[in] pvParameters Generic pointer passing the button structure container.
 */
static void button_worker_task(void *pvParameters) {
    button_handle_t dev = (button_handle_t)pvParameters;
    
    const uint32_t sample_period_ms = 20;
    uint32_t stable_state_counter = 0;
    bool current_stable_state = false;
    bool last_notified_state = false;
    uint32_t pressed_duration_ms = 0;
    bool long_press_triggered = false;

    while (dev->is_running) {
        /* Read physical logic layer input from the hardware register */
        bool raw_level = gpio_get_level(dev->gpio_num);
        bool physical_pressed = dev->active_low ? !raw_level : raw_level;

        /* Simple integrating software debouncer state machine */
        if (physical_pressed == current_stable_state) {
            stable_state_counter = 0;
        } else {
            stable_state_counter++;
            if (stable_state_counter >= 2) { /* Must be stable for 40ms */
                current_stable_state = physical_pressed;
                stable_state_counter = 0;
            }
        }

        /* Process interactions based on debounced stable system state */
        if (current_stable_state) {
            if (!last_notified_state) {
                /* Button transition from released to pressed */
                pressed_duration_ms = 0;
                long_press_triggered = false;
                last_notified_state = true;
            } else {
                /* Button is currently being held down */
                pressed_duration_ms += sample_period_ms;
                if ((pressed_duration_ms >= dev->long_press_ms) && !long_press_triggered) {
                    long_press_triggered = true;
                    if (dev->callback != NULL) {
                        dev->callback(BUTTON_EVENT_LONG_PRESS, dev->user_data);
                    }
                }
            }
        } else {
            if (last_notified_state) {
                /* Button transition from pressed to released */
                last_notified_state = false;
                if (dev->callback != NULL) {
                    if (!long_press_triggered) {
                        dev->callback(BUTTON_EVENT_SINGLE_PRESS, dev->user_data);
                    }
                    dev->callback(BUTTON_EVENT_RELEASED, dev->user_data);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(sample_period_ms));
    }

    vTaskDelete(NULL);
}

esp_err_t button_handler_init(button_handle_t *out_handle, const button_config_t *config) {
    ESP_RETURN_ON_FALSE(out_handle && config, ESP_ERR_INVALID_ARG, TAG, "Invalid initialization arguments");

    /* Allocate structure container instance memory safely */
    struct button_device *dev = malloc(sizeof(struct button_device));
    if (!dev) {
        return ESP_ERR_NO_MEM;
    }

    dev->gpio_num = config->gpio_num;
    dev->long_press_ms = config->long_press_ms;
    dev->callback = config->callback;
    dev->user_data = config->user_data;
    dev->active_low = config->active_low;
    dev->is_running = true;

    /* Configure physical GPIO configuration matrix registers */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << dev->gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = dev->active_low ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = dev->active_low ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE /* Polling inside task context, no raw ISR noise */
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        free(dev);
        return err;
    }

    /* Spawn background worker handler tracking tasks profile context */
    BaseType_t rc = xTaskCreate(button_worker_task, "btn_worker", 2048, dev, 4, &dev->task_handle);
    if (rc != pdPASS) {
        free(dev);
        return ESP_FAIL;
    }

    *out_handle = dev;
    ESP_LOGI(TAG, "Initialized reusable button monitor loop on GPIO %d", dev->gpio_num);
    return ESP_OK;
}

esp_err_t button_handler_deinit(button_handle_t handle) {
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Null pointer handle deinit error");
    handle->is_running = false;
    /* Memory container gets cleared inside worker lifecycle termination safely */
    free(handle);
    return ESP_OK;
}