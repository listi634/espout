/**
 * @file main_screen.c
 * @brief Main screen implementation for the application.
 * 
 * This screen displays the selectable application functions.
 */

#include "main_screen.h"
#include "screen.h"
#include "screen_manager.h"
#include "../../../lcd/st7789.h"
#include "../../lvgl/lvgl_display.h"
#include "../../event_bus/event_bus.h"
#include "../../../buzzer/buzzer_notes.h"
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
    lv_obj_t *function_containers[APP_FUNCTION_COUNT];
    lv_obj_t *function_icons[APP_FUNCTION_COUNT];
    lv_obj_t *function_labels[APP_FUNCTION_COUNT];
} main_screen_widgets_t;

/**
 * @brief Update message structure.
 */
typedef struct {
    app_function_t selected_function;
} main_screen_update_t;

// Static variables
static lv_theme_t *s_theme = NULL;
static main_screen_widgets_t s_widgets = {0};
static QueueHandle_t s_update_queue = NULL;
static screen_t s_main_screen = {0};
static buzzer_handle_t s_buzzer = NULL;

// Forward declarations
static esp_err_t create_main_screen(lv_obj_t *parent);
static esp_err_t activate_main_screen(void);
static esp_err_t deactivate_main_screen(void);
static esp_err_t destroy_main_screen(void);
static void process_pending_updates(void);
static void render_selection(app_function_t selected_function);
static void handle_input(const app_input_event_t *event);
static void apply_function_step(int step);

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
        render_selection(update.selected_function);
    }
}

static void render_selection(app_function_t selected_function)
{
    static const char *icons[APP_FUNCTION_COUNT] = {
        LV_SYMBOL_SETTINGS,
        LV_SYMBOL_CHARGE
    };

    if (selected_function < 0 || selected_function >= APP_FUNCTION_COUNT) {
        return;
    }

    for (int index = 0; index < APP_FUNCTION_COUNT; index++) {
        bool is_selected = index == selected_function;
        int y_position = is_selected ? 105 :
                         (selected_function == APP_FUNCTION_SETTINGS ? 185 : 25);

        lv_obj_set_y(s_widgets.function_containers[index], y_position);
        lv_obj_set_style_opa(s_widgets.function_containers[index],
                             is_selected ? LV_OPA_COVER : LV_OPA_50, 0);
        lv_obj_set_style_text_color(s_widgets.function_icons[index],
                                    is_selected ? lv_color_hex(0xFFFFFF) :
                                    lv_color_hex(0x8A8F98), 0);
        lv_label_set_text(s_widgets.function_icons[index], icons[index]);
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
    for (int index = 0; index < APP_FUNCTION_COUNT; index++) {
        lv_obj_t *container = lv_obj_create(s_widgets.screen);
        s_widgets.function_containers[index] = container;
        lv_obj_set_size(container, 70, 70);
        lv_obj_set_x(container, LCD_WIDTH/2 - 35);
        lv_obj_set_style_bg_color(container, lv_color_hex(0x20242B), 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_style_radius(container, 12, 0);
        lv_obj_set_style_pad_all(container, 8, 0);

        s_widgets.function_icons[index] = lv_label_create(container);
        lv_obj_align(s_widgets.function_icons[index], LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_font(s_widgets.function_icons[index],
                                   &lv_font_montserrat_36, 0);
    }

    render_selection(APP_FUNCTION_SETTINGS);

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
    
    for (int index = 0; index < APP_FUNCTION_COUNT; index++) {
        s_widgets.function_containers[index] = NULL;
        s_widgets.function_icons[index] = NULL;
        s_widgets.function_labels[index] = NULL;
    }

    ESP_LOGI(TAG, "Main screen destroyed");
    return ESP_OK;
}

static void handle_input(const app_input_event_t *event)
{
    if (event == NULL) {
        return;
    }

    if (event->type == APP_INPUT_POT_STEP) {
        int direction = event->data.pot.delta > 0 ? 1 : -1;
        int count = event->data.pot.delta > 0 ? event->data.pot.delta :
                    -event->data.pot.delta;
        for (int index = 0; index < count; index++) {
            apply_function_step(direction);
        }
        return;
    }

    if (event->type == APP_INPUT_IR_KEY) {
        if (event->data.ir.key == IR_KEY_UP) {
            apply_function_step(-1);
        } else if (event->data.ir.key == IR_KEY_DOWN) {
            apply_function_step(1);
        }
    }
}

static void apply_function_step(int step)
{
    const app_state_t *state = app_state_get();
    int next_function = (int)state->selected_function + step;

    if (step < 0) {
        if (buzzer_beep(s_buzzer, NOTE_FS4, 80) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to play function selection feedback");
        }
    } else if (step > 0) {
        if (buzzer_beep(s_buzzer, NOTE_G4, 80) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to play function selection feedback");
        }
    }

    next_function %= APP_FUNCTION_COUNT;
    if (next_function < 0) {
        next_function += APP_FUNCTION_COUNT;
    }

    if ((app_function_t)next_function != state->selected_function &&
        app_state_set_selected_function((app_function_t)next_function)) {
        const app_event_t changed_event = {.type = EVENT_FUNCTION_CHANGED};
        event_bus_publish(&changed_event);
    }
}

esp_err_t main_screen_init(buzzer_handle_t buzzer)
{
    if (buzzer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_buzzer = buzzer;
    // Initialize the screen structure
    screen_init(&s_main_screen, SCREEN_MAIN, "Main Screen");
    s_main_screen.on_create = create_main_screen;
    s_main_screen.on_activate = activate_main_screen;
    s_main_screen.on_deactivate = deactivate_main_screen;
    s_main_screen.on_destroy = destroy_main_screen;
    s_main_screen.on_event = NULL;
    s_main_screen.on_input = handle_input;
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

void main_screen_update_selection(app_function_t function)
{
    if (s_update_queue != NULL) {
        main_screen_update_t update = {.selected_function = function};
        xQueueSend(s_update_queue, &update, 0);
    }
}

lv_obj_t *main_screen_get_view(void)
{
    return s_widgets.screen;
}
