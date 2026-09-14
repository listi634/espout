/**
 * @file main_screen.c
 * @brief Main screen implementation for the application.
 * 
 * This screen displays the primary UI with power status, brightness controls.
 */

#include "main_screen.h"
#include "screen.h"
#include "screen_manager.h"
#include "../../lvgl/lvgl_display.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "MAIN_SCREEN";

/**
 * @brief UI widget references for the main screen.
 */
typedef struct {
    lv_obj_t *screen;
    lv_obj_t *power_label;
    lv_obj_t *brightness_label;
    lv_obj_t *brightness_bar;
} main_screen_widgets_t;

/**
 * @brief Update message types for the queue.
 */
typedef enum {
    MAIN_SCREEN_UPDATE_BRIGHTNESS,
    MAIN_SCREEN_UPDATE_POWER
} main_screen_update_type_t;

/**
 * @brief Update message structure.
 */
typedef struct {
    main_screen_update_type_t type;
    union {
        int brightness;
        bool power_on;
    } data;
} main_screen_update_t;

// Static variables
static lv_theme_t *s_theme = NULL;
static main_screen_widgets_t s_widgets = {0};
static QueueHandle_t s_update_queue = NULL;
static screen_t s_main_screen = {0};

// Forward declarations
static esp_err_t create_main_screen(lv_obj_t *parent);
static esp_err_t activate_main_screen(void);
static esp_err_t deactivate_main_screen(void);
static esp_err_t destroy_main_screen(void);
static void process_pending_updates(void);

/**
 * @brief LVGL timer handler task.
 * 
 * This task handles LVGL timer updates and processes pending UI updates.
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
 * @brief Process pending UI updates from the queue.
 */
static void process_pending_updates(void)
{
    main_screen_update_t update;

    while (xQueueReceive(s_update_queue, &update, 0) == pdTRUE) {
        if (update.type == MAIN_SCREEN_UPDATE_BRIGHTNESS) {
            int value = update.data.brightness;
            if (value < 0) value = 0;
            if (value > 100) value = 100;

            if (s_widgets.brightness_label != NULL) {
                lv_label_set_text_fmt(s_widgets.brightness_label,
                                      "Brightness: %d%%", value);
                lv_obj_invalidate(s_widgets.brightness_label);
            }
            if (s_widgets.brightness_bar != NULL) {
                lv_bar_set_value(s_widgets.brightness_bar, value,
                                 LV_ANIM_OFF);
                lv_obj_invalidate(s_widgets.brightness_bar);
            }
        } else if (update.type == MAIN_SCREEN_UPDATE_POWER) {
            if (s_widgets.power_label != NULL) {
                if (update.data.power_on) {
                    lv_label_set_text(s_widgets.power_label, "Power: ON");
                    lv_obj_set_style_text_color(
                        s_widgets.power_label, lv_color_hex(0x4CAF50), 0);
                } else {
                    lv_label_set_text(s_widgets.power_label, "Power: OFF");
                    lv_obj_set_style_text_color(
                        s_widgets.power_label, lv_color_hex(0xFF4444), 0);
                }
                lv_obj_invalidate(s_widgets.power_label);
            }
        }
    }
}

/**
 * @brief Create the theme for the application.
 * 
 * @return lv_theme_t* Pointer to the created theme, or NULL on failure.
 */
static lv_theme_t *create_theme(void)
{
    return lv_theme_default_init(NULL,
                                  lv_palette_main(LV_PALETTE_BLUE),
                                  lv_palette_main(LV_PALETTE_RED),
                                  false,
                                  LV_FONT_DEFAULT);
}

