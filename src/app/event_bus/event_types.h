/**
 * @file event_types.h
 * @brief Application event type definitions.
 */

#ifndef APP_EVENT_TYPES_H
#define APP_EVENT_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "ir_config.h"
#include "button_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application event types.
 */
typedef enum {
    EVENT_IR_KEY_PRESSED,
    EVENT_BUTTON_PRESSED,
    EVENT_BUTTON_LONG_PRESSED,
    EVENT_BUTTON_RELEASED,
    EVENT_POTENTIOMETER_CHANGED,
    EVENT_POWER_CHANGED,
    EVENT_BRIGHTNESS_CHANGED,
} app_event_type_t;

/**
 * @brief Application event structure.
 */
typedef struct {
    app_event_type_t type;

    union {
        struct {
            ir_key_t key;
            const ir_lookup_entry_t *entry;
        } ir;

        struct {
            button_event_t button_event;
            int button_id;
        } button;

        struct {
            int value;
        } potentiometer;

        struct {
            bool is_on;
        } power;

        struct {
            int value;
        } brightness;
    } data;
} app_event_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENT_TYPES_H */
