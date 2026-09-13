/**
 * @file buzzer.c
 * @brief Implementation of the passive buzzer LEDC driver and non-blocking player.
 */

#include "buzzer/buzzer.h"
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "BUZZER";

#define BUZZER_DUTY_RES           LEDC_TIMER_10_BIT
#define BUZZER_DUTY_50_PERCENT    512
#define BUZZER_INITIAL_FREQ_HZ    1000
#define BUZZER_TASK_STACK_SIZE    3072
#define BUZZER_TASK_PRIORITY      5
#define BUZZER_SLICE_PERIOD_MS    10

/**
 * @brief Internal device structure holding runtime state and peripheral assignments.
 */
struct buzzer_dev_s {
    gpio_num_t gpio_num;
    ledc_mode_t speed_mode;
    ledc_timer_t timer_num;
    ledc_channel_t channel;
    uint32_t duty_50;

    /* Background player state */
    TaskHandle_t player_task_handle;
    SemaphoreHandle_t mutex;
    const buzzer_melody_t *current_melody;
    size_t current_note_idx;
    uint16_t current_tempo_bpm;
    buzzer_player_state_t state;
    bool loop_enabled;
    bool task_running;
};

/**
 * @brief Computes active and pause durations in milliseconds for a specific note.
 */
static void calculate_note_timings(const buzzer_note_t *note, uint16_t tempo_bpm,
                                  uint32_t *out_active_ms, uint32_t *out_pause_ms)
{
    const uint32_t wholenote_ms = (60000U * 4U) / (uint32_t)tempo_bpm;
    uint32_t total_duration_ms = 0;

    if (note->duration_divider > 0) {
        total_duration_ms = wholenote_ms / (uint32_t)note->duration_divider;
    } else if (note->duration_divider < 0) {
        const uint32_t abs_divider = (uint32_t)(-note->duration_divider);
        total_duration_ms = (wholenote_ms * 3U) / (abs_divider * 2U);
    }

    if (total_duration_ms == 0) {
        *out_active_ms = 0;
        *out_pause_ms  = 0;
        return;
    }

    /* 90% note duration active tone, 10% silent articulation gap */
    *out_active_ms = (total_duration_ms * 9U) / 10U;
    *out_pause_ms  = total_duration_ms - *out_active_ms;
}

/**
 * @brief Background task handling non-blocking playback and tempo scaling.
 */
