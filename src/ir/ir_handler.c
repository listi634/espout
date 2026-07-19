/**
 * @file ir_handler.c
 * @brief Pure hardware driver implementation for NEC protocol decoding.
 */

#include "ir_handler.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/rmt_rx.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "IR";

#define IR_RESOLUTION_HZ        1000000 
#define BUFFER_SYMBOLS_NUM      64      
#define IR_QUEUE_SIZE           10
#define IR_DEBOUNCE_INTERVAL_MS 200

static rmt_symbol_word_t s_raw_symbols_buffer[BUFFER_SYMBOLS_NUM];
static ir_handler_callback_t s_client_callback = NULL;
static QueueHandle_t s_ir_rx_queue = NULL;

/* Pointers to injected configuration profile */
static const ir_lookup_entry_t *s_active_profile = NULL;
static size_t s_active_profile_size = 0;

/**
 * @brief Parses incoming raw RMT symbols down into a consistent 32-bit NEC frame.
 * @param symbols Pointer to the raw RMT symbols buffer.
 * @param num_symbols Number of symbols received.
 * @return uint32_t The decoded 32-bit NEC frame payload, or 0 if invalid.
 */
static uint32_t ir_parse_rmt_to_nec(const rmt_symbol_word_t *symbols, 
                                    int num_symbols)
{
    if (num_symbols < 34) {
        return 0;
    }

    uint32_t decoded_data = 0;

    for (int i = 0; i < 32; i++) {
        uint32_t high_duration = symbols[i + 1].duration1;
        decoded_data >>= 1;
        
        if (high_duration > 1100) {
            decoded_data |= 0x80000000;
        }
    }
    return decoded_data;
}

/**
 * @brief Iterates over the injected profile map to locate an identified code frame.
 * @param hex_code The 32-bit NEC hex code to search for.
 * @return const ir_lookup_entry_t* Matching profile entry or fallback entry.
 */
static const ir_lookup_entry_t *ir_search_key(uint32_t hex_code)
{
    static ir_lookup_entry_t unknown_fallback;

    if (s_active_profile != NULL) {
        for (size_t i = 0; i < s_active_profile_size; i++) {
            if (s_active_profile[i].hex_code == hex_code) {
                return &s_active_profile[i];
            }
        }
    }

    unknown_fallback.hex_code = hex_code;
    unknown_fallback.key_id = IR_KEY_UNKNOWN;
    unknown_fallback.key_name = "UNKNOWN";
    return &unknown_fallback;
}

/**
 * @brief RMT RX interrupt callback to dispatch event data into processing queue.
 */
static bool rmt_rx_done_callback(rmt_channel_handle_t rx_chan, 
                                 const rmt_rx_done_event_data_t *edata, 
                                 void *user_data)
{
    QueueHandle_t queue = (QueueHandle_t)user_data;
    BaseType_t high_task_wakeup = pdFALSE;
    xQueueSendFromISR(queue, edata, &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}

/**
 * @brief FreeRTOS task handling queue processing, debouncing, and filtering.
 */
static void ir_handler_task(void *pvParameters)
{
    rmt_channel_handle_t rx_channel = (rmt_channel_handle_t)pvParameters;
    rmt_rx_done_event_data_t rx_event_data;
    
    /* Variables for software debouncing logic */
    ir_key_t last_valid_key = IR_KEY_UNKNOWN;
    TickType_t last_valid_key_time = 0;

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 1000,
        .signal_range_max_ns = 30000000,
    };

    ESP_ERROR_CHECK(rmt_receive(rx_channel, s_raw_symbols_buffer, 
                                sizeof(s_raw_symbols_buffer), &receive_config));

    while (1) {
        if (xQueueReceive(s_ir_rx_queue, &rx_event_data, portMAX_DELAY) == pdTRUE) {
            uint32_t decoded_hex = ir_parse_rmt_to_nec(rx_event_data.received_symbols, 
                                                       rx_event_data.num_symbols);

            if (decoded_hex != 0) {
                const ir_lookup_entry_t *entry = ir_search_key(decoded_hex);

                if (entry->key_id == IR_KEY_UNKNOWN) {
                    /* Suppress noise and only log on Debug level to clear the monitor */
                    ESP_LOGD(TAG, "Noise detected | Code: 0x%08X", (unsigned int)decoded_hex);
                } else {
                    TickType_t current_time = xTaskGetTickCount();
                    TickType_t elapsed_ticks = current_time - last_valid_key_time;
                    uint32_t elapsed_ms = pdTICKS_TO_MS(elapsed_ticks);

                    /* Software Debounce: Block rapid duplicate identical frames */
                    if ((entry->key_id == last_valid_key) && 
                        (elapsed_ms < IR_DEBOUNCE_INTERVAL_MS)) {
                        ESP_LOGD(TAG, "Suppressed duplicate key: %s (Debounce)", entry->key_name);
                    } else {
                        ESP_LOGI(TAG, "Key Transmitted: %s | Code: 0x%08X", 
                                 entry->key_name, (unsigned int)entry->hex_code);
                        
                        last_valid_key = entry->key_id;
                        last_valid_key_time = current_time;

                        if (s_client_callback != NULL) {
                            s_client_callback(entry->key_id, entry);
                        }
                    }
                }
            } else if (rx_event_data.num_symbols > 2) {
                ESP_LOGD(TAG, "NEC Repeat frame detected.");
                /* Optional: reset debounce timer here if you want to support continuous holding */
            }

            ESP_ERROR_CHECK(rmt_receive(rx_channel, s_raw_symbols_buffer, 
                                        sizeof(s_raw_symbols_buffer), &receive_config));
        }
    }
}

esp_err_t ir_handler_init(const ir_lookup_entry_t *profile, 
                          size_t profile_size, 
                          ir_handler_callback_t callback)
{
    if ((profile == NULL) || (profile_size == 0) || (callback == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    s_active_profile = profile;
    s_active_profile_size = profile_size;
    s_client_callback = callback;

    gpio_config_t vcc_io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_ESPOUT_IR_VCC_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&vcc_io_conf), TAG, "Failed VCC pin configuration");
    ESP_RETURN_ON_ERROR(gpio_set_level(CONFIG_ESPOUT_IR_VCC_GPIO, 1), TAG, "Failed powering IR rail");

    s_ir_rx_queue = xQueueCreate(IR_QUEUE_SIZE, sizeof(rmt_rx_done_event_data_t));
    if (s_ir_rx_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    rmt_rx_channel_config_t rx_chan_config = {
        .gpio_num = CONFIG_ESPOUT_IR_OUT_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = IR_RESOLUTION_HZ,
        .mem_block_symbols = BUFFER_SYMBOLS_NUM,
        .flags.with_dma = false,
    };
    rmt_channel_handle_t rx_channel = NULL;
    ESP_RETURN_ON_ERROR(rmt_new_rx_channel(&rx_chan_config, &rx_channel), TAG, "Failed creating channel");

    rmt_rx_event_callbacks_t cbs = { 
        .on_recv_done = rmt_rx_done_callback 
    };
    ESP_RETURN_ON_ERROR(rmt_rx_register_event_callbacks(rx_channel, &cbs, s_ir_rx_queue), TAG, "Failed callback assign");
    ESP_RETURN_ON_ERROR(rmt_enable(rx_channel), TAG, "Failed enabling peripheral");

    BaseType_t task_created = xTaskCreate(ir_handler_task, "ir_handler_task", 3072, rx_channel, 10, NULL);
    if (task_created != pdPASS) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Generic IR driver loaded with explicit runtime layout configuration profile.");
    return ESP_OK;
}