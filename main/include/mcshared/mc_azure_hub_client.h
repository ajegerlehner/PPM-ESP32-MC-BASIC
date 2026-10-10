#ifndef _MC_AZURE_HUB_CLIENT_H_
#define _MC_AZURE_HUB_CLIENT_H_

#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"

typedef struct AzDeviceTwin
{
    char *currentSoftwareVersion;
    char *desiredSoftwareVersion;
    char *softwareFilePath;
}  DeviceTwin;

// opaque handle of the Azure IoT Hub connection
typedef struct mc_iothub_client *mc_iothub_handle_t;

mc_iothub_handle_t init_iothub_client(char *deviceId);
int send_azure_msgs(char *msgToSend, mc_iothub_handle_t iothub_deviceclient);
void process_azure_msgs(mc_iothub_handle_t iothub_deviceclient);
void destroy_iothub_client(mc_iothub_handle_t iothub_deviceclient);

char* deviceTwinToJSON(DeviceTwin* twin);
DeviceTwin* deviceTwinfromJSON(const char* jsonStr, bool completeTwin);

#endif
