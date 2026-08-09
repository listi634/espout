#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "ir_handler.h"
#include "ir_config.h"
#include "lcd/st7789.h"
#include "lcd/lcd_assets.h"
#include "utils/app_config.h"

static const char *TAG = "MAIN";

/**
 * @brief LCD display handle.
 */
static st7789_handle_t g_lcd_handle = NULL;

/**
 * @brief Initializes the LCD display with configuration from Kconfig.
 *
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
static esp_err_t lcd_display_init(void)
{
    st7789_config_t lcd_config = {
        .spi_host = SPI2_HOST,
        .clock_speed_hz = 10 * 1000 * 1000,  /* 10 MHz */
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
 * @brief Displays the test image on the LCD screen.
 *
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
static esp_err_t lcd_display_test_image(const lcd_image_t *image)
{
    if (g_lcd_handle == NULL) {
        ESP_LOGE(TAG, "LCD not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Clear screen with black color */
    esp_err_t ret = st7789_clear(g_lcd_handle, 0x0000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear LCD: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Draw test image at top-left corner (0, 0) */
    ret = st7789_draw_image(g_lcd_handle, 0, 0, image);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to draw test image: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Test image displayed successfully");
    return ESP_OK;
}

/**
 * @brief Application action layer processing identified key event codes.
 */
static void main_ir_event_callback(ir_key_t key, const ir_lookup_entry_t *entry)
{
    switch (key) {
        case IR_KEY_ON:
            ESP_ERROR_CHECK(lcd_display_test_image(&g_lcd_img1));
            ESP_LOGW(TAG, "Global Action: ON detected.");
            break;
        case IR_KEY_OFF:
            ESP_ERROR_CHECK(lcd_display_test_image(&g_lcd_img2));
            ESP_LOGI(TAG, "Global Action: OFF detected.");
            break;
        default:
            break;
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(app_config_init());

    ESP_LOGI(TAG, "Starting Master Hub Application...");

    /* Initialize LCD display */
    ESP_ERROR_CHECK(lcd_display_init());

    /* Display test image */
    ESP_ERROR_CHECK(lcd_display_test_image(&g_lcd_img1));

    /* Initialize IR handler */
    size_t table_elements = sizeof(s_ir_profile_benq) / sizeof(s_ir_profile_benq[0]);
    ESP_ERROR_CHECK(ir_handler_init(s_ir_profile_benq, table_elements, main_ir_event_callback));
}