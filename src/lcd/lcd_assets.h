/**
 * @file lcd_assets.h
 * @brief Data structures and declarations for flash-resident image assets.
 */

#ifndef LCD_ASSETS_H
#define LCD_ASSETS_H

#include <stdint.h>

/**
 * @brief Descriptor for a 16-bit RGB565 image asset.
 */
typedef struct {
    uint16_t width;             /**< Image width in pixels */
    uint16_t height;            /**< Image height in pixels */
    const uint16_t *pixel_data; /**< Pointer to flash-resident RGB565 data */
} lcd_image_t;

/**
 * @brief External declaration for the test image.
 */
extern const lcd_image_t g_lcd_img1;

/**
 * @brief External declaration for the test image.
 */
extern const lcd_image_t g_lcd_img2;

#endif /* LCD_ASSETS_H */