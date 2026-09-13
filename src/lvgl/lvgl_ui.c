/**
 * @file lvgl_ui.c
 * @brief LVGL UI rendering implementation.
 */

#include "lvgl_ui.h"
#include "lvgl_display.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "LVGL_UI";

/**
 * @brief UI widget references.
 */
typedef struct {
    lv_obj_t *screen;
    lv_obj_t *power_label;
    lv_obj_t *brightness_label;
    lv_obj_t *brightness_bar;
} ui_widgets_t;

typedef enum {
    UI_UPDATE_BRIGHTNESS,
    UI_UPDATE_POWER
} ui_update_type_t;

typedef struct {
    ui_update_type_t type;
    union {
        int brightness;
        bool power_on;
    } data;
} ui_update_t;

static lv_theme_t *g_theme = NULL;
static lv_display_t *g_disp = NULL;
static ui_widgets_t g_widgets = {0};
static QueueHandle_t g_update_queue = NULL;

static void process_pending_updates(void)
{
    ui_update_t update;

    while (xQueueReceive(g_update_queue, &update, 0) == pdTRUE) {
        if (update.type == UI_UPDATE_BRIGHTNESS) {
            int value = update.data.brightness;
            if (value < 0) value = 0;
            if (value > 100) value = 100;

            if (g_widgets.brightness_label != NULL) {
                lv_label_set_text_fmt(g_widgets.brightness_label,
                                      "Brightness: %d%%", value);
                lv_obj_invalidate(g_widgets.brightness_label);
            }
            if (g_widgets.brightness_bar != NULL) {
                lv_bar_set_value(g_widgets.brightness_bar, value,
                                 LV_ANIM_OFF);
                lv_obj_invalidate(g_widgets.brightness_bar);
            }
        } else if (update.type == UI_UPDATE_POWER) {
            if (g_widgets.power_label != NULL) {
                if (update.data.power_on) {
                    lv_label_set_text(g_widgets.power_label, "Power: ON");
                    lv_obj_set_style_text_color(
                        g_widgets.power_label, lv_color_hex(0x4CAF50), 0);
                } else {
                    lv_label_set_text(g_widgets.power_label, "Power: OFF");
                    lv_obj_set_style_text_color(
                        g_widgets.power_label, lv_color_hex(0xFF4444), 0);
                }
                lv_obj_invalidate(g_widgets.power_label);
            }
        }
    }
}

/**
 * @brief LVGL timer handler task.
 */
