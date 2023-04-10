#include "http/index.h"

#include "config.h"

#include "esp_http_server.h"
#include "esp_log.h"

#define INDEX_DEFINE_CHANNEL(num) "\
        <form method='post'>\
            <label style='line-height:1.5'>Channel " #num ":</label>\
            <input type='submit' value='Open' formaction='/actions/open/" #num "' />\
            <input type='submit' value='Close' formaction='/actions/close/" #num "' />\
            <input type='submit' value='Stop' formaction='/actions/stop/" #num "' />\
        </form>"

static const char *const TAG = "HTTP       : Index    ";

// clang-format off
static const char *const INDEX_BODY = "\
<!DOCTYPE html>\
<html lang='en'>\
    <head>\
        <meta charset='UTF-8'>\
        <title>" CONFIG_INDEX_TITLE "</title>\
    </head>\
    <body>\
        <h1>Raffstore Control System</h1>\
        <p>Version: " CONFIG_APP_PROJECT_VER "</p>\
        <p>Profile: " CONFIG_INDEX_TITLE "</p>"
#if CONFIG_CONTROLLER_CHANNEL_NUM > 0
        "\
        <h2>Controls</h2>\
        <form method='post'>\
            <label style='line-height:1.5'>All Channels:</label>\
            <input type='submit' value='Open' formaction='/actions/open' />\
            <input type='submit' value='Close' formaction='/actions/close' />\
            <input type='submit' value='Stop' formaction='/actions/stop' />\
        </form>\
        <br>"
        INDEX_DEFINE_CHANNEL(0)
#endif
#if CONFIG_CONTROLLER_CHANNEL_NUM > 1
        INDEX_DEFINE_CHANNEL(1)
#endif
#if CONFIG_CONTROLLER_CHANNEL_NUM > 2
        INDEX_DEFINE_CHANNEL(2)
#endif
#if CONFIG_CONTROLLER_CHANNEL_NUM > 3
        INDEX_DEFINE_CHANNEL(3)
#endif
#if CONFIG_CONTROLLER_CHANNEL_NUM > 4
        INDEX_DEFINE_CHANNEL(4)
#endif
#if CONFIG_CONTROLLER_CHANNEL_NUM > 5
        INDEX_DEFINE_CHANNEL(5)
#endif
#if CONFIG_CONTROLLER_CHANNEL_NUM > 6
        INDEX_DEFINE_CHANNEL(6)
#endif
        "\
    </body>\
</html>";
// clang-format on

static esp_err_t get_index_handler(httpd_req_t *);

const httpd_uri_t index_uri_handler = {
    .uri = "/?",
    .method = HTTP_GET,
    .handler = &get_index_handler,
    .user_ctx = NULL,
};

static esp_err_t get_index_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Received request at \"%s\"", req->uri);

    esp_err_t err = httpd_resp_set_hdr(req, "Connection", "close");
    if (err != ESP_OK)
        return err;

    return httpd_resp_sendstr(req, INDEX_BODY);
}