static void buzzer_player_task(void *param)
{
    struct buzzer_dev_s *dev = (struct buzzer_dev_s *)param;

    while (dev->task_running) {
        if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(BUZZER_SLICE_PERIOD_MS));
            continue;
        }

        if ((dev->state != BUZZER_PLAYER_STATE_PLAYING) || (dev->current_melody == NULL)) {
            xSemaphoreGive(dev->mutex);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        const buzzer_melody_t *melody = dev->current_melody;

        /* Check for melody completion and looping */
        if (dev->current_note_idx >= melody->note_count) {
            if (dev->loop_enabled) {
                dev->current_note_idx = 0;
            } else {
                dev->state = BUZZER_PLAYER_STATE_IDLE;
                (void)buzzer_tone_stop(dev);
                xSemaphoreGive(dev->mutex);
                continue;
            }
        }

        const buzzer_note_t note = melody->notes[dev->current_note_idx];
        const uint16_t tempo = dev->current_tempo_bpm;
        xSemaphoreGive(dev->mutex);

        uint32_t active_ms = 0;
        uint32_t pause_ms = 0;
        calculate_note_timings(&note, tempo, &active_ms, &pause_ms);

        /* 1. Play active tone portion */
        if (note.freq_hz != 0) {
            (void)buzzer_tone_start(dev, note.freq_hz);
        } else {
            (void)buzzer_tone_stop(dev);
        }

        uint32_t elapsed_ms = 0;
        while ((elapsed_ms < active_ms) && dev->task_running) {
            if (xSemaphoreTake(dev->mutex, portMAX_DELAY) == pdTRUE) {
                if (dev->state != BUZZER_PLAYER_STATE_PLAYING) {
                    (void)buzzer_tone_stop(dev);
                    xSemaphoreGive(dev->mutex);
                    break;
                }

                /* If tempo updated during note playback, recalculate timing dynamically */
                if (dev->current_tempo_bpm != tempo) {
                    calculate_note_timings(&note, dev->current_tempo_bpm, &active_ms, &pause_ms);
                }
                xSemaphoreGive(dev->mutex);
            }

            const uint32_t slice = (active_ms - elapsed_ms > BUZZER_SLICE_PERIOD_MS)
                                 ? BUZZER_SLICE_PERIOD_MS : (active_ms - elapsed_ms);
            vTaskDelay(pdMS_TO_TICKS(slice));
            elapsed_ms += slice;
        }

        (void)buzzer_tone_stop(dev);

        /* 2. Play articulation silence */
        elapsed_ms = 0;
        while ((elapsed_ms < pause_ms) && dev->task_running) {
            if (xSemaphoreTake(dev->mutex, portMAX_DELAY) == pdTRUE) {
                if (dev->state != BUZZER_PLAYER_STATE_PLAYING) {
                    xSemaphoreGive(dev->mutex);
                    break;
                }
                xSemaphoreGive(dev->mutex);
            }

            const uint32_t slice = (pause_ms - elapsed_ms > BUZZER_SLICE_PERIOD_MS)
                                 ? BUZZER_SLICE_PERIOD_MS : (pause_ms - elapsed_ms);
            vTaskDelay(pdMS_TO_TICKS(slice));
            elapsed_ms += slice;
        }

        /* 3. Advance to next note if not paused */
        if (xSemaphoreTake(dev->mutex, portMAX_DELAY) == pdTRUE) {
            if (dev->state == BUZZER_PLAYER_STATE_PLAYING) {
                dev->current_note_idx++;
            }
            xSemaphoreGive(dev->mutex);
        }
    }

    vTaskDelete(NULL);
}

esp_err_t buzzer_init(buzzer_handle_t *out_handle, const buzzer_config_t *config)
{
    if ((out_handle == NULL) || (config == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    struct buzzer_dev_s *dev = (struct buzzer_dev_s *)calloc(1, sizeof(struct buzzer_dev_s));
    if (dev == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for buzzer device context");
        return ESP_ERR_NO_MEM;
    }

    dev->gpio_num           = config->gpio_num;
    dev->speed_mode         = LEDC_LOW_SPEED_MODE;
    dev->timer_num          = config->timer_num;
    dev->channel            = config->channel;
    dev->duty_50            = BUZZER_DUTY_50_PERCENT;
    dev->state              = BUZZER_PLAYER_STATE_IDLE;
    dev->task_running       = true;
    dev->current_tempo_bpm  = 120;

    dev->mutex = xSemaphoreCreateMutex();
    if (dev->mutex == NULL) {
        free(dev);
        return ESP_ERR_NO_MEM;
    }

    const ledc_timer_config_t timer_conf = {
        .speed_mode       = dev->speed_mode,
        .timer_num        = dev->timer_num,
        .duty_resolution  = BUZZER_DUTY_RES,
        .freq_hz          = BUZZER_INITIAL_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        vSemaphoreDelete(dev->mutex);
        free(dev);
        return err;
    }

    const ledc_channel_config_t chan_conf = {
        .gpio_num       = dev->gpio_num,
        .speed_mode     = dev->speed_mode,
        .channel        = dev->channel,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = dev->timer_num,
        .duty           = 0,
        .hpoint         = 0
    };
    err = ledc_channel_config(&chan_conf);
    if (err != ESP_OK) {
        vSemaphoreDelete(dev->mutex);
        free(dev);
        return err;
    }

    BaseType_t task_ret = xTaskCreate(buzzer_player_task, "buzzer_player", BUZZER_TASK_STACK_SIZE,
                                      dev, BUZZER_TASK_PRIORITY, &dev->player_task_handle);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to spawn buzzer player background task");
        vSemaphoreDelete(dev->mutex);
        free(dev);
        return ESP_FAIL;
    }

    *out_handle = dev;
    ESP_LOGI(TAG, "Initialized passive buzzer on GPIO %d with player engine", dev->gpio_num);
    return ESP_OK;
}

esp_err_t buzzer_deinit(buzzer_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    handle->task_running = false;
    vTaskDelay(pdMS_TO_TICKS(50));

    (void)buzzer_tone_stop(handle);
    (void)ledc_stop(handle->speed_mode, handle->channel, 0);

    if (handle->mutex != NULL) {
        vSemaphoreDelete(handle->mutex);
    }
    free(handle);
    return ESP_OK;
}

esp_err_t buzzer_tone_start(buzzer_handle_t handle, uint32_t freq_hz)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (freq_hz == 0) {
        return buzzer_tone_stop(handle);
    }

    esp_err_t err = ledc_set_freq(handle->speed_mode, handle->timer_num, freq_hz);
    if (err != ESP_OK) {
        return err;
    }

    err = ledc_set_duty(handle->speed_mode, handle->channel, handle->duty_50);
    if (err != ESP_OK) {
        return err;
    }

    return ledc_update_duty(handle->speed_mode, handle->channel);
}

