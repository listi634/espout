/**
 * @file st7789.h
 * @brief Driver interface for the ST7789V2 1.69-inch SPI display.
 */

#ifndef ST7789_H
#define ST7789_H

#include "driver/spi_master.h"
#include "esp_err.h"
#include "lcd_assets.h"
#include <stdbool.h>
#include <stdint.h>

#define LCD_WIDTH  240
#define LCD_HEIGHT 280

/**
 * @brief Opaque handle representing an initialized ST7789 display device.
 */
typedef struct st7789_dev *st7789_handle_t;

/**
 * @brief Configuration parameters for ST7789 driver initialization.
 */
typedef struct {
    spi_host_device_t spi_host; /**< SPI peripheral host (e.g., SPI2_HOST) */
    uint32_t clock_speed_hz;    /**< SPI clock speed in Hz */
    int gpio_mosi;              /**< MOSI GPIO number */
    int gpio_clk;               /**< SCLK GPIO number */
    int gpio_cs;                /**< CS GPIO number */
    int gpio_dc;                /**< D/C GPIO number */
    int gpio_rst;               /**< Reset GPIO number */
    int gpio_bckl;              /**< Backlight GPIO number */
} st7789_config_t;

/**
 * @brief Initializes the SPI bus, control GPIOs, and ST7789 display controller.
 *
 * @param[in] config Pointer to initialization configuration structure.
 * @param[out] out_handle Pointer to variable where display handle will be stored.
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t st7789_init(const st7789_config_t *config, st7789_handle_t *out_handle);

/**
 * @brief Deinitializes the ST7789 display driver and frees allocated resources.
 *
 * @param[in] handle Display device handle.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t st7789_deinit(st7789_handle_t handle);

/**
 * @brief Enables or disables the screen backlight.
 *
 * @param[in] handle Display device handle.
 * @param[in] is_enabled True to enable backlight, false to disable.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t st7789_set_backlight(st7789_handle_t handle, bool is_enabled);

/**
 * @brief Fills the entire screen with a single RGB565 color using DMA.
 *
 * @param[in] handle Display device handle.
 * @param[in] color RGB565 color value.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t st7789_clear(st7789_handle_t handle, uint16_t color);

/**
 * @brief Draws a rectangular flash-resident image asset at specified coordinates.
 *
 * @param[in] handle Display device handle.
 * @param[in] x_start Starting X coordinate (0 to LCD_WIDTH - 1).
 * @param[in] y_start Starting Y coordinate (0 to LCD_HEIGHT - 1).
 * @param[in] image Pointer to image asset descriptor.
 * @return esp_err_t ESP_OK on success, or ESP_ERR_INVALID_ARG on boundary error.
 */
esp_err_t st7789_draw_image(st7789_handle_t handle,
                            uint16_t x_start,
                            uint16_t y_start,
                            const lcd_image_t *image);

/**
 * @brief Gets the SPI device handle for direct access.
 *
 * @param[in] handle Display device handle.
 * @return spi_device_handle_t The underlying SPI device handle.
 */
spi_device_handle_t st7789_get_spi_device(st7789_handle_t handle);

/**
 * @brief Gets the D/C GPIO number.
 *
 * @param[in] handle Display device handle.
 * @return int The D/C GPIO number.
 */
int st7789_get_gpio_dc(st7789_handle_t handle);

/**
 * @brief Sends a command byte to the ST7789 controller.
 *
 * @param[in] handle Display device handle.
 * @param[in] cmd Command byte to send.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t st7789_send_cmd(st7789_handle_t handle, uint8_t cmd);

/**
 * @brief Sends data bytes to the ST7789 controller.
 *
 * @param[in] handle Display device handle.
 * @param[in] data Pointer to data bytes.
 * @param[in] len Number of bytes to send.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t st7789_send_data(st7789_handle_t handle, const uint8_t *data, size_t len);

/**
 * @brief Sets the memory window boundaries for pixel data.
 *
 * @param[in] handle Display device handle.
 * @param[in] x1 Starting X coordinate (0 to LCD_WIDTH - 1).
 * @param[in] y1 Starting Y coordinate (0 to LCD_HEIGHT - 1).
 * @param[in] x2 Ending X coordinate (0 to LCD_WIDTH - 1).
 * @param[in] y2 Ending Y coordinate (0 to LCD_HEIGHT - 1).
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t st7789_set_window(st7789_handle_t handle, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

#endif /* ST7789_H */
