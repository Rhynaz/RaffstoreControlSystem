#include "http.h"

#include "http/index.h"
#include "http/actions.h"
#include "http/status.h"
#include "http/flash.h"

#include "config.h"
#include "network.h"

#include "esp_log.h"
#include "esp_http_server.h"

static const char *const TAG = "HTTP       ";

static httpd_handle_t server_handle = NULL;
static httpd_config_t config = HTTPD_DEFAULT_CONFIG();
static bool is_initialized = false;

void http_init()
{
    config.server_port = CONFIG_HTTP_SERVER_PORT;
    config.lru_purge_enable = true;
    config.uri_match_fn = &httpd_uri_match_wildcard;

    is_initialized = true;

    ESP_LOGI(TAG, "Register network handlers.");
    network_register_connect_handler(&http_start);
    network_register_disconnect_handler(&http_stop);
}

void http_start()
{
    if (server_handle != NULL)
        return;

    ESP_LOGI(TAG, "Starting...");

    if (!is_initialized)
    {
        ESP_LOGE(TAG, "Not initialized.");
        return;
    }

    if (httpd_start(&server_handle, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start server.");
        return;
    }

    httpd_register_uri_handler(server_handle, &index_uri_handler);

    httpd_register_uri_handler(server_handle, &actions_open_uri_handler);
    httpd_register_uri_handler(server_handle, &actions_close_uri_handler);
    httpd_register_uri_handler(server_handle, &actions_stop_uri_handler);

    httpd_register_uri_handler(server_handle, &status_uri_handler);

    httpd_register_uri_handler(server_handle, &flash_uri_handler);

    ESP_LOGI(TAG, "Started!");
}

void http_stop()
{
    if (server_handle == NULL)
        return;

    ESP_LOGI(TAG, "Stopping...");

    if (httpd_stop(server_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to stop server.");
        return;
    }

    server_handle = NULL;
    ESP_LOGI(TAG, "Stopped!");
}
