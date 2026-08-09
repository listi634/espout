/**
 * @file lvgl_ui.c
 * @brief LVGL UI initialization and management implementation.
 *
 * This module creates and manages the main UI for the ESP32-C6 device.
 * It provides widgets for displaying power status, brightness, and handling
 * IR remote control events.
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
 * @brief UI update message types.
 */
typedef enum {
    UI_MSG_POWER_ON,
    UI_MSG_POWER_OFF,
    UI_MSG_BRIGHTNESS_UP,
    UI_MSG_BRIGHTNESS_DOWN,
    UI_MSG_STATUS_TEXT,
} ui_msg_type_t;

/**
 * @brief UI update message structure.
 */
typedef struct {
    ui_msg_type_t type;
    int value;             /**< For brightness value or other numeric data */
    const char *text;     /**< For status text messages */
} ui_msg_t;

/**
 * @brief Queue for UI update messages.
 */
static QueueHandle_t s_ui_msg_queue = NULL;

/**
 * @brief UI elements structure.
 */
typedef struct {
    lv_obj_t *screen;             /**< Main screen object */
    lv_obj_t *power_label;        /**< Label for power status */
    lv_obj_t *brightness_label;   /**< Label for brightness value */
    lv_obj_t *brightness_bar;     /**< Bar for visual brightness feedback */
    lv_obj_t *status_label;       /**< Label for general status messages */
    bool power_on;                /**< Current power state */
    int brightness;               /**< Current brightness value (0-100) */
} ui_state_t;

/**
 * @brief Global UI state.
 */
static ui_state_t g_ui_state = {0};

/**
 * @brief Theme for the UI.
 */
static lv_theme_t *g_theme = NULL;

/**
 * @brief LVGL display instance.
 */
static lv_display_t *g_disp = NULL;

/**
 * @brief Processes a UI update message.
 *
 * This function runs in the context of the LVGL timer task, so it's safe
 * to call LVGL functions here.
 */
static void process_ui_message(const ui_msg_t *msg)
{
    if (msg == NULL) {
        return;
    }

    ESP_LOGD(TAG, "Processing UI message type: %d", msg->type);
    
    switch (msg->type) {
        case UI_MSG_POWER_ON:
            g_ui_state.power_on = true;
            lvgl_ui_update_power_status(true);
            lvgl_ui_update_brightness(g_ui_state.brightness);
            lv_label_set_text(g_ui_state.status_label, "Power ON");
            break;

        case UI_MSG_POWER_OFF:
            g_ui_state.power_on = false;
            lvgl_ui_update_power_status(false);
            lv_label_set_text(g_ui_state.status_label, "Power OFF");
            break;

        case UI_MSG_BRIGHTNESS_UP:
            if (g_ui_state.brightness < 100) {
                g_ui_state.brightness += 10;
                if (g_ui_state.brightness > 100) {
                    g_ui_state.brightness = 100;
                }
                lvgl_ui_update_brightness(g_ui_state.brightness);
                lv_label_set_text(g_ui_state.status_label, "Brightness +");
            }
            break;

        case UI_MSG_BRIGHTNESS_DOWN:
            if (g_ui_state.brightness > 0) {
                g_ui_state.brightness -= 10;
                if (g_ui_state.brightness < 0) {
                    g_ui_state.brightness = 0;
                }
                lvgl_ui_update_brightness(g_ui_state.brightness);
                lv_label_set_text(g_ui_state.status_label, "Brightness -");
            }
            break;

        case UI_MSG_STATUS_TEXT:
            if (msg->value >= 0) {
                lv_label_set_text_fmt(g_ui_state.status_label, "Key: %d", msg->value);
            }
            break;

        default:
            break;
    }
}

/**
 * @brief LVGL timer handler task.
 * 
 * This task continuously calls lv_timer_handler() to process LVGL timing events
 * and processes UI update messages from the IR handler task.
 * It resets the task watchdog to prevent the IDLE task from being flagged as stuck.
 */
