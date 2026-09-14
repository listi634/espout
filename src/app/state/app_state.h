/**
 * @file app_state.h
 * @brief Application state management.
 */

#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Selectable functions on the main screen.
 */
typedef enum {
    APP_FUNCTION_SETTINGS = 0,
    APP_FUNCTION_WEATHER,
    APP_FUNCTION_COUNT
} app_function_t;

/**
 * @brief Application state structure.
 */
typedef struct {
    app_function_t selected_function;
} app_state_t;

/**
 * @brief Initialize application state to default values.
 */
void app_state_init(void);

/**
 * @brief Get pointer to current application state.
 *
 * @return const pointer to the global state.
 */
const app_state_t *app_state_get(void);

/**
 * @brief Set the selected main-screen function.
 *
 * @param function The function to select.
 * @return true if state changed, false if no change.
 */
bool app_state_set_selected_function(app_function_t function);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
