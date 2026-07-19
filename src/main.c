#include <stdio.h>
#include "esp_log.h"
#include "ir_handler.h"
#include "ir_config.h"  // Enthält die globalen Tabellen
#include "utils/app_config.h"

static const char *TAG = "MAIN";

/**
 * @brief Application action layer processing identified key event codes.
 */
static void main_ir_event_callback(ir_key_t key, const ir_lookup_entry_t *entry)
{
    switch (key) {
        case IR_KEY_ON:
            ESP_LOGW(TAG, "Global Action: ON detected -> Booting peripherals.");
            break;
        case IR_KEY_VOLUME_UP:
            ESP_LOGI(TAG, "Global Action: VOLUME_UP -> Updating UI.");
            break;
        default:
            break;
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(app_config_init());

    ESP_LOGI(TAG, "Starting Master Hub Application...");

    /* Wir injizieren die globale Tabelle und deren Größe direkt beim Init */
    size_t table_elements = sizeof(s_ir_profile_benq) / sizeof(s_ir_profile_benq[0]);
    
    ESP_ERROR_CHECK(ir_handler_init(s_ir_profile_benq, table_elements, main_ir_event_callback));
}