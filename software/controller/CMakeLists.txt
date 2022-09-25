idf_component_register(
    SRCS "src/controller.c" "src/channel.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_event"
    PRIV_REQUIRES "config" "driver"
)
