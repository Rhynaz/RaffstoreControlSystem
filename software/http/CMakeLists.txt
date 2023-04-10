idf_component_register(
    SRCS "src/actions.c" "src/flash.c" "src/http.c" "src/index.c" "src/status.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_http_server"
    PRIV_REQUIRES "config" "controller" "network" "update"
)
