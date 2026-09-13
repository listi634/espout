/**
 * @file app_state.h
 * @brief Application state management.
 */

#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System operation modes.
 */
typedef enum {
    MODE_MANUAL = 0,
    MODE_AUTOMATIC,
    MODE_DEMO,
    MODE_COUNT
} system_mode_t;

/**
 * @brief Application state structure.
 */
typedef struct {
    bool power_on;
    int brightness;
    uint8_t led_red;
    uint8_t led_green;
    uint8_t led_blue;
    system_mode_t mode;
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
 * @brief Set power state.
 *
 * @param is_on true to turn on, false to turn off.
 * @return true if state changed, false if no change.
 */
bool app_state_set_power(bool is_on);

/**
 * @brief Set brightness value.
 *
 * @param value Brightness percentage (0-100).
 * @return true if state changed, false if no change.
 */
bool app_state_set_brightness(int value);

/**
 * @brief Set LED RGB color.
 *
 * @param red Red component (0-255).
 * @param green Green component (0-255).
 * @param blue Blue component (0-255).
 * @return true if state changed, false if no change.
 */
bool app_state_set_led_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Set system operation mode.
 *
 * @param mode The new system mode.
 * @return true if state changed, false if no change.
 */
bool app_state_set_mode(system_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
