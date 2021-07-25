#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mc_azure_hub_client.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "mc_common.h"
#include "mc_iotcentral.h"
#include "mc_device.h"
#include "mc_ota.h"

#define TAG "AUZURE_IOT"

DeviceTwin currentDeviceTwin;

IOTHUB_DEVICE_CLIENT_LL_HANDLE init_iothub_client(char *deviceId)
{
    DeviceConfig devConfig;

    // Initializing Iot Hub SDK
    int iotHubInitResult = IoTHub_Init();
    if (iotHubInitResult != 0)
    {
        ESP_LOGE(TAG,"AZURE INFO: Error initializing Azure IoT Client");
        return NULL;
    }

    // get the device config from ppm iot central and provide connection string etc. 
    get_device_config(deviceId);
    ESP_LOGI(TAG, "IoT Central device config loaded for device %s", deviceId);
    
    // get device configuration struct for connection string
    devConfig = get_device_config_data();
    ESP_LOGI(TAG, "Device connection string is: %s", devConfig.deviceConnectionString);
    
    // Creating IoT Hub Client Handle from Connection String
//    IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient = 
//                IoTHubDeviceClient_LL_CreateFromConnectionString(devConfig.deviceConnectionString, MQTT_Protocol);
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient = 
                IoTHubDeviceClient_LL_CreateFromConnectionString(devConfig.deviceConnectionString, MQTT_WebSocket_Protocol);
    
    // Setting IoT Hub Device Client 
    // Enable verbose logging (e.g., for debugging).
    bool traceOn = true;
    bool encodeOn = true;
    (void)IoTHubDeviceClient_LL_SetOption(iothub_deviceclient, OPTION_LOG_TRACE, &traceOn);
    (void)IoTHubDeviceClient_LL_SetOption(iothub_deviceclient, OPTION_AUTO_URL_ENCODE_DECODE, &encodeOn);

    return iothub_deviceclient;
}

int send_azure_msgs(char *msgToSend, IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient)
{
    IOTHUB_MESSAGE_HANDLE deviceMsgHandle;
    int azureSendMsgResult;

    if (iothub_deviceclient == NULL)
    {
        printf("ERROR: iothub_deviceclient is NULL!\n");
        printf("Check IoT Hub Device Connection String in Config\n");
        ESP_LOGE(TAG, "AZURE INFO: ERROR-IoT Hub Client Handle is NULL");
        return -1;
    }
    
    // Creating IoT Hub Message 
    // NOTE: Two ways to create message, Azure accepts string based input
    //deviceMsgHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)msgToSend, sizeof(msgToSend));
    deviceMsgHandle = IoTHubMessage_CreateFromString(msgToSend);
 
    if (deviceMsgHandle == NULL)
    {
        printf("ERROR: deviceMsgHandle is NULL! Create Message failed.\n");
        ESP_LOGE(TAG, "AZURE INFO: ERROR-Failed to Create Azure Message");
        return -1;
    }
    
    IoTHubMessage_SetContentEncodingSystemProperty(deviceMsgHandle, "utf-8");
    IoTHubMessage_SetContentTypeSystemProperty(deviceMsgHandle, "application/json");
    
    // Sending Message to IoT Hub
    azureSendMsgResult = IoTHubClient_LL_SendEventAsync(iothub_deviceclient, deviceMsgHandle, NULL, NULL);

    if (azureSendMsgResult != IOTHUB_CLIENT_OK)
    {
        printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\n");
        ESP_LOGE(TAG, "AZURE INFO: ERROR-Failed Azure Send Event");
        IoTHubMessage_Destroy(deviceMsgHandle);
        return -1;
    }
    
    time_t sendTime = time(NULL);
    printf("SUCCESS: IoTHubClient_LL_SendEventAsync accepted message for transmission to IoT Hub.\n");
    printf("Message sent at %s\n", asctime(gmtime(&sendTime)));
    ESP_LOGI(TAG, "AZURE INFO: Completed Message successfully accepted to Azure transmission");
    
    IoTHubMessage_Destroy(deviceMsgHandle);

    return 0;
}

void process_azure_msgs(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient)
{
    IOTHUB_CLIENT_STATUS status;

    ESP_LOGI(TAG, "AZURE INFO: Start Processing Azure Messages");
    printf("Do Work: Processing IoT Hub Send Message Requests %X\n", (int)iothub_deviceclient);
    while ((IoTHubClient_LL_GetSendStatus(iothub_deviceclient, &status) == IOTHUB_CLIENT_OK) 
                && (status == IOTHUB_CLIENT_SEND_STATUS_BUSY))
    {
        IoTHubClient_LL_DoWork(iothub_deviceclient);
    }
    
    ESP_LOGI(TAG, "AZURE INFO: Stop Processing Azure Messages");
}

void destroy_iothub_client(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient)
{
    ESP_LOGI(TAG, "AZURE INFO: Cleaning up - Removing Device Client");
    IoTHubClient_LL_Destroy(iothub_deviceclient);
    IoTHub_Deinit();
}

