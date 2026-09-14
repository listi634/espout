#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "ir_handler.h"
#include "ir_config.h"
#include "lcd/st7789.h"
#include "lvgl/lvgl_display.h"
#include "utils/app_config.h"
#include "app/event_bus/event_bus.h"
#include "app/state/app_state.h"
#include "app/actions/action_handler.h"
#include "app/ui/ui_controller.h"
#include "buzzer/buzzer.h"

static const char *TAG = "MAIN";

/**
 * @brief LCD display handle.
 */
static st7789_handle_t g_lcd_handle = NULL;

/**
 * @brief LVGL display handle.
 */
static lv_display_t *g_lvgl_disp = NULL;

/**
 * @brief Passive buzzer handle.
 */
static buzzer_handle_t g_buzzer_handle = NULL;

/**
 * @brief Initializes the LCD display with configuration from Kconfig.
 *
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
static esp_err_t lcd_display_init(void)
{
    st7789_config_t lcd_config = {
        .spi_host = SPI2_HOST,
        .clock_speed_hz = 10 * 1000 * 1000,
        .gpio_mosi = CONFIG_ESPOUT_LCD_DIN_GPIO,
        .gpio_clk = CONFIG_ESPOUT_LCD_SCLK_GPIO,
        .gpio_cs = CONFIG_ESPOUT_LCD_CS_GPIO,
        .gpio_dc = CONFIG_ESPOUT_LCD_DC_GPIO,
        .gpio_rst = CONFIG_ESPOUT_LCD_RESET_GPIO,
        .gpio_bckl = CONFIG_ESPOUT_LCD_BACKLIGHT_GPIO
    };

    esp_err_t ret = st7789_init(&lcd_config, &g_lcd_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "LCD initialized successfully");
    return ESP_OK;
}

/**
 * @brief IR event callback that publishes to event bus.
 */
static void ir_event_publisher(ir_key_t key, const ir_lookup_entry_t *entry)
{
    app_event_t event = {
        .type = EVENT_IR_KEY_PRESSED,
        .data.ir = {
            .key = key,
            .entry = entry
        }
    };
    event_bus_publish(&event);
}

void app_main(void)
{
    ESP_ERROR_CHECK(app_config_init());

    ESP_LOGI(TAG, "Starting Master Hub Application...");

    /* Initialize Components */
    ESP_ERROR_CHECK(lcd_display_init());
    /* Initialize LVGL display driver (lv_init() is called internally) */
    g_lvgl_disp = lvgl_display_init(g_lcd_handle);
    if (g_lvgl_disp == NULL) {
        ESP_LOGE(TAG, "Failed to initialize LVGL display driver");
        return;
    }

    /* Initialize Application Layer */
    ESP_ERROR_CHECK(event_bus_init());
    app_state_init();
    const buzzer_config_t buzzer_config = {
        .gpio_num = CONFIG_ESPOUT_BUZZER_GPIO,
        .timer_num = LEDC_TIMER_0,
        .channel = LEDC_CHANNEL_0
    };
    ESP_ERROR_CHECK(buzzer_init(&g_buzzer_handle, &buzzer_config));
    ESP_ERROR_CHECK(action_handler_init(g_buzzer_handle));
    ESP_ERROR_CHECK(ui_controller_init());

    /* Subscribe action handler to hardware events */
    app_event_type_t action_events[] = {
        EVENT_IR_KEY_PRESSED
    };
    event_bus_subscribe(
        action_events,
        sizeof(action_events) / sizeof(action_events[0]),
        action_handler_process_event);

    /* Initialize IR handler with event publishing */
    size_t table_elements = sizeof(s_ir_profile_benq) / sizeof(s_ir_profile_benq[0]);
    ESP_ERROR_CHECK(ir_handler_init(s_ir_profile_benq, table_elements, ir_event_publisher));

    ESP_LOGI(TAG, "Application started successfully");
}
