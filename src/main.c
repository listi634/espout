/**
 * @file main.c
 * @brief Master application orchestrating IR remote, Potentiometer, ST7789 LCD, and Buzzer.
 */

#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"

#include "buzzer/buzzer.h"
#include "buzzer/assets/song_got.h"
#include "pot/pot_adc.h"
#include "ir/ir_handler.h"
#include "ir/ir_config.h"
#include "lcd/st7789.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "utils/app_config.h"

static const char *TAG = "MAIN_APP";

/* --- Display Color Constants (RGB565) --- */
#define COLOR_BLUE               0x6c33
#define COLOR_BLACK              0x0000

/* --- Potentiometer to Tempo Mapping Constants --- */
#define MIN_TEMPO_BPM            40
#define MAX_TEMPO_BPM            200

/**
 * @brief Events dispatched to the UI rendering queue.
 */
typedef enum {
    UI_EVT_UPDATE = 0
} ui_event_type_t;

typedef struct {
    ui_event_type_t type;
} ui_event_t;

/**
 * @brief Master application context passed across tasks and callbacks.
 */
typedef struct {
    buzzer_handle_t buzzer;
    pot_device_handle_t pot;
    st7789_handle_t lcd;
    QueueHandle_t ui_queue;
    uint16_t last_tempo_bpm;
    int last_pot_pct;
    buzzer_player_state_t last_player_state;
} app_context_t;

static app_context_t s_app_ctx;

/**
 * @brief IR key event callback executed upon receiving decoded infrared commands.
 */
static void on_ir_key_event(ir_key_t key, const ir_lookup_entry_t *entry)
{
    app_context_t *ctx = &s_app_ctx;
    if (ctx == NULL) {
        return;
    }

    if (key == IR_KEY_ON) {
        const buzzer_player_state_t state = buzzer_player_get_state(ctx->buzzer);
        if (state == BUZZER_PLAYER_STATE_IDLE) {
            ESP_LOGI(TAG, "IR Command [ON]: Starting GoT theme (looping enabled)");
            (void)buzzer_player_start(ctx->buzzer, &song_got, true);
        } else if (state == BUZZER_PLAYER_STATE_PAUSED) {
            ESP_LOGI(TAG, "IR Command [ON]: Resuming GoT theme playback");
            (void)buzzer_player_resume(ctx->buzzer);
        } else {
            ESP_LOGD(TAG, "IR Command [ON]: Song already actively playing");
        }

        const ui_event_t evt = { .type = UI_EVT_UPDATE };
        (void)xQueueSend(ctx->ui_queue, &evt, 0);
    } else if (key == IR_KEY_OFF) {
        ESP_LOGI(TAG, "IR Command [OFF]: Pausing GoT theme");
        (void)buzzer_player_pause(ctx->buzzer);

        const ui_event_t evt = { .type = UI_EVT_UPDATE };
        (void)xQueueSend(ctx->ui_queue, &evt, 0);
    }
}

/**
 * @brief Renders the playback state and potentiometer tempo onto the ST7789 LCD.
 */
static void render_ui_screen(st7789_handle_t lcd, buzzer_player_state_t state,
                             uint16_t tempo_bpm, int pot_pct)
{
    char state_buffer[32];
    char tempo_buffer[32];
    char pot_buffer[32];

    if (state == BUZZER_PLAYER_STATE_PLAYING) {
        (void)snprintf(state_buffer, sizeof(state_buffer), "STATE:  PLAYING");
        (void)st7789_clear(lcd, COLOR_BLUE);
    } else if (state == BUZZER_PLAYER_STATE_PAUSED) {
        (void)snprintf(state_buffer, sizeof(state_buffer), "STATE:  PAUSED ");
        (void)st7789_clear(lcd, COLOR_BLACK);
    } else {
        (void)snprintf(state_buffer, sizeof(state_buffer), "STATE:  STOPPED");
    }

    (void)snprintf(tempo_buffer, sizeof(tempo_buffer), "SPEED:  %3u BPM", tempo_bpm);
    (void)snprintf(pot_buffer, sizeof(pot_buffer),     "POTI:   %3d %%",  pot_pct);

    /* TODO: Implement text rendering - st7789_draw_string not available */
    /* For now, just log the UI update */
    //ESP_LOGI(TAG, "UI Update: %s | %s | %s", state_buffer, tempo_buffer, pot_buffer);
}

/**
 * @brief Dedicated FreeRTOS task handling UI screen refreshes.
 */
static void ui_task(void *pvParameters)
{
    app_context_t *ctx = (app_context_t *)pvParameters;
    ui_event_t evt;

    /* Fill background solid white once at boot */
    (void)st7789_clear(ctx->lcd, COLOR_BLUE);
    render_ui_screen(ctx->lcd, ctx->last_player_state, ctx->last_tempo_bpm, ctx->last_pot_pct);

    while (1) {
        if (xQueueReceive(ctx->ui_queue, &evt, portMAX_DELAY) == pdTRUE) {
            const buzzer_player_state_t current_state = buzzer_player_get_state(ctx->buzzer);
            const uint16_t current_tempo = buzzer_player_get_tempo(ctx->buzzer);

            render_ui_screen(ctx->lcd, current_state, current_tempo, ctx->last_pot_pct);

            ctx->last_player_state = current_state;
            ctx->last_tempo_bpm = current_tempo;
        }
    }
}