static void lvgl_timer_task(void *arg)
{
    (void)arg;
    bool watchdog_registered = false;

    esp_err_t ret = esp_task_wdt_add(NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add LVGL timer task to watchdog: %s",
                 esp_err_to_name(ret));
    } else {
        watchdog_registered = true;
    }

    while (1) {
        /* Process any pending UI update messages first */
        ui_msg_t msg;
        bool ui_updated = false;
        while (xQueueReceive(s_ui_msg_queue, &msg, 0) == pdTRUE) {
            process_ui_message(&msg);
            ui_updated = true;
        }
        
        uint32_t delay_ms;
        /* If UI was updated, force immediate display update */
        if (ui_updated) {
            /* Explicitly invalidate all UI elements */
            if (g_ui_state.power_label != NULL) lv_obj_invalidate(g_ui_state.power_label);
            if (g_ui_state.brightness_label != NULL) lv_obj_invalidate(g_ui_state.brightness_label);
            if (g_ui_state.brightness_bar != NULL) lv_obj_invalidate(g_ui_state.brightness_bar);
            if (g_ui_state.status_label != NULL) lv_obj_invalidate(g_ui_state.status_label);
            if (g_ui_state.screen != NULL) lv_obj_invalidate(g_ui_state.screen);
            
            /* Call timer handler directly to process invalidations immediately */
            lv_timer_handler();
            delay_ms = 1;  /* Small delay to prevent busy loop */
        } else {
            /* No UI update - use period-based timer handler */
            delay_ms = lv_timer_handler_run_in_period(10);
            /* Ensure we never sleep for 0ms or too long (cap at 100ms to prevent watchdog) */
            if (delay_ms == 0 || delay_ms > 100) {
                delay_ms = 1;
            }
        }
        
        if (watchdog_registered) {
            esp_task_wdt_reset();
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/**
 * @brief Creates the main screen with all UI elements.
 *
 * @return esp_err_t ESP_OK on success.
 */
static esp_err_t create_main_screen(void)
{
    /* Use the default font */
    lv_font_t *font_large = LV_FONT_DEFAULT;
    lv_font_t *font_medium = LV_FONT_DEFAULT;

    /* Create a new screen */
    g_ui_state.screen = lv_obj_create(NULL);
    if (g_ui_state.screen == NULL) {
        ESP_LOGE(TAG, "Failed to create screen");
        return ESP_ERR_NO_MEM;
    }
    lv_obj_set_size(g_ui_state.screen, LCD_WIDTH, LCD_HEIGHT);
    
    /* Set black background for better contrast */
    lv_obj_set_style_bg_color(g_ui_state.screen, lv_color_hex(0x000000), 0);
    lv_obj_set_layout(g_ui_state.screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_ui_state.screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_ui_state.screen, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(g_ui_state.screen, 10, 0);
    lv_screen_load(g_ui_state.screen);

    /* Create a container for the top section */
    lv_obj_t *top_container = lv_obj_create(g_ui_state.screen);
    lv_obj_set_size(top_container, LV_PCT(100), LV_PCT(30));
    lv_obj_set_layout(top_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(top_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(top_container, LV_OPA_80, 0);
    lv_obj_set_style_border_width(top_container, 0, 0);
    lv_obj_set_style_radius(top_container, 5, 0);
    lv_obj_set_style_pad_all(top_container, 10, 0);

    /* Create power status label with white text */
    g_ui_state.power_label = lv_label_create(top_container);
    lv_label_set_text(g_ui_state.power_label, "Power: OFF");
    lv_obj_set_style_text_color(g_ui_state.power_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_ui_state.power_label, font_large, 0);

    /* Create brightness label with white text */
    g_ui_state.brightness_label = lv_label_create(top_container);
    lv_label_set_text(g_ui_state.brightness_label, "Brightness: 0%");
    lv_obj_set_style_text_color(g_ui_state.brightness_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_ui_state.brightness_label, font_large, 0);

    /* Create a container for the brightness bar */
    lv_obj_t *bar_container = lv_obj_create(g_ui_state.screen);
    lv_obj_set_size(bar_container, LV_PCT(90), LV_PCT(20));
    lv_obj_set_layout(bar_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(bar_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(bar_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(bar_container, LV_OPA_80, 0);
    lv_obj_set_style_border_width(bar_container, 0, 0);
    lv_obj_set_style_radius(bar_container, 5, 0);
    lv_obj_set_style_pad_all(bar_container, 5, 0);

    /* Create a label for the bar with white text */
    lv_obj_t *bar_label = lv_label_create(bar_container);
    lv_label_set_text(bar_label, "LED Brightness");
    lv_obj_set_style_text_color(bar_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(bar_label, font_medium, 0);
    lv_obj_set_style_text_align(bar_label, LV_TEXT_ALIGN_CENTER, 0);

    /* Create brightness bar with green indicator */
    g_ui_state.brightness_bar = lv_bar_create(bar_container);
    lv_obj_set_size(g_ui_state.brightness_bar, LV_PCT(100), 20);
    lv_bar_set_range(g_ui_state.brightness_bar, 0, 100);
    lv_bar_set_value(g_ui_state.brightness_bar, 0, LV_ANIM_OFF);

    /* Style the bar - dark background */
    lv_obj_set_style_bg_color(g_ui_state.brightness_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(g_ui_state.brightness_bar, LV_OPA_100, 0);
    lv_obj_set_style_radius(g_ui_state.brightness_bar, 3, 0);

    /* Style the bar indicator - green */
    lv_obj_set_style_bg_color(g_ui_state.brightness_bar, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_ui_state.brightness_bar, LV_OPA_100, LV_PART_INDICATOR);
    lv_obj_set_style_radius(g_ui_state.brightness_bar, 3, LV_PART_INDICATOR);

    /* Create status label at the bottom with white text */
    lv_obj_t *bottom_container = lv_obj_create(g_ui_state.screen);
    lv_obj_set_size(bottom_container, LV_PCT(100), LV_PCT(20));
    lv_obj_set_layout(bottom_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(bottom_container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(bottom_container, LV_OPA_80, 0);
    lv_obj_set_style_border_width(bottom_container, 0, 0);
    lv_obj_set_style_radius(bottom_container, 5, 0);

    g_ui_state.status_label = lv_label_create(bottom_container);
    lv_label_set_text(g_ui_state.status_label, "Ready");
    lv_obj_set_style_text_color(g_ui_state.status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_ui_state.status_label, font_medium, 0);

    /* Initialize UI state */
    g_ui_state.power_on = false;
    g_ui_state.brightness = 0;

    ESP_LOGI(TAG, "Main screen created successfully");
    return ESP_OK;
}

/**
 * @brief Creates a simple theme for the UI.
 *
 * @return lv_theme_t* The created theme.
 */
static lv_theme_t *create_theme(void)
{
    lv_theme_t *theme = lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED),
                                              false, LV_FONT_DEFAULT);
    return theme;
}

esp_err_t lvgl_ui_init(lv_display_t *disp)
{
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    /* LVGL should already be initialized in main.c */
    /* Buffers are already set up in lvgl_display_init */
    /* Tick callback is registered in lvgl_display_init */

    /* Store display pointer for later use */
    g_disp = disp;

    /* Create and apply theme */
    g_theme = create_theme();
    if (g_theme == NULL) {
        ESP_LOGE(TAG, "Failed to create theme");
        return ESP_ERR_NO_MEM;
    }
    lv_display_set_theme(disp, g_theme);

    /* Initialize UI state */
    g_ui_state.power_on = false;
    g_ui_state.brightness = 0;

    /* Create the main screen */
    esp_err_t ret = create_main_screen();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create main screen: %s", esp_err_to_name(ret));
        lv_theme_delete(g_theme);
        g_theme = NULL;
        return ret;
    }

    /* Force immediate render of the initial screen */
    lv_timer_handler();
    
    /* Create message queue for UI updates from IR task */
    s_ui_msg_queue = xQueueCreate(10, sizeof(ui_msg_t));
    if (s_ui_msg_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create UI message queue");
        lv_theme_delete(g_theme);
        g_theme = NULL;
        return ESP_ERR_NO_MEM;
    }

    
    /* Small delay to allow flush to complete before creating timer task */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Create LVGL timer task */
    xTaskCreate(lvgl_timer_task, "lvgl_timer", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "LVGL UI initialized successfully");
    return ESP_OK;
}

void lvgl_ui_handle_ir_event(ir_key_t key, const ir_lookup_entry_t *entry)
{
    (void)entry; /* Unused for now */
    
    if (s_ui_msg_queue == NULL) {
        ESP_LOGW(TAG, "UI message queue not initialized, dropping event");
        return;
    }

    ui_msg_t msg = {0};
    bool send_msg = false;

    switch (key) {
        case IR_KEY_ON:
            msg.type = UI_MSG_POWER_ON;
            send_msg = true;
            ESP_LOGI(TAG, "IR: Power ON");
            break;

        case IR_KEY_OFF:
            msg.type = UI_MSG_POWER_OFF;
            send_msg = true;
            ESP_LOGI(TAG, "IR: Power OFF");
            break;

        case IR_KEY_ZOOM_UP:
            msg.type = UI_MSG_BRIGHTNESS_UP;
            send_msg = true;
            break;

        case IR_KEY_ZOOM_DOWN:
            msg.type = UI_MSG_BRIGHTNESS_DOWN;
            send_msg = true;
            break;

        case IR_KEY_MENU:
        case IR_KEY_OK:
        case IR_KEY_UP:
        case IR_KEY_DOWN:
        case IR_KEY_LEFT:
        case IR_KEY_RIGHT:
        case IR_KEY_BACK:
        case IR_KEY_MUTE:
        case IR_KEY_VOLUME_UP:
        case IR_KEY_VOLUME_DOWN:
        default:
            msg.type = UI_MSG_STATUS_TEXT;
            msg.value = key;
            send_msg = true;
            break;
    }
    
    if (send_msg) {
        /* Send message to timer task - don't block if queue is full */
        if (xQueueSend(s_ui_msg_queue, &msg, pdMS_TO_TICKS(10)) != pdPASS) {
            ESP_LOGW(TAG, "Failed to send UI message to queue");
        }
    }
}

void lvgl_ui_update_brightness(int value)
{
    if (value < 0) {
        value = 0;
    } else if (value > 100) {
        value = 100;
    }

    g_ui_state.brightness = value;
    ESP_LOGD(TAG, "Updating brightness to %d%%", value);

    if (g_ui_state.brightness_label != NULL) {
        lv_label_set_text_fmt(g_ui_state.brightness_label, "Brightness: %d%%", value);
    }

    if (g_ui_state.brightness_bar != NULL) {
        lv_bar_set_value(g_ui_state.brightness_bar, value, LV_ANIM_OFF);
    }
}

void lvgl_ui_update_power_status(bool is_on)
{
    g_ui_state.power_on = is_on;
    ESP_LOGD(TAG, "Updating power status to %s", is_on ? "ON" : "OFF");

    if (g_ui_state.power_label != NULL) {
        if (is_on) {
            lv_label_set_text(g_ui_state.power_label, "Power: ON");
            lv_obj_set_style_text_color(g_ui_state.power_label, lv_color_hex(0x4CAF50), 0);
        } else {
            lv_label_set_text(g_ui_state.power_label, "Power: OFF");
            lv_obj_set_style_text_color(g_ui_state.power_label, lv_color_hex(0xFF4444), 0);
        }
    }
}

void lvgl_ui_deinit(void)
{
    /* Buffers are managed by lvgl_display, not here */
    
    /* Delete theme */
    if (g_theme != NULL) {
        lv_theme_delete(g_theme);
        g_theme = NULL;
    }
    
    /* Delete screen */
    if (g_ui_state.screen != NULL) {
        lv_obj_delete(g_ui_state.screen);
        g_ui_state.screen = NULL;
    }
    
    /* Delete message queue */
    if (s_ui_msg_queue != NULL) {
        vQueueDelete(s_ui_msg_queue);
        s_ui_msg_queue = NULL;
    }
    
    ESP_LOGI(TAG, "LVGL UI deinitialized");
}

/**
 * @brief Tick handler for LVGL.
 * 
 * Note: This function is not used when lv_tick_set_cb() is configured.
 * It's kept for compatibility in case it's called from external code.
 * When using manual tick increments, call this with the elapsed time in ms.
 */
void lvgl_tick_handler(uint32_t tick_period_ms)
{
    lv_tick_inc(tick_period_ms);
}
