#include "controller.h"

#include "controller/channel.h"

#include "config.h"

#include "esp_log.h"
#include "esp_event.h"
#include "driver/gpio.h"

#define CONTROLLER_DEFINE_CHANNEL(num)                                         \
    [CONFIG_CONTROLLER_CHANNEL##num##_INDEX] = (channel_t)                     \
    {                                                                          \
        .index = CONFIG_CONTROLLER_CHANNEL##num##_INDEX,                       \
        .stop_timeout_sec = CONFIG_CONTROLLER_CHANNEL##num##_STOP_TIMEOUT_SEC, \
        .motor_enable = CONFIG_CONTROLLER_CHANNEL##num##_MOTOR_ENABLE,         \
        .motor_direction = CONFIG_CONTROLLER_CHANNEL##num##_MOTOR_DIRECTION,   \
        .motor_invert = CONFIG_CONTROLLER_CHANNEL##num##_MOTOR_INVERT,         \
        .switch_up = CONFIG_CONTROLLER_CHANNEL##num##_SWITCH_UP,               \
        .switch_down = CONFIG_CONTROLLER_CHANNEL##num##_SWITCH_DOWN,           \
        .switch_invert = CONFIG_CONTROLLER_CHANNEL##num##_SWITCH_INVERT,       \
    }

static const char *const TAG = "Controller ";

static channel_t channels[CONFIG_CONTROLLER_CHANNEL_NUM] = {
#ifdef CONFIG_CONTROLLER_CHANNEL0_ENABLE
    CONTROLLER_DEFINE_CHANNEL(0),
#endif
#ifdef CONFIG_CONTROLLER_CHANNEL1_ENABLE
    CONTROLLER_DEFINE_CHANNEL(1),
#endif
#ifdef CONFIG_CONTROLLER_CHANNEL2_ENABLE
    CONTROLLER_DEFINE_CHANNEL(2),
#endif
#ifdef CONFIG_CONTROLLER_CHANNEL3_ENABLE
    CONTROLLER_DEFINE_CHANNEL(3),
#endif
#ifdef CONFIG_CONTROLLER_CHANNEL4_ENABLE
    CONTROLLER_DEFINE_CHANNEL(4),
#endif
#ifdef CONFIG_CONTROLLER_CHANNEL5_ENABLE
    CONTROLLER_DEFINE_CHANNEL(5),
#endif
#ifdef CONFIG_CONTROLLER_CHANNEL6_ENABLE
    CONTROLLER_DEFINE_CHANNEL(6),
#endif
};

void controller_init()
{
    ESP_LOGI(TAG, "Configure GPIOs.");
    gpio_config_t switch_config = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config_t motor_config = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    for (uint8_t i = 0; i < CONFIG_CONTROLLER_CHANNEL_NUM; i++)
    {
        switch_config.pin_bit_mask |= (1ULL << channels[i].switch_up) | (1ULL << channels[i].switch_down);
        motor_config.pin_bit_mask |= (1ULL << channels[i].motor_enable) | (1ULL << channels[i].motor_direction);
    }

    ESP_ERROR_CHECK(gpio_config(&switch_config));
    ESP_ERROR_CHECK(gpio_config(&motor_config));

    ESP_LOGI(TAG, "Initialize channels.");
    for (uint8_t i = 0; i < CONFIG_CONTROLLER_CHANNEL_NUM; i++)
        channel_init(&channels[i]);
}

void controller_open(uint8_t channel_num, bool user_initiated)
{
    if (channel_num >= CONFIG_CONTROLLER_CHANNEL_NUM)
    {
        ESP_LOGE(TAG, "Tried opening channel %u of %u.", channel_num, CONFIG_CONTROLLER_CHANNEL_NUM);
        return;
    }

    esp_event_post_to(channels[channel_num].event_loop, CHANNEL_EVENT, CHANNEL_EVENT_OPEN, &user_initiated, sizeof(user_initiated), 0);
}

void controller_open_all(bool user_initiated)
{
    ESP_LOGI(TAG, "Open all channels.");

    for (uint8_t i = 0; i < CONFIG_CONTROLLER_CHANNEL_NUM; i++)
    {
        controller_open(i, user_initiated);
    }
}

void controller_close(uint8_t channel_num, bool user_initiated)
{
    if (channel_num >= CONFIG_CONTROLLER_CHANNEL_NUM)
    {
        ESP_LOGE(TAG, "Tried closing channel %u of %u.", channel_num, CONFIG_CONTROLLER_CHANNEL_NUM);
        return;
    }

    esp_event_post_to(channels[channel_num].event_loop, CHANNEL_EVENT, CHANNEL_EVENT_CLOSE, &user_initiated, sizeof(user_initiated), 0);
}

void controller_close_all(bool user_initiated)
{
    ESP_LOGI(TAG, "Close all channels.");

    for (uint8_t i = 0; i < CONFIG_CONTROLLER_CHANNEL_NUM; i++)
    {
        controller_close(i, user_initiated);
    }
}

void controller_stop(uint8_t channel_num, bool user_initiated)
{
    if (channel_num >= CONFIG_CONTROLLER_CHANNEL_NUM)
    {
        ESP_LOGE(TAG, "Tried stopping channel %u of %u.", channel_num, CONFIG_CONTROLLER_CHANNEL_NUM);
        return;
    }

    esp_event_post_to(channels[channel_num].event_loop, CHANNEL_EVENT, CHANNEL_EVENT_STOP, &user_initiated, sizeof(user_initiated), 0);
}

void controller_stop_all(bool user_initiated)
{
    ESP_LOGI(TAG, "Stop all channels.");

    for (uint8_t i = 0; i < CONFIG_CONTROLLER_CHANNEL_NUM; i++)
    {
        controller_stop(i, user_initiated);
    }
}

int8_t controller_query(uint8_t channel_num)
{
    if (channel_num >= CONFIG_CONTROLLER_CHANNEL_NUM)
    {
        ESP_LOGE(TAG, "Tried querying channel %u of %u.", channel_num, CONFIG_CONTROLLER_CHANNEL_NUM);
        return -1;
    }

    return channels[channel_num].last_user_event;
}

int8_t *controller_query_all()
{
    ESP_LOGI(TAG, "Query all channels.");

    static int8_t results[CONFIG_CONTROLLER_CHANNEL_NUM + 1];
    results[CONFIG_CONTROLLER_CHANNEL_NUM] = -1;

    for (uint8_t i = 0; i < CONFIG_CONTROLLER_CHANNEL_NUM; i++)
    {
        results[i] = controller_query(i);
    }

    return results;
}
