idf_component_register(
    SRCS "src/ethernet.c" "src/network.c" "src/wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_wifi"
    PRIV_REQUIRES "config" "esp_event" "esp_eth" "driver" "esp_netif"
)
