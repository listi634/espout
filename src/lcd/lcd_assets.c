/**
 * @file lcd_assets.c
 * @brief Flash-resident image asset definitions.
 */

#include "lcd_assets.h"
#include "assets/img1.h"
#include "assets/img2.h"

/**
 * @brief Test image descriptor for the LCD test pattern.
 * 
 * This image is 240x280 pixels in RGB565 format (16-bit color).
 * The data is stored as uint8_t array but accessed as uint16_t.
 */
const lcd_image_t g_lcd_img1 = {
    .width = 240,
    .height = 280,
    .pixel_data = (const uint16_t *)gImage_img1
};

/**
 * @brief Test image descriptor for the LCD test pattern.
 * 
 * This image is 240x280 pixels in RGB565 format (16-bit color).
 * The data is stored as uint8_t array but accessed as uint16_t.
 */
const lcd_image_t g_lcd_img2 = {
    .width = 240,
    .height = 280,
    .pixel_data = (const uint16_t *)gImage_img2
};