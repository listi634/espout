/**
 * @file st7789.c
 * @brief High-performance DMA-accelerated ST7789 display driver.
 */

#include "st7789.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define ST7789_ROW_OFFSET   20
#define ST7789_DMA_BUF_SIZE (LCD_WIDTH * 16 * sizeof(uint16_t))

static const char *TAG = "ST7789_DRIVER";

/**
 * @brief Internal driver state structure.
 */
struct st7789_dev {
    spi_device_handle_t spi_dev;
    int gpio_dc;
    int gpio_rst;
    int gpio_bckl;
    uint16_t *dma_buffer;
};

/**
 * @brief Per-transaction context used by the SPI pre-transfer callback.
 */
typedef struct {
    st7789_handle_t dev;
    bool is_data;
} st7789_tx_context_t;

/**
 * @brief Pre-transfer callback to handle D/C pin toggling in hardware ISR.
 */
static void IRAM_ATTR st7789_spi_pre_transfer_callback(spi_transaction_t *t) {
    const st7789_tx_context_t *ctx = (const st7789_tx_context_t *)t->user;

    if (ctx == NULL) {
        return;
    }

    gpio_set_level((gpio_num_t)ctx->dev->gpio_dc, ctx->is_data ? 1 : 0);
}

/**
 * @brief Sends a command byte to the ST7789 controller.
 */
esp_err_t st7789_send_cmd(st7789_handle_t dev, uint8_t cmd) {
    st7789_tx_context_t tx_ctx = {
        .dev = dev,
        .is_data = false,
    };

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &cmd,
        .user = &tx_ctx,
        .flags = 0,
    };
    return spi_device_polling_transmit(dev->spi_dev, &trans);
}

/**
 * @brief Sends data bytes to the ST7789 controller via polling.
 */
esp_err_t st7789_send_data(st7789_handle_t dev,
                                  const uint8_t *data,
                                  size_t len) {
    if (len == 0) {
        return ESP_OK;
    }

    st7789_tx_context_t tx_ctx = {
        .dev = dev,
        .is_data = true,
    };

    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
        .user = &tx_ctx,
        .flags = 0,
    };
    return spi_device_polling_transmit(dev->spi_dev, &trans);
}

/**
 * @brief Sets the memory window boundaries for incoming pixel data.
 */
esp_err_t st7789_set_window(st7789_handle_t dev,
                                   uint16_t x1,
                                   uint16_t y1,
                                   uint16_t x2,
                                   uint16_t y2) {
    esp_err_t ret = ESP_OK;
    uint8_t data[4];

    y1 += ST7789_ROW_OFFSET;
    y2 += ST7789_ROW_OFFSET;

    ret |= st7789_send_cmd(dev, 0x2A); /* Column Address Set */
    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1 & 0xFF);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2 & 0xFF);
    ret |= st7789_send_data(dev, data, 4);

    ret |= st7789_send_cmd(dev, 0x2B); /* Row Address Set */
    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1 & 0xFF);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2 & 0xFF);
    ret |= st7789_send_data(dev, data, 4);

    ret |= st7789_send_cmd(dev, 0x2C); /* RAM Write */
    return ret;
}

/**
 * @brief Performs hardware reset sequence on the display.
 */
