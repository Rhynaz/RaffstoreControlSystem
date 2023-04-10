#include "http/status.h"

#include "config.h"
#include "controller.h"

#include "esp_log.h"
#include "esp_http_server.h"

static const char *const TAG = "HTTP       : Status   ";

static esp_err_t get_status_handler(httpd_req_t *);

const httpd_uri_t status_uri_handler = {
    .uri = CONFIG_STATUS_URI "/?*",
    .method = HTTP_GET,
    .handler = &get_status_handler,
    .user_ctx = NULL,
};

static esp_err_t get_status_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received request at \"%s\"", req->uri);

    size_t length = 64;
    char *response = calloc(length, sizeof(char));

    if (!strcmp(req->uri, CONFIG_STATUS_URI) || !strcmp(req->uri, CONFIG_STATUS_URI "/"))
    {
        int8_t *status = controller_query_all();

        strcat(response, "[ ");

        char tmp[8];
        while (*status != -1)
        {
            snprintf(tmp, 8, "%d, ", *status);
            status++;

            if (strlen(response) + strlen(tmp) > length)
            {
                length *= 2;
                response = realloc(response, sizeof(char) * length);
            }

            strcat(response, tmp);
        }

        size_t len = strlen(response);
        response[len - 2] = ' ';
        response[len - 1] = ']';
    }
    else
    {
        char *end;
        const char *start = req->uri + strlen(CONFIG_STATUS_URI "/");
        uint64_t channel = strtoul(start, &end, 10);

        if (channel > 255 || end == start || *end)
            return ESP_ERR_INVALID_ARG;

        int8_t status = controller_query((uint8_t)channel);
        snprintf(response, length, "%d", status);
    }

    esp_err_t err = httpd_resp_set_hdr(req, "Connection", "close");
    if (err != ESP_OK)
        return err;

    err = httpd_resp_set_hdr(req, "Content-Type", "application/json");
    if (err != ESP_OK)
        return err;

    err = httpd_resp_sendstr(req, response);
    free(response);
    return err;
}
