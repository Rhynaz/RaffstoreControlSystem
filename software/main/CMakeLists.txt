idf_component_register(
    SRCS "src/main.c"
    PRIV_REQUIRES "network" "controller" "http" "update" "esp_event" "driver"
)
