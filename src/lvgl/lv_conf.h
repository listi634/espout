/**
 * @file lv_conf.h
 * @brief LVGL configuration for ESP32-C6 with ST7789V2 display.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include "sdkconfig.h"

/* Display dimensions */
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 280

/* Color depth - RGB565 for ST7789 display */
#define LV_COLOR_DEPTH 16

/* Use 16-bit color internally with byte swap for little-endian systems */
#define LV_COLOR_16_SWAP 1

/* Memory configuration */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (32U * 1024U)  /* 32KB */

/* Enable required features */
#define LV_USE_LOG 0
#define LV_USE_ASSERT 0
#define LV_USE_FLEX 1
#define LV_USE_GRID 0

/* Widgets */
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_BUTTON 0
#define LV_USE_OBJ 1

/* Fonts - Montserrat */
#define LV_FONT_MONTSERRAT_8 1
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0

/* Theme */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_INIT 1

/* Disable unused features to save memory */
#define LV_USE_ANIMATION 1
#define LV_USE_ARC 0
#define LV_USE_BTNMATRIX 0
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 0
#define LV_USE_CIRCULAR_CHART 0
#define LV_USE_CIRCULAR_GAUGE 0
#define LV_USE_DROPDOWN 0
#define LV_USE_GAUGE 0
#define LV_USE_IMG 0
#define LV_USE_IMGBTN 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LINE 0
#define LV_USE_LIST 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 0
#define LV_USE_SLIDER 0
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SWITCH 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TEXTAREA 0
#define LV_USE_TILEVIEW 0

/* Enable draw buffers */
#define LV_USE_DRAW_BUF 1

#endif /* LV_CONF_H */
