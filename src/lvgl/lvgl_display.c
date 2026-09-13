/**
 * @file lvgl_display.c
 * @brief LVGL display driver implementation for ST7789V2 LCD.
 *
 * This module provides the glue between LVGL and the existing ST7789 display driver.
 * It implements the LVGL display interface (flush_cb) using direct SPI access
 * to the underlying ST7789 hardware.
 */

#include "lvgl_display.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include "lcd/st7789.h"

static const char *TAG = "LVGL_DISPLAY";

/**
 * @brief Custom tick source for LVGL using ESP32 system timer.
 */
static uint32_t custom_tick_get_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/**
 * @brief Internal driver state structure.
 */
typedef struct {
    st7789_handle_t st7789_handle; /**< Underlying ST7789 display handle */
    spi_device_handle_t spi_dev;   /**< SPI device handle for direct access */
    int gpio_dc;                  /**< D/C GPIO number */
    uint16_t *flush_buffer;       /**< Buffer for LVGL flush operations */
    size_t flush_buffer_size;     /**< Size of flush buffer in bytes */
} lvgl_display_driver_t;

/**
 * @brief LVGL flush callback for ST7789 display.
 *
 * Called by LVGL when it needs to send pixel data to the display.
 * This implementation uses direct SPI transactions for efficiency.
 *
 * @param[in] disp_drv LVGL display driver instance.
 * @param[in] area The area of the screen to update.
 * @param[in] map Color map of the area to update (as raw bytes).
 */
static void lvgl_flush_cb(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *map)
{
    static uint32_t flush_count = 0;
    flush_count++;
    
    lvgl_display_driver_t *driver = (lvgl_display_driver_t *)lv_display_get_user_data(disp_drv);

    if ((driver == NULL) || (driver->spi_dev == NULL)) {
        ESP_LOGE(TAG, "Flush callback called with invalid driver state");
        lv_display_flush_ready(disp_drv);
        return;
    }
    
    /* Clamp the area to display bounds using direct structure access */
    lv_coord_t x1 = area->x1;
    lv_coord_t y1 = area->y1;
    lv_coord_t x2 = area->x2;
    lv_coord_t y2 = area->y2;

    /* Ensure coordinates are within bounds */
    if (x1 >= LCD_WIDTH) x1 = LCD_WIDTH - 1;
    if (y1 >= LCD_HEIGHT) y1 = LCD_HEIGHT - 1;
    if (x2 >= LCD_WIDTH) x2 = LCD_WIDTH - 1;
    if (y2 >= LCD_HEIGHT) y2 = LCD_HEIGHT - 1;

    size_t width = (size_t)(x2 - x1 + 1);
    size_t height = (size_t)(y2 - y1 + 1);
    size_t pixel_count = width * height;

    /* Check if the area fits in our flush buffer */
    if (pixel_count * sizeof(uint16_t) > driver->flush_buffer_size) {
        ESP_LOGW(TAG, "Flush area too large for buffer, truncating");
        /* For simplicity, just skip this update */
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* Rotate bytes for SPI transfer */
    uint16_t *buf16 = (uint16_t *)map;
    for (size_t i = 0; i < pixel_count; i++) {
        buf16[i] = (buf16[i] >> 8) | (buf16[i] << 8);
    }

    /* Use ST7789 driver functions which properly handle D/C via pre-transfer callback */
    /* First, set the window address (this also sends Memory Write command 0x2C) */
    esp_err_t ret = st7789_set_window(driver->st7789_handle, x1, y1, x2, y2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set window: %s", esp_err_to_name(ret));
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* Send pixel data - st7789_send_data uses D/C=1 via pre-transfer callback */
    /* map contains LVGL's rendered pixels (already in correct format for ST7789) */
    ret = st7789_send_data(driver->st7789_handle, (const uint8_t *)map, pixel_count * sizeof(uint16_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send pixel data: %s", esp_err_to_name(ret));
    }

    /* Tell LVGL the flushing is done */
    lv_display_flush_ready(disp_drv);
}

lv_display_t *lvgl_display_init(st7789_handle_t st7789_handle)
{
    if (st7789_handle == NULL) {
        ESP_LOGE(TAG, "ST7789 handle is NULL");
        return NULL;
    }

    /* Initialize LVGL library */
    lv_init();

    /* Register hardware timer as LVGL tick source BEFORE any LVGL display operations */
    lv_tick_set_cb(custom_tick_get_cb);

    /* Allocate driver state */
    lvgl_display_driver_t *driver = calloc(1, sizeof(lvgl_display_driver_t));
    if (driver == NULL) {
        ESP_LOGE(TAG, "Failed to allocate driver memory");
        return NULL;
    }

    driver->st7789_handle = st7789_handle;

    /* Extract SPI device handle and GPIO using accessor functions */
    driver->spi_dev = st7789_get_spi_device(st7789_handle);
    driver->gpio_dc = st7789_get_gpio_dc(st7789_handle);

    /* Clear the display with black to start with a clean slate */
    ESP_ERROR_CHECK(st7789_clear(st7789_handle, 0x0000));

    if ((driver->spi_dev == NULL) || (driver->gpio_dc < 0)) {
        ESP_LOGE(TAG, "Failed to get SPI device or GPIO from ST7789 handle");
        free(driver);
        return NULL;
    }

    /* Allocate flush buffer (16 rows of RGB565 pixels to match ST7789 driver's max transfer size) */
    driver->flush_buffer_size = LCD_WIDTH * 16 * sizeof(uint16_t);
    driver->flush_buffer = heap_caps_malloc(driver->flush_buffer_size,
                                             MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (driver->flush_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate flush buffer");
        free(driver);
        return NULL;
    }

    /* Register LVGL display driver */
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        heap_caps_free(driver->flush_buffer);
        free(driver);
        return NULL;
    }

    /* Set up display buffers - allocate 2 buffers for double buffering */
    lv_display_set_buffers(disp, driver->flush_buffer, NULL, 
                           driver->flush_buffer_size / sizeof(uint16_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_user_data(disp, driver);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    /* Use RGB565 to match the ST7789 native transfer format */
    /* With LV_COLOR_16_SWAP enabled in lv_conf.h, LVGL generates byte-swapped (big-endian) RGB565 */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

    /* Set as default display */
    lv_display_set_default(disp);

    ESP_LOGI(TAG, "LVGL display driver initialized successfully (W=%d, H=%d)",
             LCD_WIDTH, LCD_HEIGHT);
    return disp;
}

esp_err_t lvgl_display_deinit(lv_display_t *disp)
{
    if (disp == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    lvgl_display_driver_t *driver = (lvgl_display_driver_t *)lv_display_get_user_data(disp);
    if (driver != NULL) {
        if (driver->flush_buffer != NULL) {
            heap_caps_free(driver->flush_buffer);
        }
        free(driver);
    }

    lv_display_delete(disp);
    return ESP_OK;
}