/**
 * @brief Background task monitoring potentiometer changes and updating song speed.
 */
static void pot_monitor_task(void *pvParameters)
{
    app_context_t *ctx = (app_context_t *)pvParameters;
    int scaled_pot_pct = 0;

    while (1) {
        if (pot_adc_read_scaled(ctx->pot, 100, &scaled_pot_pct) == ESP_OK) {
            /* Apply 2% noise hysteresis filter */
            if (abs(scaled_pot_pct - ctx->last_pot_pct) >= 2) {
                ctx->last_pot_pct = scaled_pot_pct;

                /* Map 0..100% linearly to MIN_TEMPO_BPM..MAX_TEMPO_BPM */
                const uint16_t new_tempo = (uint16_t)(MIN_TEMPO_BPM +
                    (scaled_pot_pct * (MAX_TEMPO_BPM - MIN_TEMPO_BPM)) / 100);

                (void)buzzer_player_set_tempo(ctx->buzzer, new_tempo);

                const ui_event_t evt = { .type = UI_EVT_UPDATE };
                (void)xQueueSend(ctx->ui_queue, &evt, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(app_config_init());

    ESP_LOGI(TAG, "Booting Interactive Buzzer Music Center...");

    /* 1. Initialize UI message queue */
    s_app_ctx.ui_queue = xQueueCreate(10, sizeof(ui_event_t));
    if (s_app_ctx.ui_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create UI queue");
        return;
    }

    s_app_ctx.last_player_state = BUZZER_PLAYER_STATE_IDLE;
    s_app_ctx.last_tempo_bpm    = song_got.tempo_bpm;
    s_app_ctx.last_pot_pct      = 0;

    /* 2. Initialize Passive Buzzer */
    const buzzer_config_t buzzer_cfg = {
        .gpio_num  = (gpio_num_t)CONFIG_ESPOUT_BUZZER_GPIO,
        .timer_num = LEDC_TIMER_0,
        .channel   = LEDC_CHANNEL_0
    };
    ESP_ERROR_CHECK(buzzer_init(&s_app_ctx.buzzer, &buzzer_cfg));

    /* 3. Initialize ST7789 LCD (1.69" 240x280) */
    const st7789_config_t lcd_cfg = {
        .spi_host = SPI2_HOST,
        .clock_speed_hz = 40000000,
        .gpio_mosi = CONFIG_ESPOUT_LCD_DIN_GPIO,
        .gpio_clk = CONFIG_ESPOUT_LCD_SCLK_GPIO,
        .gpio_cs = CONFIG_ESPOUT_LCD_CS_GPIO,
        .gpio_dc = CONFIG_ESPOUT_LCD_DC_GPIO,
        .gpio_rst = CONFIG_ESPOUT_LCD_RESET_GPIO,
        .gpio_bckl = CONFIG_ESPOUT_LCD_BACKLIGHT_GPIO
    };
    ESP_ERROR_CHECK(st7789_init(&lcd_cfg, &s_app_ctx.lcd));

    /* 4. Initialize Potentiometer ADC */
    adc_oneshot_unit_handle_t adc_unit_handle;
    adc_oneshot_unit_init_cfg_t unit_init = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_init, &adc_unit_handle));

    const pot_config_t pot_cfg = {
        .adc_unit_handle = adc_unit_handle,
        .adc_gpio = (gpio_num_t)CONFIG_ESPOUT_POT_ADC_GPIO,
        .vcc_gpio = (gpio_num_t)CONFIG_ESPOUT_POT_VCC_GPIO,
        .gnd_gpio = (gpio_num_t)CONFIG_ESPOUT_POT_GND_GPIO,
        .attenuation = ADC_ATTEN_DB_12,
        .min_raw_value = 0,
        .max_raw_value = 4095
    };
    ESP_ERROR_CHECK(pot_adc_init(&s_app_ctx.pot, &pot_cfg));

    /* Read initial pot position to set baseline tempo */
    int initial_pct = 0;
    if (pot_adc_read_scaled(s_app_ctx.pot, 100, &initial_pct) == ESP_OK) {
        s_app_ctx.last_pot_pct   = initial_pct;
        s_app_ctx.last_tempo_bpm = (uint16_t)(MIN_TEMPO_BPM +
            (initial_pct * (MAX_TEMPO_BPM - MIN_TEMPO_BPM)) / 100);
        (void)buzzer_player_set_tempo(s_app_ctx.buzzer, s_app_ctx.last_tempo_bpm);
    }

    /* 5. Initialize IR Receiver with callback */
    ESP_ERROR_CHECK(ir_handler_init(s_ir_profile_benq, 
                                    sizeof(s_ir_profile_benq) / sizeof(s_ir_profile_benq[0]),
                                    on_ir_key_event));

    /* 6. Spawn Background Tasks */
    xTaskCreate(ui_task, "ui_task", 4096, &s_app_ctx, 3, NULL);
    xTaskCreate(pot_monitor_task, "pot_monitor_task", 3072, &s_app_ctx, 4, NULL);

    ESP_LOGI(TAG, "Application online. Press IR [ON] to play Game of Thrones theme.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}