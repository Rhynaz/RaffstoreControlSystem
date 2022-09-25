#include "network.h"

#include "network/ethernet.h"
#include "network/wifi.h"

#include "config.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

ESP_EVENT_DEFINE_BASE(NETWORK_EVENT);
enum
{
    NETWORK_EVENT_CONNECTED,
    NETWORK_EVENT_DISCONNECTED,
};

static const char *const TAG = "Network    ";

static void event_handler_helper(void *, esp_event_base_t, int32_t, void *);
static void network_event_handler(void *, esp_event_base_t, int32_t, void *);

static void ethernet_handler(void *, esp_event_base_t, int32_t, void *);
static void fallback_timer_handler(TimerHandle_t);

void network_init()
{
    ESP_LOGI(TAG, "Create network interfaces.");
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_LOGI(TAG, "Initialize Ethernet driver.");
    ethernet_init();

    ESP_LOGI(TAG, "Initialize Wi-Fi driver.");
    wifi_init();

    ESP_LOGI(TAG, "Create fallback timer.");
    TimerHandle_t fallback_timer = xTimerCreate("network_fallback", CONFIG_NETWORK_FALLBACK_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS, pdFALSE, NULL, &fallback_timer_handler);
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &ethernet_handler, fallback_timer, NULL));

    ESP_LOGI(TAG, "Register network events.");
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &network_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &network_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &network_event_handler, NULL, NULL));

    ESP_LOGI(TAG, "Start network.");
    ethernet_start();
    xTimerStart(fallback_timer, 0);
}

esp_err_t network_register_connect_handler(connection_handler_func *on_connect)
{
    return esp_event_handler_instance_register(NETWORK_EVENT, NETWORK_EVENT_CONNECTED, &event_handler_helper, on_connect, NULL);
}

esp_err_t network_register_disconnect_handler(connection_handler_func *on_disconnect)
{
    return esp_event_handler_instance_register(NETWORK_EVENT, NETWORK_EVENT_DISCONNECTED, &event_handler_helper, on_disconnect, NULL);
}

static void event_handler_helper(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (*(connection_handler_func *)arg)();
}

static void network_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch (id)
    {
    case IP_EVENT_ETH_GOT_IP:
    case IP_EVENT_STA_GOT_IP:
        esp_event_post(NETWORK_EVENT, NETWORK_EVENT_CONNECTED, NULL, 0, 0);
        break;

    case ETHERNET_EVENT_DISCONNECTED:
    case WIFI_EVENT_STA_DISCONNECTED:
        esp_event_post(NETWORK_EVENT, NETWORK_EVENT_DISCONNECTED, NULL, 0, 0);
        break;

    default:
        break;
    }
}

static void ethernet_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch (id)
    {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Stop Wi-Fi fallback timeout.");
        xTimerStop((TimerHandle_t)arg, 0);
        wifi_stop();
        break;

    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Start Wi-Fi fallback timeout.");
        xTimerReset((TimerHandle_t)arg, 0);
        break;

    default:
        break;
    }
}

static void fallback_timer_handler(TimerHandle_t timer)
{
    xTimerStop(timer, 0);

    ESP_LOGI(TAG, "Wi-Fi fallback timeout reached.");
    wifi_start();
}
