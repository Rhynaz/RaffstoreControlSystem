#pragma once

#include "controller/channel.h"

void controller_init();

void controller_open(uint8_t channel_num, bool user_initiated);
void controller_open_all(bool user_initiated);

void controller_close(uint8_t channel_num, bool user_initiated);
void controller_close_all(bool user_initiated);

void controller_stop(uint8_t channel_num, bool user_initiated);
void controller_stop_all(bool user_initiated);

int8_t controller_query(uint8_t channel_num);
int8_t *controller_query_all();
