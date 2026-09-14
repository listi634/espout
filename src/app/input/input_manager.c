/**
 * @file input_manager.c
 * @brief Application input source manager implementation.
 */

#include "input_manager.h"
#include "input_event.h"
#include "pot_monitor.h"
#include "../../ir/ir_handler.h"
#include "../ui/screens/screen_manager.h"

static void ir_input_callback(
    ir_key_t key,
    const ir_lookup_entry_t *entry)
{
    input_manager_process_ir(key, entry);
}

void input_manager_process_pot(int8_t delta)
{
    const app_input_event_t event = {
        .type = APP_INPUT_POT_STEP,
        .data.pot = {.delta = delta}
    };
    screen_manager_dispatch_input(&event);
}

esp_err_t input_manager_init(
    const ir_lookup_entry_t *ir_profile,
    size_t ir_profile_size)
{
    if (ir_profile == NULL || ir_profile_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = pot_monitor_init(input_manager_process_pot);
    if (err != ESP_OK) {
        return err;
    }

    return ir_handler_init(ir_profile, ir_profile_size, ir_input_callback);
}

void input_manager_process_ir(ir_key_t key, const ir_lookup_entry_t *entry)
{
    const app_input_event_t event = {
        .type = APP_INPUT_IR_KEY,
        .data.ir = {.key = key, .entry = entry}
    };
    screen_manager_dispatch_input(&event);
}