static void lvgl_timer_task(void *arg)
{
    (void)arg;
    bool watchdog_registered = false;

    esp_err_t ret = esp_task_wdt_add(NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add timer task to watchdog: %s",
                 esp_err_to_name(ret));
    } else {
        watchdog_registered = true;
    }

    while (1) {
        process_pending_updates();
        lv_timer_handler();

        if (watchdog_registered) {
            esp_task_wdt_reset();
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

/**
 * @brief Create the main screen with all UI widgets.
 */
static esp_err_t create_main_screen(void)
{
    g_widgets.screen = lv_obj_create(NULL);
    if (g_widgets.screen == NULL) {
        ESP_LOGE(TAG, "Failed to create screen");
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_size(g_widgets.screen, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(g_widgets.screen, lv_color_hex(0x000000), 0);
    lv_obj_set_layout(g_widgets.screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_widgets.screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_widgets.screen,
                          LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(g_widgets.screen, 10, 0);
    lv_screen_load(g_widgets.screen);

    lv_obj_t *top_container = lv_obj_create(g_widgets.screen);
    lv_obj_set_size(top_container, LV_PCT(100), LV_PCT(30));
    lv_obj_set_layout(top_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_container,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(top_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(top_container, LV_OPA_80, 0);
    lv_obj_set_style_border_width(top_container, 0, 0);
    lv_obj_set_style_radius(top_container, 5, 0);
    lv_obj_set_style_pad_all(top_container, 10, 0);

    g_widgets.power_label = lv_label_create(top_container);
    lv_label_set_text(g_widgets.power_label, "Power: OFF");
    lv_obj_set_style_text_color(g_widgets.power_label, lv_color_hex(0xFFFFFF), 0);

    g_widgets.brightness_label = lv_label_create(top_container);
    lv_label_set_text(g_widgets.brightness_label, "Brightness: 0%");
    lv_obj_set_style_text_color(g_widgets.brightness_label, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *bar_container = lv_obj_create(g_widgets.screen);
    lv_obj_set_size(bar_container, LV_PCT(90), LV_PCT(20));
    lv_obj_set_layout(bar_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(bar_container,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(bar_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(bar_container, LV_OPA_80, 0);
    lv_obj_set_style_border_width(bar_container, 0, 0);
    lv_obj_set_style_radius(bar_container, 5, 0);
    lv_obj_set_style_pad_all(bar_container, 5, 0);

    lv_obj_t *bar_label = lv_label_create(bar_container);
    lv_label_set_text(bar_label, "LED Brightness");
    lv_obj_set_style_text_color(bar_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(bar_label, LV_TEXT_ALIGN_CENTER, 0);

    g_widgets.brightness_bar = lv_bar_create(bar_container);
    lv_obj_set_size(g_widgets.brightness_bar, LV_PCT(100), 20);
    lv_bar_set_range(g_widgets.brightness_bar, 0, 100);
    lv_bar_set_value(g_widgets.brightness_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(g_widgets.brightness_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(g_widgets.brightness_bar, LV_OPA_100, 0);
    lv_obj_set_style_radius(g_widgets.brightness_bar, 3, 0);
    lv_obj_set_style_bg_color(g_widgets.brightness_bar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_widgets.brightness_bar, LV_OPA_100, LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_widgets.brightness_bar, 3, LV_PART_INDICATOR);

    ESP_LOGI(TAG, "Main screen created");
    return ESP_OK;
}

/**
 * @brief Create and apply theme.
 */
static lv_theme_t *create_theme(void)
{
    return lv_theme_default_init(NULL,
                                  lv_palette_main(LV_PALETTE_BLUE),
                                  lv_palette_main(LV_PALETTE_RED),
                                  false,
                                  LV_FONT_DEFAULT);
}

esp_err_t lvgl_ui_init(lv_display_t *disp)
{
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    g_disp = disp;

    g_update_queue = xQueueCreate(10, sizeof(ui_update_t));
    if (g_update_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create UI update queue");
        return ESP_ERR_NO_MEM;
    }

    g_theme = create_theme();
    if (g_theme == NULL) {
        ESP_LOGE(TAG, "Failed to create theme");
        return ESP_ERR_NO_MEM;
    }
    lv_display_set_theme(disp, g_theme);

    esp_err_t ret = create_main_screen();
    if (ret != ESP_OK) {
        lv_theme_delete(g_theme);
        g_theme = NULL;
        return ret;
    }

    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(10));

    xTaskCreate(lvgl_timer_task, "lvgl_timer", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "LVGL UI initialized");
    return ESP_OK;
}

void lvgl_ui_update_brightness(int value)
{
    if (g_update_queue != NULL) {
        ui_update_t update = {
            .type = UI_UPDATE_BRIGHTNESS,
            .data.brightness = value
        };
        xQueueSend(g_update_queue, &update, 0);
    }
}

void lvgl_ui_update_power_status(bool is_on)
{
    if (g_update_queue != NULL) {
        ui_update_t update = {
            .type = UI_UPDATE_POWER,
            .data.power_on = is_on
        };
        xQueueSend(g_update_queue, &update, 0);
    }
}

void lvgl_ui_deinit(void)
{
    if (g_theme != NULL) {
        lv_theme_delete(g_theme);
        g_theme = NULL;
    }
    if (g_widgets.screen != NULL) {
        lv_obj_delete(g_widgets.screen);
        g_widgets.screen = NULL;
    }
    if (g_update_queue != NULL) {
        vQueueDelete(g_update_queue);
        g_update_queue = NULL;
    }
    ESP_LOGI(TAG, "LVGL UI deinitialized");
}