char* deviceTwinToJSON(DeviceTwin* twin)
{
    char* jsonStr;
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);

    // send only reported properties
    json_object_set_string(root_object, "firmwareVersion", twin->currentSoftwareVersion);

    jsonStr = json_serialize_to_string(root_value);

    return jsonStr;
}

DeviceTwin* deviceTwinfromJSON(const char* jsonStr, DEVICE_TWIN_UPDATE_STATE updateState)
{
    DeviceTwin* deviceTwin = malloc(sizeof(DeviceTwin));
    JSON_Value* root_value = NULL;
    JSON_Object* root_object = NULL;
    JSON_Value* firmwareVersion;
    JSON_Value* firmwareFilePath;

    if (deviceTwin == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for device twin.");
        return NULL;
    }

    memset(deviceTwin, 0, sizeof(DeviceTwin));

    root_value = json_parse_string(jsonStr);
    root_object = json_value_get_object(root_value);

    // get only desired properties
    if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
    {
        printf("Device Twin update complete...\n");
        firmwareVersion = json_object_dotget_value(root_object, "desired.firmware.firmwareVersion");
        firmwareFilePath = json_object_dotget_value(root_object, "desired.firmware.url");
    }
    else
    {
        printf("Device Twin update partial...\n");
        firmwareVersion = json_object_dotget_value(root_object, "firmware.firmwareVersion");        
        firmwareFilePath = json_object_dotget_value(root_object, "firmware.url");
    }

    if (firmwareVersion != NULL)
    {
        const char *data = json_value_get_string(firmwareVersion);
        deviceTwin->desiredSoftwareVersion = malloc(strlen(data)+1);
        if (deviceTwin->desiredSoftwareVersion != NULL)
        {
            strcpy(deviceTwin->desiredSoftwareVersion, data);        
        }
        else
        {
            ESP_LOGE(TAG, "Failed allocating memory for desired property desiredSoftwareVersion");
        }        
    }
    if (firmwareFilePath != NULL)
    {
        const char *data = json_value_get_string(firmwareFilePath);
        deviceTwin->softwareFilePath = malloc(strlen(data)+1);
        if (deviceTwin->softwareFilePath != NULL)
        {
            strcpy(deviceTwin->softwareFilePath, data);        
        }
        else
        {
            ESP_LOGE(TAG, "Failed allocating memory for desired property desiredSoftwareFilePath");
        }        
    }

    deviceTwin->currentSoftwareVersion = malloc(strlen(currentDeviceTwin.currentSoftwareVersion)+1);
    if (deviceTwin->currentSoftwareVersion != NULL)
    {
        strcpy(deviceTwin->currentSoftwareVersion, currentDeviceTwin.currentSoftwareVersion);
    }
    else
    {
        ESP_LOGE(TAG, "Failed allocating memory for desired property currentSoftwareVersion");
    }
    

    json_value_free(root_value);

    return deviceTwin;    
}

void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback)
{
    //(void)userContextCallback;
    DeviceTwin* newTwin = deviceTwinfromJSON((const char*) payload, updateState);

    if (newTwin->desiredSoftwareVersion == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse new twin JSON desired firmwareVersion. Skipping update.");
        return;
    }

    ESP_LOGI(TAG, "Device Twin callback. New twin firmwareVersion: %s, current firmwareVersion is %s",  newTwin->desiredSoftwareVersion, currentDeviceTwin.currentSoftwareVersion);

    if (newTwin == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse device twin from JSON.");
        return;
    }

    if (newTwin->desiredSoftwareVersion != NULL)
    {
        if (strcmp(currentDeviceTwin.currentSoftwareVersion, newTwin->desiredSoftwareVersion) != 0)
        {
            // new desired firmware version reveived, prepare for OTA update
            ESP_LOGI(TAG, "Reveiced new firmware version: %s, current version is %s.", newTwin->desiredSoftwareVersion, currentDeviceTwin.currentSoftwareVersion);
            
            // perform OTA update from Azure blob storage
            performOTAUpdate(newTwin);

            free(currentDeviceTwin.currentSoftwareVersion);
            currentDeviceTwin.currentSoftwareVersion = malloc(strlen(newTwin->desiredSoftwareVersion)+1);
            strcpy(currentDeviceTwin.currentSoftwareVersion, newTwin->desiredSoftwareVersion);
        }

        free(newTwin->currentSoftwareVersion);
        free(newTwin->desiredSoftwareVersion);
        free(newTwin->softwareFilePath);
        free(newTwin);
    }
}

void reportedStateCallback(int statusCode, void* userContextCallback)
{
    (void)userContextCallback;
    ESP_LOGI(TAG, "DeviceTwin reported properties update completed with status: %d. Reported firmware version: %s", statusCode, currentDeviceTwin.currentSoftwareVersion);
}

