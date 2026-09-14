/**
 * @file app_config.c
 * @brief Centralized runtime environment settings and log whitelisting.
 *
 * Strictly adheres to table-driven design conventions and fetches all
 * parameters dynamically via sdkconfig mapping parameters.
 */

#include "app_config.h"
#include <stddef.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "APP_CONFIG";

/**
 * @brief Original vprintf function pointer stored during hook registration.
 */
static vprintf_like_t s_original_vprintf = NULL;

/**
 * @brief Structure mapping a specific component logging tag to its verbosity.
 */
typedef struct {
    const char *tag;        /**< The exact string identifier used in ESP_LOGx */
    esp_log_level_t level;  /**< Target verbosity limit (e.g., ESP_LOG_INFO) */
} log_whitelist_entry_t;

/**
 * @brief Log whitelist config.
 */
static const log_whitelist_entry_t log_whitelist[] = {
    { "MAIN",           ESP_LOG_DEBUG  },
    { "POT",            ESP_LOG_DEBUG  },
    { "POT_MONITOR",    ESP_LOG_DEBUG  },
    { "BUTTON",         ESP_LOG_DEBUG  },
    { "IR",             ESP_LOG_DEBUG  },
    { "INPUT_MANAGER",  ESP_LOG_DEBUG  },
    { "LCD",            ESP_LOG_DEBUG  },
    { "LVGL_DISPLAY",   ESP_LOG_DEBUG  },
    { "MAIN_SCREEN",    ESP_LOG_DEBUG  },
    { "SCREEN_MANAGER", ESP_LOG_DEBUG  },
    { "APP_STATE",      ESP_LOG_DEBUG  },
    { "UI_CONTROLLER",  ESP_LOG_DEBUG  },
    { "EVENT_BUS",      ESP_LOG_DEBUG  },
    { "RGB_LED",        ESP_LOG_DEBUG  },
    { "BUZZER",         ESP_LOG_DEBUG  }
};

typedef struct {
    const char *name;
    int gpio;
} gpio_assignment_t;

static esp_err_t validate_gpio_assignments(void)
{
    const gpio_assignment_t assignments[] = {
        { "LCD backlight", CONFIG_ESPOUT_LCD_BACKLIGHT_GPIO },
        { "LCD reset", CONFIG_ESPOUT_LCD_RESET_GPIO },
        { "LCD DC", CONFIG_ESPOUT_LCD_DC_GPIO },
        { "LCD CS", CONFIG_ESPOUT_LCD_CS_GPIO },
        { "LCD clock", CONFIG_ESPOUT_LCD_SCLK_GPIO },
        { "LCD data", CONFIG_ESPOUT_LCD_DIN_GPIO },
        { "IR VCC", CONFIG_ESPOUT_IR_VCC_GPIO },
        { "IR signal", CONFIG_ESPOUT_IR_OUT_GPIO },
        { "button", CONFIG_ESPOUT_BUTTON_GPIO },
        { "potentiometer VCC", CONFIG_ESPOUT_POT_VCC_GPIO },
        { "potentiometer GND", CONFIG_ESPOUT_POT_GND_GPIO },
        { "potentiometer ADC", CONFIG_ESPOUT_POT_ADC_GPIO },
        { "buzzer", CONFIG_ESPOUT_BUZZER_GPIO },
        { "RGB LED", CONFIG_ESPOUT_RGB_LED_GPIO }
    };
    const size_t assignment_count =
        sizeof(assignments) / sizeof(assignments[0]);

    for (size_t first = 0; first < assignment_count; first++) {
        for (size_t second = first + 1; second < assignment_count; second++) {
            if (assignments[first].gpio == assignments[second].gpio) {
                ESP_LOGE(TAG,
                         "Hardware conflict: %s and %s share GPIO %d",
                         assignments[first].name,
                         assignments[second].name,
                         assignments[first].gpio);
                return ESP_ERR_INVALID_ARG;
            }
        }
    }

    return ESP_OK;
}

/**
 * @brief Helper function to route formatted strings safely through the vprintf pipe.
 *
 * @param[in] format Format specifier string.
 * @param[in] ...    Variable argument values.
 * @return int       Number of characters written.
 */
static int call_original_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = 0;
    
    if (s_original_vprintf != NULL) {
        result = s_original_vprintf(format, args);
    } else {
        result = vprintf(format, args);
    }
    
    va_end(args);
    return result;
}

/**
 * @brief Custom vprintf hook callback to reset logging colors right after the prefix tag.
 *
 * Captures compiled color log strings, identifies the colon separator, and injects 
 * an ANSI color reset escape sequence before printing the main message body text.
 *
 * @param[in] format Format string passed by ESP_LOGx macros.
 * @param[in] args   Variable argument pointer list.
 * @return int       Number of formatted characters written.
 */
static int custom_log_vprintf(const char *format, va_list args) {
    char log_buffer[512];
    
    /* Format incoming message components into our local buffer safely */
    int written = vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    if (written < 0) {
        return 0;
    }

    /* Find the standard ESP-IDF component separator ': ' */
    char *separator = strstr(log_buffer, ": ");
    if (separator != NULL) {
        char *message_body = separator + 2;
        char old_char = *message_body;
        
        /* Temporary string termination to isolate the prefix portion */
        *message_body = '\0';
        call_original_printf("%s", log_buffer);
        
        /* Inject ANSI Escape sequence to clear colors back to default white */
        call_original_printf("\033[0m");
        
        /* Restore message payload content and print it cleanly */
        *message_body = old_char;
        call_original_printf("%s", message_body);
    } else {
        /* Fallback for logs without structured tags (e.g., raw panic dumps) */
        call_original_printf("%s", log_buffer);
    }

    return written;
}

esp_err_t app_config_init(void) {
    /* Calculate array element boundaries safely at compile time via sizeof */
    const size_t entry_count = sizeof(log_whitelist) / sizeof(log_whitelist[0]);
    
    /* Iterate through the data matrix to apply active runtime filters */
    for (size_t whitelist_index = 0; whitelist_index < entry_count; whitelist_index++) {
        esp_log_level_set(log_whitelist[whitelist_index].tag, 
                          log_whitelist[whitelist_index].level);
    }

    /* Intercept the logging framework with the color-splitting handler */
    s_original_vprintf = esp_log_set_vprintf(custom_log_vprintf);

    ESP_LOGI(TAG, "Runtime logging whitelist matrix parsed successfully.");

    return validate_gpio_assignments();
}