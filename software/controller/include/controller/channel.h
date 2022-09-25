#pragma once

#include "hal/gpio_types.h"
#include "esp_event_base.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

ESP_EVENT_DECLARE_BASE(CHANNEL_EVENT);
typedef enum channel_event
{
    CHANNEL_EVENT_OPEN,
    CHANNEL_EVENT_CLOSE,
    CHANNEL_EVENT_STOP,
} channel_event_t;

typedef struct channel
{
    const uint8_t index;
    const TickType_t stop_timeout_sec;

    const gpio_num_t motor_enable;
    const gpio_num_t motor_direction;
    const uint8_t motor_invert;

    const gpio_num_t switch_up;
    const gpio_num_t switch_down;
    const uint8_t switch_invert;

    esp_event_loop_handle_t event_loop;
    TaskHandle_t poll_task;
    TimerHandle_t stop_timer;
} channel_t;

void channel_init(channel_t *channel);