void setupDeviceTwin(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient)
{    
    const esp_app_desc_t *ota_app_desc;

    memset(&currentDeviceTwin, 0, sizeof(currentDeviceTwin));

    // get current app version from ota app description
    ota_app_desc = esp_ota_get_app_description();
    ESP_LOGI(TAG, "Setting up Azure Device Twin. App project is %s, App version is %s", ota_app_desc->project_name,  ota_app_desc->version);

    currentDeviceTwin.currentSoftwareVersion = malloc(strlen(ota_app_desc->version)+1);
    strcpy(currentDeviceTwin.currentSoftwareVersion, ota_app_desc->version); 

    char* reportedProperties = deviceTwinToJSON(&currentDeviceTwin);
    if (reportedProperties != NULL)
    {
        IoTHubDeviceClient_LL_SendReportedState(iothub_deviceclient, (const unsigned char*)reportedProperties, strlen(reportedProperties), reportedStateCallback, NULL);
        IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothub_deviceclient, deviceTwinCallback, &currentDeviceTwin);
    }
}

void setupDeviceCallbackMethods(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient)
{
    IOTHUB_CLIENT_RESULT iotHubResult;

    // register device client callback method
    iotHubResult = IoTHubDeviceClient_LL_SetDeviceMethodCallback(iothub_deviceclient, deviceMethodCallback, NULL);
    if (iotHubResult == IOTHUB_CLIENT_OK)
    {
        ESP_LOGI(TAG, "Registered IotHub device client method callback."); 
    }
    else
    {
        ESP_LOGE(TAG, "Error when registering the IotHub device client method callback: %d", iotHubResult);
    }
    
}

int deviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback)
{
    (void)userContextCallback;
    (void)payload;
    (void)size;

    int result;
    BaseType_t taskResult;
    RebootStruct reboot;

    if (strcmp("REBOOT", strupr((char*)method_name)) == 0)
    {
        const char deviceMethodResponse[] = "{ \"Response\": \"Rebooting chip.\" }";
        *response_size = sizeof(deviceMethodResponse)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
        result = 200;
        
        // spawn reboot task
        reboot.rebootDelay = 10;
        spawn_reboot_task(&reboot);
    }
    else if (strcmp("NVSCLEAR", strupr((char*)method_name)) == 0)
    {
        if (nvs_delete_key(NVS_SSID_TAG) == ESP_OK && 
            nvs_delete_key(NVS_SSID_PWD_TAG) == ESP_OK &&
            nvs_delete_key(NVS_NODENAME_TAG) == ESP_OK)
        {
            const char deviceMethodResponse[] = "{ \"Response\": \"WiFi keys erased.\" }";
            *response_size = sizeof(deviceMethodResponse)-1;
            *response = malloc(*response_size);
            (void)memcpy(*response, deviceMethodResponse, *response_size);
            result = 200;
            
            // spawn reboot task
            reboot.rebootDelay = 10;
            spawn_reboot_task(&reboot);
        }
        else
        {
            const char deviceMethodResponse[] = "{ \"Response\": \"Error erasing WiFi keys.\" }";
            *response_size = sizeof(deviceMethodResponse)-1;
            *response = malloc(*response_size);
            (void)memcpy(*response, deviceMethodResponse, *response_size);
            result = -1;
        }        
    }    
    else
    {
        // All other entries are ignored.
        const char deviceMethodResponse[] = "{ }";
        *response_size = sizeof(deviceMethodResponse)-1;
        *response = malloc(*response_size);
        (void)memcpy(*response, deviceMethodResponse, *response_size);
        result = -1;
    }

    return result;
}


IOTHUBMESSAGE_DISPOSITION_RESULT deviceMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{    
    const char* string_msg = NULL;
    const unsigned char* byteArrayBuffer;
    size_t bufferSize;

    // update device status
    incrementMsgReceived();

    if (message == NULL)
    {
        ESP_LOGE("MSGCB", "Message is NULL");
        return IOTHUBMESSAGE_REJECTED;
    }
        
    if (IoTHubMessage_GetContentType(message) == IOTHUBMESSAGE_STRING)
    {
        string_msg = IoTHubMessage_GetString(message);
        ESP_LOGI("MSGCALLBACK", "STRING Type: Message callback: %s", string_msg);
    }

    if (IoTHubMessage_GetContentType(message) == IOTHUBMESSAGE_BYTEARRAY)
    {
        IoTHubMessage_GetByteArray(message, &byteArrayBuffer, &bufferSize);
        string_msg = (const char *) byteArrayBuffer;
        ESP_LOGI("MSGCALLBACK", "BYTEARRAY Type: Message callback: %s", string_msg);        
    }
    
    
    if (string_msg == NULL)
    {
        ESP_LOGE(TAG, "Error receiving message callback.");
        return IOTHUBMESSAGE_REJECTED;
    }

    return IOTHUBMESSAGE_REJECTED;
}

