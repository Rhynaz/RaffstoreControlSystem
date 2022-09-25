#pragma once

#include "esp_err.h"

typedef void connection_handler_func();

void network_init();

esp_err_t network_register_connect_handler(connection_handler_func *on_connect);
esp_err_t network_register_disconnect_handler(connection_handler_func *on_disconnect);