static void st7789_hardware_reset(st7789_handle_t dev) {
    gpio_set_level((gpio_num_t)dev->gpio_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)dev->gpio_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

esp_err_t st7789_init(const st7789_config_t *config, st7789_handle_t *out_handle) {
    if ((config == NULL) || (out_handle == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    st7789_handle_t dev = calloc(1, sizeof(struct st7789_dev));
    if (dev == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for display device handle");
        return ESP_ERR_NO_MEM;
    }

    dev->gpio_dc = config->gpio_dc;
    dev->gpio_rst = config->gpio_rst;
    dev->gpio_bckl = config->gpio_bckl;

    /* Allocate DMA-capable internal buffer for high-speed line transfers */
    dev->dma_buffer = heap_caps_malloc(ST7789_DMA_BUF_SIZE,
                                       MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (dev->dma_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate DMA line buffer");
        free(dev);
        return ESP_ERR_NO_MEM;
    }

    /* Configure GPIO outputs */
    const uint64_t gpio_pin_mask = (1ULL << dev->gpio_dc) |
                                   (1ULL << dev->gpio_rst) |
                                   (1ULL << dev->gpio_bckl);
    gpio_config_t io_conf = {
        .pin_bit_mask = gpio_pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    /* Initialize SPI Bus */
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = config->gpio_mosi,
        .sclk_io_num = config->gpio_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ST7789_DMA_BUF_SIZE,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(config->spi_host, &buscfg, SPI_DMA_CH_AUTO));

    /* Attach device to SPI Bus */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = (int)config->clock_speed_hz,
        .mode = 0,
        .spics_io_num = config->gpio_cs,
        .queue_size = 7,
        .pre_cb = st7789_spi_pre_transfer_callback,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(config->spi_host, &devcfg, &dev->spi_dev));

    /* Reset and initialize controller commands */
    st7789_hardware_reset(dev);

    ESP_LOGI(TAG, "Initializing ST7789 display controller...");
    st7789_send_cmd(dev, 0x11); /* Sleep Out */
    vTaskDelay(pdMS_TO_TICKS(120));

    st7789_send_cmd(dev, 0x3A); /* Pixel Format: RGB565 (16-bit) */
    const uint8_t pixel_format = 0x55;
    st7789_send_data(dev, &pixel_format, 1);

    st7789_send_cmd(dev, 0x36); /* Memory Data Access Control */
    /* RGB color order, no rotation */
    const uint8_t madctl = 0x00;
    st7789_send_data(dev, &madctl, 1);

    st7789_send_cmd(dev, 0x21); /* Display Inversion On */
    st7789_send_cmd(dev, 0x29); /* Display On */
    vTaskDelay(pdMS_TO_TICKS(20));

    st7789_set_backlight(dev, true);

    *out_handle = dev;
    return ESP_OK;
}

esp_err_t st7789_deinit(st7789_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    st7789_set_backlight(handle, false);
    spi_bus_remove_device(handle->spi_dev);
    heap_caps_free(handle->dma_buffer);
    free(handle);
    return ESP_OK;
}

esp_err_t st7789_set_backlight(st7789_handle_t handle, bool is_enabled) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return gpio_set_level((gpio_num_t)handle->gpio_bckl, is_enabled ? 1 : 0);
}

esp_err_t st7789_clear(st7789_handle_t handle, uint16_t color) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const uint16_t byte_swapped_color = (color << 8) | (color >> 8);
    const size_t pixels_per_chunk = ST7789_DMA_BUF_SIZE / sizeof(uint16_t);

    for (size_t i = 0; i < pixels_per_chunk; i++) {
        handle->dma_buffer[i] = byte_swapped_color;
    }

    st7789_set_window(handle, 0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    const size_t total_pixels = LCD_WIDTH * LCD_HEIGHT;
    size_t pixels_remaining = total_pixels;

    while (pixels_remaining > 0) {
        const size_t current_chunk = (pixels_remaining < pixels_per_chunk)
                                        ? pixels_remaining
                                        : pixels_per_chunk;
        st7789_send_data(handle, (const uint8_t *)handle->dma_buffer, current_chunk * 2);
        pixels_remaining -= current_chunk;
    }

    return ESP_OK;
}

esp_err_t st7789_draw_image(st7789_handle_t handle,
                            uint16_t x_start,
                            uint16_t y_start,
                            const lcd_image_t *image) {
    if ((handle == NULL) || (image == NULL) || (image->pixel_data == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((x_start + image->width > LCD_WIDTH) ||
        (y_start + image->height > LCD_HEIGHT)) {
        ESP_LOGE(TAG, "Image dimensions exceed boundary limit");
        return ESP_ERR_INVALID_ARG;
    }

    st7789_set_window(handle,
                      x_start,
                      y_start,
                      x_start + image->width - 1,
                      y_start + image->height - 1);

    const size_t total_pixels = image->width * image->height;
    const size_t max_chunk_pixels = ST7789_DMA_BUF_SIZE / sizeof(uint16_t);
    size_t pixel_index = 0;

    while (pixel_index < total_pixels) {
        const size_t chunk_pixels = (total_pixels - pixel_index < max_chunk_pixels)
                                        ? (total_pixels - pixel_index)
                                        : max_chunk_pixels;

        /* Stream pixel chunks into DMA memory */
        for (size_t i = 0; i < chunk_pixels; i++) {
            handle->dma_buffer[i] = image->pixel_data[pixel_index + i];
        }

        st7789_send_data(handle, (const uint8_t *)handle->dma_buffer, chunk_pixels * 2);
        pixel_index += chunk_pixels;
    }

    return ESP_OK;
}

spi_device_handle_t st7789_get_spi_device(st7789_handle_t handle)
{
    if (handle == NULL) {
        return NULL;
    }
    return handle->spi_dev;
}

int st7789_get_gpio_dc(st7789_handle_t handle)
{
    if (handle == NULL) {
        return -1;
    }
    return handle->gpio_dc;
}
