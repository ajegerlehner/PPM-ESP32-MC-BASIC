#ifndef _MC_COMMON_H_
#define _MC_COMMON_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"

/* Variable Declaration */
#define WIFI_CONNECTED_BIT BIT0
#define ESP_WIFI_SSID CONFIG_WIFI_SSID
#define ESP_WIFI_PASS CONFIG_WIFI_PASSWORD
#define ESP_WIFI_MAXIMUM_RETRY CONFIG_WIFI_MAXIMUM_RETRY
#define ESP_HOST_ADDR CONFIG_MQTT_BROKER_IP_ADDRESS
#define ONE_SECOND_DELAY (1000 / portTICK_PERIOD_MS)

#define NVS_STORAGE_NS      "StoragePPM"
#define NVS_SSID_TAG        "SSID"
#define NVS_SSID_PWD_TAG    "SSIDPWD"

esp_err_t nvs_init();
esp_err_t wifi_init();
esp_err_t prepare_network();
esp_err_t nvs_read_str_value(const char *keyName, char* retString, size_t* retLength);
esp_err_t nvs_write_str_value(char *keyName, char* writeString);
esp_err_t nvs_read_uint32_value(char *keyName, uint32_t *ret_val);
esp_err_t nvs_write_uint32_value(char *keyName, uint32_t val);
void incrementMsgReceived();

#endif