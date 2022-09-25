#include "http/actions.h"

#include "config.h"
#include "controller.h"

#include "esp_log.h"
#include "esp_http_server.h"

static const char *const TAG = "HTTP       : Actions  ";

static esp_err_t post_open_handler(httpd_req_t *);
static esp_err_t post_close_handler(httpd_req_t *);
static esp_err_t post_stop_handler(httpd_req_t *);

static esp_err_t parse_channel(const char *, uint8_t *);
static esp_err_t redirect_to_index(httpd_req_t *);

const httpd_uri_t actions_open_uri_handler = {
    .uri = CONFIG_ACTIONS_OPEN_URI "/?*",
    .method = HTTP_POST,
    .handler = &post_open_handler,
    .user_ctx = NULL,
};

const httpd_uri_t actions_close_uri_handler = {
    .uri = CONFIG_ACTIONS_CLOSE_URI "/?*",
    .method = HTTP_POST,
    .handler = &post_close_handler,
    .user_ctx = NULL,
};

const httpd_uri_t actions_stop_uri_handler = {
    .uri = CONFIG_ACTIONS_STOP_URI "/?*",
    .method = HTTP_POST,
    .handler = &post_stop_handler,
    .user_ctx = NULL,
};

static esp_err_t post_open_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received request at \"%s\"", req->uri);

    if (!strcmp(req->uri, CONFIG_ACTIONS_OPEN_URI) || !strcmp(req->uri, CONFIG_ACTIONS_OPEN_URI "/"))
    {
        controller_open_all();
        return redirect_to_index(req);
    }

    uint8_t channel;

    esp_err_t err = parse_channel(req->uri + strlen(CONFIG_ACTIONS_OPEN_URI "/"), &channel);
    if (err != ESP_OK)
        return err;

    controller_open(channel);
    return redirect_to_index(req);
}

static esp_err_t post_close_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received request at \"%s\"", req->uri);

    if (!strcmp(req->uri, CONFIG_ACTIONS_CLOSE_URI) || !strcmp(req->uri, CONFIG_ACTIONS_CLOSE_URI "/"))
    {
        controller_close_all();
        return redirect_to_index(req);
    }

    uint8_t channel;

    esp_err_t err = parse_channel(req->uri + strlen(CONFIG_ACTIONS_CLOSE_URI "/"), &channel);
    if (err != ESP_OK)
        return err;

    controller_close(channel);
    return redirect_to_index(req);
}

static esp_err_t post_stop_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received request at \"%s\"", req->uri);

    if (!strcmp(req->uri, CONFIG_ACTIONS_STOP_URI) || !strcmp(req->uri, CONFIG_ACTIONS_STOP_URI "/"))
    {
        controller_stop_all();
        return redirect_to_index(req);
    }

    uint8_t channel;

    esp_err_t err = parse_channel(req->uri + strlen(CONFIG_ACTIONS_STOP_URI "/"), &channel);
    if (err != ESP_OK)
        return err;

    controller_stop(channel);
    return redirect_to_index(req);
}

static esp_err_t parse_channel(const char *str, uint8_t *channel_out)
{
    char *end;
    uint64_t channel = strtoul(str, &end, 10);

    if (channel > 255 || end == str || *end)
        return ESP_ERR_INVALID_ARG;

    *channel_out = (uint8_t)channel;
    return ESP_OK;
}

static esp_err_t redirect_to_index(httpd_req_t *req)
{
    esp_err_t err = httpd_resp_set_status(req, "303 See Other");
    if (err != ESP_OK)
        return err;

    err = httpd_resp_set_hdr(req, "Location", "/");
    if (err != ESP_OK)
        return err;

    err = httpd_resp_set_hdr(req, "Connection", "close");
    if (err != ESP_OK)
        return err;

    return httpd_resp_send(req, NULL, 0);
}
