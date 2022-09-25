#include "http/flash.h"

#include "config.h"
#include "update.h"

#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *const TAG = "HTTP       :  Flash    ";

static esp_err_t post_flash_handler(httpd_req_t *);

const httpd_uri_t flash_uri_handler = {
    .uri = CONFIG_FLASH_URI "/?*",
    .method = HTTP_POST,
    .handler = &post_flash_handler,
    .user_ctx = NULL,
};

static esp_err_t post_flash_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received request at \"%s\"", req->uri);

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_status(req, HTTPD_500);

    size_t remaining = req->content_len;
    switch (update_prepare(remaining))
    {
    case UPDATE_ERR_IMAGE_TOO_LARGE:;
        char msg[39];
        sprintf(msg, "Image size must be less than %u KiB.", CONFIG_UPDATE_PARTITION_SIZE / 1024);
        httpd_resp_set_status(req, "413 Payload Too Large");
        httpd_resp_sendstr(req, msg);
        return ESP_FAIL;

    case UPDATE_ERR_NO_PARTITION_FOUND:
        httpd_resp_sendstr(req, "Cannot access firmware partition.");
        return ESP_FAIL;

    case UPDATE_FAILED:
        httpd_resp_sendstr(req, "Cannot start firmware update.");
        return ESP_FAIL;

    default:
        break;
    }

    char *buffer = malloc(CONFIG_FLASH_BUFFER_SIZE);
    while (remaining > 0)
    {
        int32_t received = httpd_req_recv(req, buffer, CONFIG_FLASH_BUFFER_SIZE);

        if (received == HTTPD_SOCK_ERR_TIMEOUT)
            continue;

        if (received <= 0)
        {
            ESP_LOGE(TAG, "Image upload failed.");
            update_abort();

            httpd_resp_sendstr(req, "Image upload failed. Try again!");
            return ESP_FAIL;
        }

        if (update_write(buffer, received, &remaining) == UPDATE_FAILED)
        {
            httpd_resp_sendstr(req, "Cannot write to firmware partition.");
            return ESP_FAIL;
        }
    }

    free(buffer);

    switch (update_finish())
    {
    case UPDATE_ERR_VALIDATION_FAILED:
        httpd_resp_sendstr(req, "Image validation failed. Try again!");
        return ESP_FAIL;

    case UPDATE_FAILED:
        httpd_resp_sendstr(req, "Cannot finish firmware update.");
        return ESP_FAIL;

    case UPDATE_ERR_CHANGE_BOOT_FAILED:
        httpd_resp_sendstr(req, "Cannot boot new firmware.");
        return ESP_FAIL;

    default:
        break;
    }

    update_reboot();
}
