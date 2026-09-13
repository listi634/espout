/**
 * @file buzzer.h
 * @brief Public interface for the passive PWM buzzer driver and melody player.
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "driver/ledc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle representing an initialized buzzer device instance.
 */
typedef struct buzzer_dev_s *buzzer_handle_t;

/**
 * @brief Playback states of the background melody player.
 */
typedef enum {
    BUZZER_PLAYER_STATE_IDLE = 0,   /**< Player stopped or not yet started. */
    BUZZER_PLAYER_STATE_PLAYING,    /**< Active melody playback ongoing. */
    BUZZER_PLAYER_STATE_PAUSED      /**< Playback paused mid-melody. */
} buzzer_player_state_t;

/**
 * @brief Hardware configuration parameters for buzzer initialization.
 */
typedef struct {
    gpio_num_t gpio_num;        /**< Target GPIO pin connected to buzzer signal. */
    ledc_timer_t timer_num;     /**< LEDC hardware timer index. */
    ledc_channel_t channel;     /**< LEDC hardware channel index. */
} buzzer_config_t;

/**
 * @brief Representation of a single musical note element.
 */
typedef struct {
    uint16_t freq_hz;           /**< Pitch frequency in Hz (use REST for pauses). */
    int16_t duration_divider;   /**< Note divider: positive for standard, negative for dotted. */
} buzzer_note_t;

/**
 * @brief Representation of an immutable melody asset.
 */
typedef struct {
    const char *name;           /**< Song identifier / title. */
    uint16_t tempo_bpm;         /**< Baseline playback speed in beats per minute. */
    const buzzer_note_t *notes; /**< Contiguous array of notes. */
    size_t note_count;          /**< Number of note elements in the array. */
} buzzer_melody_t;

/**
 * @brief Initializes the LEDC PWM peripheral and spawns the background playback task.
 * 
 * @param[out] out_handle Pointer to receive the allocated device handle.
 * @param[in]  config     Pointer to valid hardware configuration structure.
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t buzzer_init(buzzer_handle_t *out_handle, const buzzer_config_t *config);

/**
 * @brief Deinitializes the buzzer, halts the background task, and frees memory.
 * 
 * @param[in,out] handle Device handle to deallocate.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if handle is NULL.
 */
esp_err_t buzzer_deinit(buzzer_handle_t handle);

/**
 * @brief Starts generating a continuous tone at the specified frequency.
 * 
 * @param[in] handle  Valid buzzer device handle.
 * @param[in] freq_hz Frequency in Hertz.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid input.
 */
esp_err_t buzzer_tone_start(buzzer_handle_t handle, uint32_t freq_hz);

/**
 * @brief Stops active PWM output, placing the buzzer into a silent state.
 * 
 * @param[in] handle Valid buzzer device handle.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if handle is NULL.
 */
esp_err_t buzzer_tone_stop(buzzer_handle_t handle);

/**
 * @brief Plays a single tone for an exact duration (synchronous/blocking).
 * 
 * @param[in] handle      Valid buzzer device handle.
 * @param[in] freq_hz     Frequency in Hertz.
 * @param[in] duration_ms Duration in milliseconds.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid input.
 */
esp_err_t buzzer_beep(buzzer_handle_t handle, uint32_t freq_hz, uint32_t duration_ms);

/**
 * @brief Starts non-blocking background playback of a melody.
 * 
 * @param[in] handle Valid buzzer device handle.
 * @param[in] melody Pointer to configured melody asset.
 * @param[in] loop   If true, restarts from the beginning when the melody completes.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid input.
 */
esp_err_t buzzer_player_start(buzzer_handle_t handle, const buzzer_melody_t *melody, bool loop);

/**
 * @brief Pauses melody playback immediately, preserving current note position.
 * 
 * @param[in] handle Valid buzzer device handle.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if handle is NULL.
 */
esp_err_t buzzer_player_pause(buzzer_handle_t handle);

/**
 * @brief Resumes paused melody playback from the exact note position.
 * 
 * @param[in] handle Valid buzzer device handle.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if handle is NULL.
 */
esp_err_t buzzer_player_resume(buzzer_handle_t handle);

/**
 * @brief Stops playback and resets the song progress to the beginning.
 * 
 * @param[in] handle Valid buzzer device handle.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if handle is NULL.
 */
esp_err_t buzzer_player_stop(buzzer_handle_t handle);

/**
 * @brief Adjusts the playback tempo in real-time while playing or paused.
 * 
 * @param[in] handle    Valid buzzer device handle.
 * @param[in] tempo_bpm New tempo in BPM (beats per minute). Must be > 0.
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG on invalid input.
 */
esp_err_t buzzer_player_set_tempo(buzzer_handle_t handle, uint16_t tempo_bpm);

/**
 * @brief Retrieves the currently applied playback tempo.
 * 
 * @param[in] handle Valid buzzer device handle.
 * @return uint16_t Active tempo in BPM, or 0 if invalid.
 */
uint16_t buzzer_player_get_tempo(buzzer_handle_t handle);

/**
 * @brief Retrieves the current player state.
 * 
 * @param[in] handle Valid buzzer device handle.
 * @return buzzer_player_state_t Current state.
 */
buzzer_player_state_t buzzer_player_get_state(buzzer_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_H */