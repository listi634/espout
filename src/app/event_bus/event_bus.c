/**
 * @file event_bus.c
 * @brief Application event bus implementation.
 */

#include "event_bus.h"
#include "event_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "EVENT_BUS";

#define EVENT_QUEUE_SIZE    20
#define MAX_EVENT_TYPES     32
#define MAX_SUBSCRIBERS     10

static QueueHandle_t s_event_queue = NULL;

typedef struct {
    void (*callback)(const app_event_t *event);
} subscriber_t;

static subscriber_t s_subscribers[MAX_EVENT_TYPES][MAX_SUBSCRIBERS];
static size_t s_subscriber_count[MAX_EVENT_TYPES] = {0};

static void event_dispatcher_task(void *arg)
{
    (void)arg;
    app_event_t event;

    while (1) {
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            do {
                if (event.type >= MAX_EVENT_TYPES) {
                    continue;
                }

                for (size_t i = 0; i < s_subscriber_count[event.type]; i++) {
                    if (s_subscribers[event.type][i].callback != NULL) {
                        s_subscribers[event.type][i].callback(&event);
                    }
                }
            } while (xQueueReceive(s_event_queue, &event, 0) == pdTRUE);
        }
    }
}

esp_err_t event_bus_init(void)
{
    s_event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(app_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0; i < MAX_EVENT_TYPES; i++) {
        s_subscriber_count[i] = 0;
    }

    if (xTaskCreate(event_dispatcher_task, "evt_dispatch",
                   4096, NULL, 5, NULL) != pdPASS) {
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
        ESP_LOGE(TAG, "Failed to create dispatcher task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Event bus initialized");
    return ESP_OK;
}

void event_bus_deinit(void)
{
    if (s_event_queue != NULL) {
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
    }
}

esp_err_t event_bus_publish(const app_event_t *event)
{
    if (event == NULL || s_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(s_event_queue, event, pdMS_TO_TICKS(10)) != pdPASS) {
        ESP_LOGW(TAG, "Event queue full, dropping event type: %d", event->type);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t event_bus_subscribe(
    const app_event_type_t *event_types,
    size_t num_types,
    void (*callback)(const app_event_t *event))
{
    if (callback == NULL || num_types == 0 || num_types > MAX_EVENT_TYPES) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < num_types; i++) {
        if (event_types[i] >= MAX_EVENT_TYPES) {
            continue;
        }

        if (s_subscriber_count[event_types[i]] >= MAX_SUBSCRIBERS) {
            ESP_LOGW(TAG, "Max subscribers reached for event type: %d", event_types[i]);
            continue;
        }

        s_subscribers[event_types[i]][s_subscriber_count[event_types[i]]].callback = callback;
        s_subscriber_count[event_types[i]]++;
    }

    return ESP_OK;
}

void event_bus_unsubscribe(void (*callback)(const app_event_t *event))
{
    if (callback == NULL) {
        return;
    }

    for (int i = 0; i < MAX_EVENT_TYPES; i++) {
        for (size_t j = 0; j < s_subscriber_count[i]; j++) {
            if (s_subscribers[i][j].callback == callback) {
                for (size_t k = j; k < s_subscriber_count[i] - 1; k++) {
                    s_subscribers[i][k] = s_subscribers[i][k + 1];
                }
                s_subscriber_count[i]--;
                break;
            }
        }
    }
}
