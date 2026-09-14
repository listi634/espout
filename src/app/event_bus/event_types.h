/**
 * @file event_types.h
 * @brief Application event type definitions.
 */

#ifndef APP_EVENT_TYPES_H
#define APP_EVENT_TYPES_H

#include <stdint.h>
#include "ir_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application event types.
 */
typedef enum {
    EVENT_IR_KEY_PRESSED,
    EVENT_FUNCTION_CHANGED,
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

    } data;
} app_event_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENT_TYPES_H */
