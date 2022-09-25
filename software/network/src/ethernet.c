#include "network/ethernet.h"

#include "config.h"

#include "esp_log.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "driver/spi_master.h"

static const char *const TAG = "Network    : Ethernet ";

static esp_eth_handle_t eth_handle;
static volatile bool is_running = false;

static void ethernet_handler(void *, esp_event_base_t, int32_t, void *);

void ethernet_init()
{
    ESP_LOGI(TAG, "Initialize SPI bus.");
    spi_bus_config_t spi_bus_cfg = {
        .miso_io_num = CONFIG_ETHERNET_SPI_PIN_MISO,
        .mosi_io_num = CONFIG_ETHERNET_SPI_PIN_MOSI,
        .sclk_io_num = CONFIG_ETHERNET_SPI_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_ETHERNET_SPI_HOST, &spi_bus_cfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Initialize SPI device.");
    spi_device_interface_config_t spi_dev_cfg = {
        .command_bits = 16, // address phase in W5500 frame
        .address_bits = 8,  // control phase in W5500 frame
        .mode = 0,
        .clock_speed_hz = CONFIG_ETHERNET_SPI_CLOCK_MHZ * 1000000,
        .spics_io_num = CONFIG_ETHERNET_SPI_PIN_CS,
        .queue_size = 20,
    };

    spi_device_handle_t spi_dev_handle;
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_ETHERNET_SPI_HOST, &spi_dev_cfg, &spi_dev_handle));

    ESP_LOGI(TAG, "Install Ethernet driver.");
    eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
    eth_w5500_config_t w5500_cfg = ETH_W5500_DEFAULT_CONFIG(spi_dev_handle);
    w5500_cfg.int_gpio_num = CONFIG_ETHERNET_SPI_PIN_INT;
    esp_eth_mac_t *eth_mac = esp_eth_mac_new_w5500(&w5500_cfg, &mac_cfg);

    eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
    phy_cfg.reset_gpio_num = CONFIG_ETHERNET_SPI_PIN_RST;
    esp_eth_phy_t *eth_phy = esp_eth_phy_new_w5500(&phy_cfg);

    esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(eth_mac, eth_phy);
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_cfg, &eth_handle));

    ESP_LOGI(TAG, "Set MAC address.");
    uint8_t mac_addr[8];
    ESP_ERROR_CHECK(esp_read_mac(mac_addr, CONFIG_ETHERNET_MAC_ADDRESS));
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr));

    ESP_LOGI(TAG, "Attach driver to network stack.");
    esp_netif_inherent_config_t netif_base_cfg = ESP_NETIF_INHERENT_DEFAULT_ETH();
    esp_netif_config_t netif_cfg = {
        .base = &netif_base_cfg,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH,
    };

    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &ethernet_handler, NULL, NULL));
}

void ethernet_start()
{
    if (!is_running)
    {
        ESP_LOGI(TAG, "Starting...");
        esp_eth_start(eth_handle);
        is_running = true;
    }
}

void ethernet_stop()
{
    if (is_running)
    {
        ESP_LOGI(TAG, "Stopping...");
        esp_eth_stop(eth_handle);
        is_running = false;
    }
}

static void ethernet_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    switch (id)
    {
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Started!");
        break;

    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Stopped!");
        break;

    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected!");
        break;

    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnected!");
        break;

    default:
        break;
    }
}
