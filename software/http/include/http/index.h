#pragma once

#include "esp_http_server.h"

#define INDEX_DEFINE_CHANNEL(num) "\
        <form method='post'>\
            <label style='line-height:1.5'>Channel " #num ":</label>\
            <input type='submit' value='Open' formaction='/actions/open/" #num "' />\
            <input type='submit' value='Close' formaction='/actions/close/" #num "' />\
            <input type='submit' value='Stop' formaction='/actions/stop/" #num "' />\
        </form>"

extern const httpd_uri_t index_uri_handler;
