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
#include "tcpip_adapter.h"
#include "lwip/netif.h"
#include "lwip/ip4.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "mc_eth.h"

#define TAG "PHY"

#define MAX_IPADDRESS_LEN 16

esp_err_t set_default_if(tcpip_adapter_if_t tcpip_if);

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

esp_err_t set_default_if(tcpip_adapter_if_t tcpip_if)
{
    struct netif *_netif;
    esp_err_t ret = tcpip_adapter_get_netif(tcpip_if, (void **)&_netif);    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error setting default netif: %s\n", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Setting default netif interface to %s", _netif->hostname);
    netif_set_default(_netif);

    return ESP_OK;
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    esp_err_t ret;

    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;

    ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_ETH, "PPM-ETHERNET"));
    ret = set_default_if(TCPIP_ADAPTER_IF_ETH);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Set Ethernet as default IF error: %s", esp_err_to_name(ret));
        }
    if (event->ip_changed)
    {
        ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_ETH, "PPM-ETHERNET"));
        ret = set_default_if(TCPIP_ADAPTER_IF_ETH);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Set Ethernet as default IF error: %s", esp_err_to_name(ret));
        }
    }
    
    ESP_LOGI(TAG, "Ethernet IP address: " IPSTR " Mask: " IPSTR " Gwy: " IPSTR, IP2STR(&ip_info->ip), IP2STR(&ip_info->netmask), IP2STR(&ip_info->gw));

    if (!deviceIPAddress)
    {
        deviceIPAddress = malloc(sizeof(char)* MAX_IPADDRESS_LEN);
        sprintf(deviceIPAddress, IPSTR, IP2STR(&ip_info->ip));
        internetConnected = 1;     
        wifiConnected = 0; 
    }
}

esp_err_t phy_init()
{
    ESP_LOGI(TAG, "Initializing ethernet adapter...");
    // tcpip_adapter_init();    

    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(tcpip_adapter_set_default_eth_handlers());
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1; // CONFIG_EXAMPLE_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = 17; //CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;
    mac_config.smi_mdc_gpio_num = 23;
    mac_config.smi_mdio_gpio_num = 18;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));

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
        sprintf(deviceIPAddress, "%15s", "UNKNOWN");
    }

    return deviceIPAddress;
}


