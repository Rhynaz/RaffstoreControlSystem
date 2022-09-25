#include "network.h"
#include "controller.h"
#include "http.h"
#include "update.h"

#include "esp_log.h"
#include "esp_event.h"
#include "driver/gpio.h"

static const char *const TAG = "Main       ";

void app_main()
{
    ESP_LOGI(TAG, "Raffstore Control System");
    ESP_LOGI(TAG, "Version " CONFIG_APP_PROJECT_VER);

    ESP_LOGI(TAG, "Register global ISR handler.");
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    ESP_LOGI(TAG, "Create default event loop.");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Initialize network stack.");
    network_init();

    ESP_LOGI(TAG, "Initialize controller.");
    controller_init();

    ESP_LOGI(TAG, "Initialize HTTP server.");
    http_init();

    update_mark_valid();
}
