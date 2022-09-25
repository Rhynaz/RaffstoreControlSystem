#include "controller/channel.h"

#include "config.h"

#include "esp_log.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

ESP_EVENT_DEFINE_BASE(CHANNEL_EVENT);

static const char *const TAG = "Controller : Channel  ";

static void motor_open_handler(void *, esp_event_base_t, int32_t, void *);
static void motor_close_handler(void *, esp_event_base_t, int32_t, void *);
static void motor_stop_handler(void *, esp_event_base_t, int32_t, void *);
static inline void motor_stop_if_moving(channel_t *);
static inline void motor_change_direction(channel_t *, uint8_t);

static void poll_task_handler(void *);
static void switch_poll_up(channel_t *);
static void switch_poll_down(channel_t *);
static inline void switch_send_event(channel_t *, channel_event_t);

static void stop_timer_handler(TimerHandle_t);

void channel_init(channel_t *channel)
{
    ESP_LOGI(TAG, "%u : Create event loop.", channel->index);
    char loop_name[16];
    sprintf(loop_name, "channel%u_evts", channel->index);

    esp_event_loop_args_t event_loop_config = {
        .task_name = loop_name,
        .task_core_id = tskNO_AFFINITY,
        .task_stack_size = CONFIG_CHANNEL_LOOP_STACK_SIZE,
        .task_priority = CONFIG_CHANNEL_LOOP_TASK_PRIORITY,
        .queue_size = CONFIG_CHANNEL_LOOP_QUEUE_SIZE,
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_config, &channel->event_loop));

    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(channel->event_loop, CHANNEL_EVENT, CHANNEL_EVENT_OPEN, &motor_open_handler, channel, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(channel->event_loop, CHANNEL_EVENT, CHANNEL_EVENT_CLOSE, &motor_close_handler, channel, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(channel->event_loop, CHANNEL_EVENT, CHANNEL_EVENT_STOP, &motor_stop_handler, channel, NULL));

    ESP_LOGI(TAG, "%u : Create poll task.", channel->index);
    char task_name[16];
    sprintf(task_name, "channel%u_task", channel->index);

    BaseType_t err = xTaskCreate(&poll_task_handler, task_name, CONFIG_CHANNEL_POLL_STACK_SIZE, channel, CONFIG_CHANNEL_POLL_TASK_PRIORITY, &channel->poll_task);
    if (err != pdPASS)
        ESP_ERROR_CHECK(ESP_FAIL);

    ESP_LOGI(TAG, "%u : Create stop timer.", channel->index);
    char timer_name[16];
    sprintf(timer_name, "channel%u_timr", channel->index);

    channel->stop_timer = xTimerCreate(timer_name, channel->stop_timeout_sec * 1000 / portTICK_PERIOD_MS, pdFALSE, channel, &stop_timer_handler);
}

static void motor_open_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    channel_t *channel = (channel_t *)arg;

    ESP_LOGI(TAG, "%u : Opening...", channel->index);
    xTimerReset(channel->stop_timer, 0);

    motor_stop_if_moving(channel);
    motor_change_direction(channel, CONFIG_CHANNEL_MOTOR_DIRECTION_OFF != channel->motor_invert);
    gpio_set_level(channel->motor_enable, CONFIG_CHANNEL_MOTOR_ENABLE_ON);
}

static void motor_close_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    channel_t *channel = (channel_t *)arg;

    ESP_LOGI(TAG, "%u : Closing...", channel->index);
    xTimerReset(channel->stop_timer, 0);

    motor_stop_if_moving(channel);
    motor_change_direction(channel, CONFIG_CHANNEL_MOTOR_DIRECTION_ON != channel->motor_invert);
    gpio_set_level(channel->motor_enable, CONFIG_CHANNEL_MOTOR_ENABLE_ON);
}

static void motor_stop_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    channel_t *channel = (channel_t *)arg;

    ESP_LOGI(TAG, "%u : Stopped!", channel->index);
    xTimerStop(channel->stop_timer, 0);

    gpio_set_level(channel->motor_enable, CONFIG_CHANNEL_MOTOR_ENABLE_OFF);
    motor_change_direction(channel, CONFIG_CHANNEL_MOTOR_DIRECTION_OFF);
}

static inline void motor_stop_if_moving(channel_t *channel)
{
    if (gpio_get_level(channel->motor_enable) == CONFIG_CHANNEL_MOTOR_ENABLE_OFF)
        return;

    gpio_set_level(channel->motor_enable, CONFIG_CHANNEL_MOTOR_ENABLE_OFF);
    vTaskDelay(CONFIG_CHANNEL_MOTOR_REVERSING_DELAY_MS / portTICK_PERIOD_MS);
}