/**
 * @brief Create the main screen with all UI widgets.
 * 
 * @param parent The parent LVGL object (unused, screen is top-level).
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
static esp_err_t create_main_screen(lv_obj_t *parent)
{
    (void)parent;  // Screen is created at top level

    s_widgets.screen = lv_obj_create(NULL);
    if (s_widgets.screen == NULL) {
        ESP_LOGE(TAG, "Failed to create screen");
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_size(s_widgets.screen, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(s_widgets.screen, lv_color_hex(0x000000), 0);
    lv_obj_set_layout(s_widgets.screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_widgets.screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_widgets.screen,
                          LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(s_widgets.screen, 10, 0);

    lv_obj_t *top_container = lv_obj_create(s_widgets.screen);
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

    s_widgets.power_label = lv_label_create(top_container);
    lv_label_set_text(s_widgets.power_label, "Power: OFF");
    lv_obj_set_style_text_color(s_widgets.power_label, lv_color_hex(0xFFFFFF), 0);

    s_widgets.brightness_label = lv_label_create(top_container);
    lv_label_set_text(s_widgets.brightness_label, "Brightness: 0%");
    lv_obj_set_style_text_color(s_widgets.brightness_label, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t *bar_container = lv_obj_create(s_widgets.screen);
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

    s_widgets.brightness_bar = lv_bar_create(bar_container);
    lv_obj_set_size(s_widgets.brightness_bar, LV_PCT(100), 20);
    lv_bar_set_range(s_widgets.brightness_bar, 0, 100);
    lv_bar_set_value(s_widgets.brightness_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_widgets.brightness_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(s_widgets.brightness_bar, LV_OPA_100, 0);
    lv_obj_set_style_radius(s_widgets.brightness_bar, 3, 0);
    lv_obj_set_style_bg_color(s_widgets.brightness_bar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s_widgets.brightness_bar, LV_OPA_100, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_widgets.brightness_bar, 3, LV_PART_INDICATOR);

    // Store the screen reference
    s_main_screen.view = s_widgets.screen;

    ESP_LOGI(TAG, "Main screen created");
    return ESP_OK;
}

/**
 * @brief Activate the main screen (load it into the display).
 * 
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
static esp_err_t activate_main_screen(void)
{
    if (s_widgets.screen != NULL) {
        lv_screen_load(s_widgets.screen);
    }
    ESP_LOGI(TAG, "Main screen activated");
    return ESP_OK;
}

/**
 * @brief Deactivate the main screen (hide it).
 * 
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
static esp_err_t deactivate_main_screen(void)
{
    if (s_widgets.screen != NULL) {
        lv_obj_set_hidden(s_widgets.screen, true);
    }
    ESP_LOGI(TAG, "Main screen deactivated");
    return ESP_OK;
}

/**
 * @brief Destroy the main screen and free resources.
 * 
 * @return esp_err_t ESP_OK on success, error code on failure.
 */
static esp_err_t destroy_main_screen(void)
{
    if (s_theme != NULL) {
        lv_theme_delete(s_theme);
        s_theme = NULL;
    }
    
    if (s_widgets.screen != NULL) {
        lv_obj_delete(s_widgets.screen);
        s_widgets.screen = NULL;
    }
    
    s_widgets.power_label = NULL;
    s_widgets.brightness_label = NULL;
    s_widgets.brightness_bar = NULL;

    ESP_LOGI(TAG, "Main screen destroyed");
    return ESP_OK;
}

esp_err_t main_screen_init(void)
{
    // Initialize the screen structure
    screen_init(&s_main_screen, SCREEN_MAIN, "Main Screen");
    s_main_screen.on_create = create_main_screen;
    s_main_screen.on_activate = activate_main_screen;
    s_main_screen.on_deactivate = deactivate_main_screen;
    s_main_screen.on_destroy = destroy_main_screen;
    s_main_screen.on_event = NULL;
    s_main_screen.on_update = NULL;

    // Create update queue
    s_update_queue = xQueueCreate(10, sizeof(main_screen_update_t));
    if (s_update_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create update queue");
        return ESP_ERR_NO_MEM;
    }

    // Create theme
    s_theme = create_theme();
    if (s_theme == NULL) {
        ESP_LOGE(TAG, "Failed to create theme");
        vQueueDelete(s_update_queue);
        s_update_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    // Register with screen manager
    esp_err_t ret = screen_manager_register(&s_main_screen);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register main screen: %s", esp_err_to_name(ret));
        lv_theme_delete(s_theme);
        s_theme = NULL;
        vQueueDelete(s_update_queue);
        s_update_queue = NULL;
        return ret;
    }

    // Apply theme to display
    lv_display_t *disp = lvgl_display_get_instance();
    if (disp != NULL) {
        lv_display_set_theme(disp, s_theme);
    }

    // Start LVGL timer task
    xTaskCreate(lvgl_timer_task, "lvgl_timer", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "Main screen initialized");
    return ESP_OK;
}

void main_screen_update_brightness(int value)
{
    if (s_update_queue != NULL) {
        main_screen_update_t update = {
            .type = MAIN_SCREEN_UPDATE_BRIGHTNESS,
            .data.brightness = value
        };
        xQueueSend(s_update_queue, &update, 0);
    }
}

void main_screen_update_power_status(bool is_on)
{
    if (s_update_queue != NULL) {
        main_screen_update_t update = {
            .type = MAIN_SCREEN_UPDATE_POWER,
            .data.power_on = is_on
        };
        xQueueSend(s_update_queue, &update, 0);
    }
}

lv_obj_t *main_screen_get_view(void)
{
    return s_widgets.screen;
}
