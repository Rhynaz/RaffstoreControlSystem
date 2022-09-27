#include "controller/channel.h"

#include "controller.h"

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
static inline void motor_stop_if_moving(channel_t *, uint8_t);
static inline void motor_change_direction(channel_t *, uint8_t);

static void poll_task_handler(void *);
static inline void switch_poll(channel_t *, uint8_t, gpio_num_t, uint8_t, gpio_num_t, uint8_t);

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

    motor_stop_if_moving(channel, CONFIG_CHANNEL_MOTOR_DIRECTION_ACTIVE == channel->motor_invert);
    motor_change_direction(channel, CONFIG_CHANNEL_MOTOR_DIRECTION_ACTIVE == channel->motor_invert);
    gpio_set_level(channel->motor_enable, CONFIG_CHANNEL_MOTOR_ENABLE_ACTIVE);
}

static void motor_close_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    channel_t *channel = (channel_t *)arg;

    ESP_LOGI(TAG, "%u : Closing...", channel->index);
    xTimerReset(channel->stop_timer, 0);

    motor_stop_if_moving(channel, CONFIG_CHANNEL_MOTOR_DIRECTION_ACTIVE != channel->motor_invert);
    motor_change_direction(channel, CONFIG_CHANNEL_MOTOR_DIRECTION_ACTIVE != channel->motor_invert);
    gpio_set_level(channel->motor_enable, CONFIG_CHANNEL_MOTOR_ENABLE_ACTIVE);
}

static void motor_stop_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    channel_t *channel = (channel_t *)arg;

    ESP_LOGI(TAG, "%u : Stopped!", channel->index);
    xTimerStop(channel->stop_timer, 0);

    gpio_set_level(channel->motor_enable, !CONFIG_CHANNEL_MOTOR_ENABLE_ACTIVE);
    motor_change_direction(channel, !CONFIG_CHANNEL_MOTOR_DIRECTION_ACTIVE);
}

static inline void motor_stop_if_moving(channel_t *channel, uint8_t direction)
{
    if (gpio_get_level(channel->motor_enable) != CONFIG_CHANNEL_MOTOR_ENABLE_ACTIVE ||
        gpio_get_level(channel->motor_direction) == direction)
        return;

    gpio_set_level(channel->motor_enable, !CONFIG_CHANNEL_MOTOR_ENABLE_ACTIVE);
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

    gpio_num_t switch_up = channel->switch_up;
    gpio_num_t switch_down = channel->switch_down;

    uint8_t switch_up_active = CONFIG_CHANNEL_SWITCH_UP_ACTIVE;
    uint8_t switch_down_active = CONFIG_CHANNEL_SWITCH_DOWN_ACTIVE;

    if (channel->switch_invert)
    {
        switch_up = channel->switch_down;
        switch_down = channel->switch_up;

        switch_up_active = CONFIG_CHANNEL_SWITCH_DOWN_ACTIVE;
        switch_down_active = CONFIG_CHANNEL_SWITCH_UP_ACTIVE;
    }

    ESP_LOGI(TAG, "%u : Start polling.", channel->index);
    while (1)
    {
        switch_poll(channel, 1, switch_up, switch_up_active, switch_down, switch_down_active);
        vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);

        switch_poll(channel, 0, switch_down, switch_down_active, switch_up, switch_up_active);
        vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);
    }
}

static inline void switch_poll(channel_t *channel, uint8_t direction,
                               gpio_num_t primary_switch, uint8_t primary_switch_active,
                               gpio_num_t secondary_switch, uint8_t secondary_switch_active)
{
    if (gpio_get_level(primary_switch) != primary_switch_active)
        return;

    ESP_LOGI(TAG, "%u : %s : Switch pressed.", channel->index, direction ? " Up " : "Down");

    if (direction)
        controller_open(channel->index);
    else
        controller_close(channel->index);

    vTaskDelay(CONFIG_CHANNEL_SWITCH_HOLD_DELAY_MS / portTICK_PERIOD_MS);

    if (gpio_get_level(primary_switch) == primary_switch_active)
    {
        ESP_LOGI(TAG, "%u : %s : Switch held.", channel->index, direction ? " Up " : "Down");

        while (gpio_get_level(primary_switch) == primary_switch_active)
            vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "%u : %s : Switch hold released.", channel->index, direction ? " Up " : "Down");

        controller_stop(channel->index);
        return;
    }

    ESP_LOGI(TAG, "%u : %s : Switch released.", channel->index, direction ? " Up " : "Down");

    uint32_t delay_count = 0;
    uint8_t double_clicked = 0;
    while (1)
    {
        if (gpio_get_level(primary_switch) == primary_switch_active)
        {
            if (double_clicked || delay_count * CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS >= CONFIG_CHANNEL_SWITCH_CLICK_DELAY_MS)
                break;

            ESP_LOGI(TAG, "%u : %s : Switch double clicked.", channel->index, direction ? " Up " : "Down");

            double_clicked = 1;
            delay_count = 0;

            if (direction)
                controller_open_all();
            else
                controller_close_all();

            vTaskDelay(CONFIG_CHANNEL_SWITCH_HOLD_DELAY_MS / portTICK_PERIOD_MS);
            continue;
        }

        if (gpio_get_level(secondary_switch) == secondary_switch_active)
            break;

        vTaskDelay(CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS / portTICK_PERIOD_MS);
        if (++delay_count * CONFIG_CHANNEL_SWITCH_POLLING_DELAY_MS >= channel->stop_timeout_sec * 1000)
            return;
    }

    ESP_LOGI(TAG, "%u : %s : Switch pressed again.", channel->index, direction ? " Up " : "Down");

    if (double_clicked)
        controller_stop_all();
    else
        controller_stop(channel->index);

    vTaskDelay(CONFIG_CHANNEL_SWITCH_HOLD_DELAY_MS / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "%u : %s : Switch released again.", channel->index, direction ? " Up " : "Down");
}

static void stop_timer_handler(TimerHandle_t timer)
{
    xTimerStop(timer, 0);

    channel_t *channel = (channel_t *)pvTimerGetTimerID(timer);
    ESP_LOGI(TAG, "%u : Stop timeout reached.", channel->index);
    esp_event_post_to(channel->event_loop, CHANNEL_EVENT, CHANNEL_EVENT_STOP, NULL, 0, 0);
}