static inline void motor_change_direction(channel_t *channel, uint8_t direction)
{
    vTaskDelay(CONFIG_CHANNEL_MOTOR_RELAY_DELAY_MS / portTICK_PERIOD_MS);
    gpio_set_level(channel->motor_direction, direction);
    vTaskDelay(CONFIG_CHANNEL_MOTOR_RELAY_DELAY_MS / portTICK_PERIOD_MS);
}

static void poll_task_handler(void *arg)
{
    channel_t *channel = (channel_t *)arg;

    ESP_LOGI(TAG, "%u : Start polling.", channel->index);
    while (1)
    {
        switch_poll_up(channel);
        switch_poll_down(channel);

        vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);
    }
}

static void switch_poll_up(channel_t *channel)
{
    if (gpio_get_level(channel->switch_up) == CONFIG_CHANNEL_SWITCH_UP_OFF)
        return;

    ESP_LOGI(TAG, "%u : Switch UP pressed.", channel->index);
    switch_send_event(channel, CHANNEL_EVENT_OPEN);
    vTaskDelay(CONFIG_CHANNEL_SWITCH_HOLD_DELAY_MS / portTICK_PERIOD_MS);

    if (gpio_get_level(channel->switch_up) == CONFIG_CHANNEL_SWITCH_UP_ON) // Holding
    {
        ESP_LOGI(TAG, "%u : Switch UP held.", channel->index);
        while (gpio_get_level(channel->switch_up) == CONFIG_CHANNEL_SWITCH_UP_ON)
            vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "%u : Switch UP hold released.", channel->index);
        switch_send_event(channel, CHANNEL_EVENT_STOP);
        return;
    }

    ESP_LOGI(TAG, "%u : Switch UP released.", channel->index);
    for (uint32_t delay_count = 1; gpio_get_level(channel->switch_up) == CONFIG_CHANNEL_SWITCH_UP_OFF; delay_count++)
    {
        vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);
        if (delay_count * CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS > channel->stop_timeout_sec * 1000)
            return;
    }

    ESP_LOGI(TAG, "%u : Switch UP pressed again.", channel->index);
    switch_send_event(channel, CHANNEL_EVENT_STOP);
    vTaskDelay(CONFIG_CHANNEL_SWITCH_HOLD_DELAY_MS / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "%u : Switch UP released again.", channel->index);
}

static void switch_poll_down(channel_t *channel)
{
    if (gpio_get_level(channel->switch_down) == CONFIG_CHANNEL_SWITCH_DOWN_OFF)
        return;

    ESP_LOGI(TAG, "%u : Switch DOWN pressed.", channel->index);
    switch_send_event(channel, CHANNEL_EVENT_CLOSE);
    vTaskDelay(CONFIG_CHANNEL_SWITCH_HOLD_DELAY_MS / portTICK_PERIOD_MS);

    if (gpio_get_level(channel->switch_down) == CONFIG_CHANNEL_SWITCH_DOWN_ON) // Holding
    {
        ESP_LOGI(TAG, "%u : Switch DOWN held.", channel->index);
        while (gpio_get_level(channel->switch_down) == CONFIG_CHANNEL_SWITCH_DOWN_ON)
            vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "%u : Switch DOWN hold released.", channel->index);
        switch_send_event(channel, CHANNEL_EVENT_STOP);
        return;
    }

    ESP_LOGI(TAG, "%u : Switch DOWN released.", channel->index);
    for (uint32_t delay_count = 1; gpio_get_level(channel->switch_down) == CONFIG_CHANNEL_SWITCH_DOWN_OFF; delay_count++)
    {
        vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);
        if (delay_count * CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS > channel->stop_timeout_sec * 1000)
            return;
    }

    ESP_LOGI(TAG, "%u : Switch DOWN pressed again.", channel->index);
    switch_send_event(channel, CHANNEL_EVENT_STOP);
    vTaskDelay(CONFIG_CHANNEL_SWITCH_HOLD_DELAY_MS / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "%u : Switch DOWN released again.", channel->index);
}

static inline void switch_send_event(channel_t *channel, channel_event_t evt)
{
    if (channel->switch_invert && evt == CHANNEL_EVENT_OPEN)
        evt = CHANNEL_EVENT_CLOSE;
    else if (channel->switch_invert && evt == CHANNEL_EVENT_CLOSE)
        evt = CHANNEL_EVENT_OPEN;

    esp_event_post_to(channel->event_loop, CHANNEL_EVENT, evt, NULL, 0, 0);
}

static void stop_timer_handler(TimerHandle_t timer)
{
    xTimerStop(timer, 0);

    channel_t *channel = (channel_t *)pvTimerGetTimerID(timer);
    ESP_LOGI(TAG, "%u : Stop timeout reached.", channel->index);
    esp_event_post_to(channel->event_loop, CHANNEL_EVENT, CHANNEL_EVENT_STOP, NULL, 0, 0);
}
