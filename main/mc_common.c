/**
 * Name:            mc_comon.c
 * Description:     common functiuins to setup communications etc.
 * Initial Date:    2020-10-31
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include "nvs_flash.h"
#include "nvs.h"

#include "mc_common.h"
#include "mc_data.h"
#include "mc_sensor_data.h"
#include "mc_oled_display.h"
#include "mc_web_server.h"


#define TAG "COMMON"

// WiFi AP mode
#define SSID_AP         "PPM-MC"
#define SSID_AP_PWD     "12345678"
#define SSID_AP_MAXCONN 4
#define SSID_AP_CHANNEL 0

#define REBOOT_DELAY    10

//Declare static prototype
static esp_err_t esp32_event_handler(void *ctx, system_event_t *event);

//WIFI settings
static EventGroupHandle_t wifi_event_grp;
static int wifi_retry_num = 0;
static ip4_addr_t* esp32_ip_addr;

extern DeviceStatus deviceStatus;
extern SamplingMessage samplingMsg;
extern char *iotDeviceId;


/******************************************************************************
 * FunctionName : esp32_event_handler
 * Description  : Callback to handle base level events from the ESP32 
 *                System Start, WIFI, etc. 
 * Parameters   : *ctx (void) 
 *                *event (system_event_t)
 * Returns      : esp_err_t
*******************************************************************************/

static esp_err_t esp32_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) 
    {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "got ip:%s",
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            esp32_ip_addr = &event->event_info.got_ip.ip_info.ip;	
            wifi_retry_num = 0;
            xEventGroupSetBits(wifi_event_grp, WIFI_CONNECTED_BIT);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG,"Event State: Disconnected from WiFi");

            if (wifi_retry_num < ESP_WIFI_MAXIMUM_RETRY) 
            {
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_grp, WIFI_CONNECTED_BIT);
                wifi_retry_num++;
                ESP_LOGI(TAG,"Retry connecting to the WiFi AP");
            }
            else 
            {
                ESP_LOGE(TAG,"Connection to the WiFi AP failure\n");
            }
            break;
            

        default:
            break;
    }
    
    return ESP_OK;
}
/******************************************************************************
 * FunctionName : nvs_init
 * Description  : Initialize NVS
 * Parameters   : none
 * Returns      : esp_err_t
*******************************************************************************/
esp_err_t nvs_init()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS is initialized");

    return ESP_OK; 
}

/******************************************************************************
 * FunctionName : wifi_init
 * Description  : Initialize WiFI
 * Parameters   : none
 * Returns      : esp_err_t
*******************************************************************************/
esp_err_t wifi_init()
{
    char *ssid = NULL;
    char *ssid_pwd = NULL;
    char *node_name = NULL;
    esp_err_t ret;

    //Initialize WiFi
    wifi_event_grp = xEventGroupCreate();

    //tcpip_adapter_init();

    //ESP_ERROR_CHECK(esp_event_loop_init(esp32_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ret = nvs_read_str_value(NVS_NODENAME_TAG, &node_name);
    if (ret != ESP_OK || strlen(node_name) == 0)
    {
        enter_config_mode();

        // restart chip
        esp_restart();
    }

    ret = nvs_read_str_value(NVS_SSID_TAG, &ssid);
    if (ret != ESP_OK)
    {
        // wifi not configured, exit wifi_init
        return ESP_OK;

        //enter_config_mode();

        // restart chip
        //esp_restart();
    }
    
    ret = nvs_read_str_value(NVS_SSID_PWD_TAG, &ssid_pwd);
    if (ret != ESP_OK)
    {
        // wifi not configured, exit wifi_init
        return ESP_OK;

        //enter_config_mode();
        
        // restart chip
        //esp_restart();
    }
    
    wifi_config_t wifi_config = {
        .sta = {}
    };

    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, ssid_pwd, strlen(ssid_pwd));
    


    //Check for errors
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init finished.");
    ESP_LOGI(TAG, "Connected to WiFi AP-SSID:%s password:%s", ssid, ssid_pwd);
 
    return ESP_OK;
}

esp_err_t enter_config_mode()
{
    // no ssid and password was previously saved in nvs
    // enter WiFi AP mode to serve a web page to select WiFi
    // network
    char ipStr[16];

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SSID_AP,
            .password = SSID_AP_PWD,
            .max_connection = SSID_AP_MAXCONN,
            .channel = SSID_AP_CHANNEL,
            .ssid_hidden = 0,
            .authmode = WIFI_AUTH_OPEN
        }
    };

    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

    tcpip_adapter_ip_info_t ip_info;
    memset(&ip_info, 0, sizeof(ip_info));
    IP4_ADDR(&(ip_info.ip), 192,168,1,99);
    IP4_ADDR(&(ip_info.gw), 192,168,1,99);
    IP4_ADDR(&(ip_info.netmask), 255,255,255,0);
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ip_info));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    // setup web server to configure wifi settings
    register_endpoints();

    // display config ip adress on oled display
    sprintf(ipStr, IPSTR, IP2STR(&ip_info.ip));
    i2c_bus_init();
    oled_init();
    oled_wifi_config(ipStr, SSID_AP);

    while (1)
    {
        vTaskDelay(ONE_SECOND_DELAY * 10);
        printf("Waiting for WiFi config...\n");
    }
    return ESP_OK;
}

esp_err_t prepare_network()
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(esp32_event_handler, NULL));

    return ESP_OK;
}

