/**
 * @file input_event.h
 * @brief Application input event definitions.
 */

#ifndef APP_INPUT_EVENT_H
#define APP_INPUT_EVENT_H

#include <stdint.h>
#include "ir_config.h"

typedef enum {
    APP_INPUT_IR_KEY,
    APP_INPUT_POT_STEP
} app_input_type_t;

typedef struct {
    app_input_type_t type;
    union {
        struct {
            ir_key_t key;
            const ir_lookup_entry_t *entry;
        } ir;
        struct {
            int8_t delta;
        } pot;
    } data;
} app_input_event_t;

#endif /* APP_INPUT_EVENT_H */