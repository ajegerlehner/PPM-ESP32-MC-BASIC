#include "mc_common.h"
#include "mc_data.h"
#include "mc_sensor_data.h"
#include "nvs_flash.h"
#include "nvs.h"

#define TAG "COMMON"

//Declare static prototype
static esp_err_t esp32_event_handler(void *ctx, system_event_t *event);

//WIFI settings
static EventGroupHandle_t wifi_event_grp;
static int wifi_retry_num = 0;
static ip4_addr_t* esp32_ip_addr;

extern DeviceStatus deviceStatus;
extern SamplingMessage samplingMsg;


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
                ESP_LOGI(TAG,"Connection to the WiFi AP failure\n");
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
    uint8_t *ssid = NULL;
    uint8_t *ssid_pwd = NULL;
    esp_err_t ret;
    size_t  strLen;  
    char statusTxt[100];  

    //Initialize WiFi
    wifi_event_grp = xEventGroupCreate();

    //tcpip_adapter_init();

    //ESP_ERROR_CHECK(esp_event_loop_init(esp32_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ret = nvs_read_str_value(NVS_SSID_TAG, (char *)ssid, &strLen);
    if (ret != ESP_OK)
    {
        // enter AP mode
        //return ESP_OK;
        nvs_write_str_value(NVS_SSID_TAG, "devolo-444");
        nvs_read_str_value(NVS_SSID_TAG, (char *)ssid, &strLen);
    }

    ret = nvs_read_str_value(NVS_SSID_PWD_TAG, (char*)ssid_pwd, &strLen);
    if (ret != ESP_OK)
    {
        // enter AP mode
        //return ESP_OK;
        nvs_write_str_value(NVS_SSID_PWD_TAG, "BGTXKDVWQGNQOJLH");
        nvs_read_str_value(NVS_SSID_PWD_TAG, (char *)ssid_pwd, &strLen);
    }
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS
        },
    };   

    //Check for errors
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init finished.");
    ESP_LOGI(TAG, "Connected to WiFi AP-SSID:%s password:%s",
             ssid, ssid_pwd);

    if (ssid != NULL && ssid_pwd != NULL)
    {
    sprintf(statusTxt, "WiFi: %s, %s", ssid, ssid_pwd);
    create_status_msg(&deviceStatus, &samplingMsg, statusTxt);         
    }
 
    free(ssid);
    free(ssid_pwd);
    return ESP_OK;
}

esp_err_t prepare_network()
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(esp32_event_handler, NULL));

    return ESP_OK;
}

esp_err_t nvs_read_str_value(const char *keyName, char *retString, size_t* retLength)
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
        ESP_LOGE(TAG, "Error reading nvs key/value pair string length%s", keyName);
        nvs_close(nvsHandle);
        return ret;
    }

    // strLength including null terminator
    retString = malloc(strLength);
    ret = nvs_get_str(nvsHandle, (const char *)keyName, retString, &strLength);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading nvs key/value pair %s", keyName);
        nvs_close(nvsHandle);
        return ret;
    }

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

void incrementMsgReceived()
{
    deviceStatus.azureMsgReceived++;
}