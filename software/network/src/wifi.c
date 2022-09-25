#include "network/wifi.h"

#include "config.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

static const char *const TAG = "Network    : Wi-Fi    ";

static volatile bool is_running = false;

static void wifi_handler(void *, esp_event_base_t, int32_t, void *);
static void reconnect_timer_handler(TimerHandle_t);

void wifi_init()
{
    ESP_LOGI(TAG, "Install Wi-Fi driver.");
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_LOGI(TAG, "Set connection details.");
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSPHRASE,
            .threshold.authmode = CONFIG_WIFI_AUTHENTICATION,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    ESP_LOGI(TAG, "Attach driver to network stack.");
    esp_netif_create_default_wifi_sta();

    ESP_LOGI(TAG, "Create reconnect timer.");
    TimerHandle_t reconnect_timer = xTimerCreate("wifi_reconnect", CONFIG_WIFI_RECONNECT_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS, pdFALSE, NULL, &reconnect_timer_handler);
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_handler, reconnect_timer, NULL));
}

void wifi_start()
{
    if (!is_running)
    {
        ESP_LOGI(TAG, "Starting...");
        esp_wifi_start();
        is_running = true;
    }
}

void wifi_stop()
{
    if (is_running)
    {
        ESP_LOGI(TAG, "Stopping...");
        esp_wifi_stop();
        is_running = false;
    }
}

static void wifi_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch (id)
    {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "Started!");
        ESP_LOGI(TAG, "Connecting...");
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_STOP:
        ESP_LOGI(TAG, "Stopped!");
        xTimerStop((TimerHandle_t)arg, 0);
        break;

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "Connected!");
        xTimerStop((TimerHandle_t)arg, 0);
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnected!");
        xTimerReset((TimerHandle_t)arg, 0);
        break;

    default:
        break;
    }
}

static void reconnect_timer_handler(TimerHandle_t timer)
{
    xTimerStop(timer, 0);

    if (is_running)
    {
        ESP_LOGI(TAG, "Reconnecting...");
        esp_wifi_connect();
    }
}
