#pragma once

#include "controller/channel.h"

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

void controller_init();

void controller_open(uint8_t channel_num);
void controller_close(uint8_t channel_num);
void controller_stop(uint8_t channel_num);

void controller_open_all();
void controller_close_all();
void controller_stop_all();
