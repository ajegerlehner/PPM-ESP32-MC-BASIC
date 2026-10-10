/**
 * Name:            mc_eth.c
 * Description:     Driver for ethernet connection, using LAX8720A chip
 * Initial Date:    2020-09-05
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include "stdio.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "mc_eth.h"

#define TAG "PHY"

#define MAX_IPADDRESS_LEN 16

static esp_netif_t *eth_netif;

extern char* deviceIPAddress;
extern int internetConnected;
extern uint8_t wifiConnected;

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        ESP_LOGI(TAG, "Ethernet default event *****");
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    esp_err_t ret;

    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    // ethernet is preferred over wifi
    ESP_LOGI(TAG, "Setting default netif interface to ethernet");
    ret = esp_netif_set_default_netif(eth_netif);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Set Ethernet as default IF error: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Ethernet IP address: " IPSTR " Mask: " IPSTR " Gwy: " IPSTR, IP2STR(&ip_info->ip), IP2STR(&ip_info->netmask), IP2STR(&ip_info->gw));

    if (!deviceIPAddress)
    {
        deviceIPAddress = malloc(sizeof(char)* MAX_IPADDRESS_LEN);
        ESP_LOGW(TAG,"malloc deviceIPaddress got_IP_adress_handler"); //todo
        sprintf(deviceIPAddress, IPSTR, IP2STR(&ip_info->ip));
        internetConnected = 1;
        wifiConnected = 0;
    }
}

esp_err_t phy_init()
{
    ESP_LOGI(TAG, "Initializing ethernet adapter...");

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&netif_config);

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1; // CONFIG_EXAMPLE_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = 17; //CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;
    esp32_emac_config.smi_gpio.mdc_num = 23;
    esp32_emac_config.smi_gpio.mdio_num = 18;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));

    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    ESP_ERROR_CHECK(esp_netif_set_hostname(eth_netif, "PPM-ETHERNET"));

    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    ESP_LOGI(TAG, "Ethernet adapter initialized.");

    return ESP_OK;
}

char* getDeviceIPAddress()
{
    if (!deviceIPAddress)
    {
        deviceIPAddress = malloc(sizeof(char)* MAX_IPADDRESS_LEN);
        ESP_LOGW(TAG,"malloc getDeviceIPaddress()"); //todo
        sprintf(deviceIPAddress, "%15s", "UNKNOWN");
    }

    return deviceIPAddress;
}