esp_err_t nvs_read_str_value(const char *keyName, char **retString)
{
    nvs_handle_t nvsHandle;
    esp_err_t    ret;
    size_t strLength;

    ret = nvs_open(NVS_STORAGE_NS, NVS_READWRITE, &nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Read: Error opening NVS storage %s", NVS_STORAGE_NS);
        return ret;
    }

    // determine string length stored in nvs
    ret = nvs_get_str(nvsHandle, (const char *)keyName, NULL, &strLength);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading nvs key/value %s string length", keyName);
        nvs_close(nvsHandle);
        return ret;
    }

    // strLength including null terminator
    *retString = malloc(strLength);
    ret = nvs_get_str(nvsHandle, (const char *)keyName, *retString, &strLength);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading nvs key/value pair %s", keyName);
        nvs_close(nvsHandle);
        return ret;
    }

    ESP_LOGI(TAG, "Read key/value pair from NVS: %s / %s", keyName, *retString);

    nvs_close(nvsHandle);

    return ESP_OK;
}

esp_err_t nvs_read_uint32_value(char *keyName, uint32_t *ret_val)
{
    nvs_handle_t nvsHandle;
    esp_err_t    ret;

    ret = nvs_open(NVS_STORAGE_NS, NVS_READWRITE, &nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Read: Error opening NVS storage %s", NVS_STORAGE_NS);
        return ret;
    }

    ret = nvs_get_u32(nvsHandle, (const char *)keyName, ret_val);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading nvs key/value pair %s", keyName);
        nvs_close(nvsHandle);
        return ret;
    }

    nvs_close(nvsHandle);

    return ESP_OK;
}

esp_err_t nvs_write_str_value(char *keyName, char *writeString)
{
    nvs_handle_t nvsHandle;
    esp_err_t    ret;

    ret = nvs_open(NVS_STORAGE_NS, NVS_READWRITE, &nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Write: Error opening NVS storage %s", NVS_STORAGE_NS);
        return ret;
    }

    ret = nvs_set_str(nvsHandle, (const char *)keyName, writeString);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Write: Error writing keyValue %s:%s", keyName, writeString);
        nvs_close(nvsHandle);
        return ret;
    }

    ret = nvs_commit(nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Write: Error commiting NVS write %s:%s.", keyName, writeString);
        return ret;
    }

    nvs_close(nvsHandle);
    return ESP_OK;
}

esp_err_t nvs_write_uint32_value(char *keyName, uint32_t val)
{
    nvs_handle_t nvsHandle;
    esp_err_t    ret;

    ret = nvs_open(NVS_STORAGE_NS, NVS_READWRITE, &nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Write: Error opening NVS storage %s", NVS_STORAGE_NS);
        return ret;
    }

    ret = nvs_set_u32(nvsHandle, (const char *)keyName, val);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Write: Error writing keyValue %s:%d", keyName, val);
        nvs_close(nvsHandle);
        return ret;
    }

    ret = nvs_commit(nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Write: Error commiting NVS write %s:%d.", keyName, val);
        return ret;
    }

    nvs_close(nvsHandle);
    return ESP_OK;
}

esp_err_t nvs_delete_key(char *keyName)
{
    nvs_handle_t nvsHandle;
    esp_err_t    ret;

    ret = nvs_open(NVS_STORAGE_NS, NVS_READWRITE, &nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Erase: Error opening NVS storage %s", NVS_STORAGE_NS);
        return ret;
    }

    ret = nvs_erase_key(nvsHandle, (const char *)keyName);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Erase: Error erasing keyValue %s", keyName);
        nvs_close(nvsHandle);
        return ret;
    }

    ret = nvs_commit(nvsHandle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS Erase: Error commiting erase operation for keyValue %s", keyName);
        nvs_close(nvsHandle);
        return ret;
    }

    nvs_close(nvsHandle);

    ESP_LOGI(TAG, "NVS key %s successfully erased.", keyName);

    return ESP_OK;
}

esp_err_t init_iotDeviceId()
{
    esp_err_t ret;
    
    // read iotDeviceId from nvs flash storage
    ret = nvs_read_str_value(NVS_NODENAME_TAG, &iotDeviceId);
    if (ret != ESP_OK)
    {
        wifi_init();
        enter_config_mode();

        // restart chip
        esp_restart();
    }

    return ESP_OK;
}

esp_err_t nvs_read_spi_flash_size(char *flashSize)
{
    esp_err_t ret;
    
    // read spi flash size from nvs flash storage
    ret = nvs_read_str_value(CONFIG_ESPTOOLPY_FLASHSIZE, flashSize);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading SPI flash size from config.");
    }

    return ESP_OK;
}

void incrementMsgReceived()
{
    deviceStatus.azureMsgReceived++;
}

void spawn_reboot_task(RebootStruct *reboot)
{
    BaseType_t taskResult;

    // restart chip
    // spawn reboot task
    taskResult = xTaskCreate(&reboot_task, "reboot_task", 1024, reboot, 9, NULL); 
    if (taskResult != pdPASS)
    {   
        ESP_LOGE(TAG, "Create reboot task failed"); 
    }

     return ESP_OK;
}

void reboot_task(void *pvParameter)
{
    RebootStruct *reboot = (RebootStruct *)pvParameter;
    

    while(1)
    {
        ESP_LOGW(TAG, "Rebooting chip in %d seconds.", reboot->rebootDelay);        
        vTaskDelay(ONE_SECOND_DELAY * reboot->rebootDelay);
        esp_restart();
    }
    
    vTaskDelete(NULL);  
}