esp_err_t buzzer_tone_stop(buzzer_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ledc_set_duty(handle->speed_mode, handle->channel, 0);
    if (err != ESP_OK) {
        return err;
    }

    return ledc_update_duty(handle->speed_mode, handle->channel);
}

esp_err_t buzzer_beep(buzzer_handle_t handle, uint32_t freq_hz, uint32_t duration_ms)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = buzzer_tone_start(handle, freq_hz);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    return buzzer_tone_stop(handle);
}

esp_err_t buzzer_player_start(buzzer_handle_t handle, const buzzer_melody_t *melody, bool loop)
{
    if ((handle == NULL) || (melody == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    handle->current_melody    = melody;
    handle->current_note_idx  = 0;
    handle->loop_enabled      = loop;
    handle->current_tempo_bpm = (melody->tempo_bpm > 0) ? melody->tempo_bpm : 120;
    handle->state             = BUZZER_PLAYER_STATE_PLAYING;

    xSemaphoreGive(handle->mutex);
    ESP_LOGI(TAG, "Started playing '%s' (Loop: %d, Tempo: %d BPM)",
             melody->name, loop, handle->current_tempo_bpm);
    return ESP_OK;
}

esp_err_t buzzer_player_pause(buzzer_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (handle->state == BUZZER_PLAYER_STATE_PLAYING) {
        handle->state = BUZZER_PLAYER_STATE_PAUSED;
        (void)buzzer_tone_stop(handle);
        ESP_LOGI(TAG, "Playback paused at note %u", (unsigned int)handle->current_note_idx);
    }

    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

esp_err_t buzzer_player_resume(buzzer_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (handle->state == BUZZER_PLAYER_STATE_PAUSED) {
        handle->state = BUZZER_PLAYER_STATE_PLAYING;
        ESP_LOGI(TAG, "Playback resumed from note %u", (unsigned int)handle->current_note_idx);
    }

    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

esp_err_t buzzer_player_stop(buzzer_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    handle->state            = BUZZER_PLAYER_STATE_IDLE;
    handle->current_note_idx = 0;
    (void)buzzer_tone_stop(handle);

    xSemaphoreGive(handle->mutex);
    ESP_LOGI(TAG, "Playback stopped");
    return ESP_OK;
}

esp_err_t buzzer_player_set_tempo(buzzer_handle_t handle, uint16_t tempo_bpm)
{
    if ((handle == NULL) || (tempo_bpm == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    handle->current_tempo_bpm = tempo_bpm;
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

uint16_t buzzer_player_get_tempo(buzzer_handle_t handle)
{
    if (handle == NULL) {
        return 0;
    }

    uint16_t tempo = 0;
    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE) {
        tempo = handle->current_tempo_bpm;
        xSemaphoreGive(handle->mutex);
    }
    return tempo;
}

buzzer_player_state_t buzzer_player_get_state(buzzer_handle_t handle)
{
    if (handle == NULL) {
        return BUZZER_PLAYER_STATE_IDLE;
    }

    buzzer_player_state_t current_state = BUZZER_PLAYER_STATE_IDLE;
    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE) {
        current_state = handle->state;
        xSemaphoreGive(handle->mutex);
    }
    return current_state;
}