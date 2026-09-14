/**
 * @file event_types.h
 * @brief Application event type definitions.
 */

#ifndef APP_EVENT_TYPES_H
#define APP_EVENT_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application event types.
 */
typedef enum {
    EVENT_FUNCTION_CHANGED,
} app_event_type_t;

/**
 * @brief Application event structure.
 */
typedef struct {
    app_event_type_t type;

} app_event_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENT_TYPES_H */
