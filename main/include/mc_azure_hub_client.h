#ifndef _MC_AZURE_HUB_CLIENT_H_
#define _MC_AZURE_HUB_CLIENT_H_

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/platform.h"
#include "iothub_client_options.h"
#include "parson.h"
#include "sdkconfig.h"
#include "iothub_client_ll.h"
#include "certs.h"

#include "esp_log.h"

#include "iothub_client_version.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/shared_util_options.h"

#include "azure_prov_client/prov_transport_mqtt_client.h"

typedef struct AzDeviceTwin
{
    char *currentSoftwareVersion;
    char *desiredSoftwareVersion;
    char *softwareFilePath;
}  DeviceTwin;



IOTHUB_DEVICE_CLIENT_LL_HANDLE init_iothub_client(char *deviceId);
int send_azure_msgs(char *msgToSend, IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient);
void process_azure_msgs(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient);
void destroy_iothub_client(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient);

char* deviceTwinToJSON(DeviceTwin* twin);
DeviceTwin* deviceTwinfromJSON(const char* jsonStr, DEVICE_TWIN_UPDATE_STATE updateState);
void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback);
void reportedStateCallback(int statusCode, void* userContextCallback);
void setupDeviceTwin(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient);
void setupDeviceCallbackMethods(IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_deviceclient);
int deviceMethodCallback(const char* method_name, const unsigned char* payload, size_t size, unsigned char** response, size_t* response_size, void* userContextCallback);
IOTHUBMESSAGE_DISPOSITION_RESULT deviceMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback);

#endif