/**
 * Name:            mc_device.c
 * Description:     Manages device related data
 * Initial Date:    2020-06-07
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_log.h"
#include "cJSON.h"

#include "mc_device.h"

#define TAG "DEV CONFIG"


char * assign_json_tag_str(cJSON *pl, const char * tag_name);
float assign_json_tag_float(cJSON *pl, const char * tag_name);
uint16_t assign_json_tag_uint16(cJSON *pl, const char * tag_name);
uint32_t assign_json_tag_uint32(cJSON *pl, const char * tag_name);
uint8_t assign_json_tag_uint8(cJSON *pl, const char * tag_name);

extern DeviceConfig deviceConfig;

void deserializeDeviceConfig(char *jsonStr)
{
    cJSON *payload = cJSON_Parse(jsonStr);

    ESP_LOGI(TAG, "Deserializing device config %s\n", jsonStr);

    // deserialize strings
    deviceConfig.iotDeviceId = assign_json_tag_str(payload, "iotDeviceId");
    deviceConfig.deviceConnectionString = assign_json_tag_str(payload, "deviceConnectionString");

    // deserialize double
    deviceConfig.speedFactor = assign_json_tag_float(payload, "speedFactor");
    deviceConfig.counterFactor = assign_json_tag_float(payload, "counterFactor");
    deviceConfig.setuptimeRate = assign_json_tag_float(payload, "setuptimeRate");
    deviceConfig.runtimeRate = assign_json_tag_float(payload, "runtimeRate");

    // deserialize uint16
    deviceConfig.deviceToCloudInterval = assign_json_tag_uint16(payload, "deviceToCloudInterval");
    deviceConfig.cloudToDeviceInterval = assign_json_tag_uint16(payload, "cloudToDeviceInterval");
    deviceConfig.stoppedTimeFence = assign_json_tag_uint16(payload, "stoppedTimeFence");
    deviceConfig.downtimeTimeFence = assign_json_tag_uint16(payload, "downTimeTimeFence");
    deviceConfig.setuptimeTimeFence = assign_json_tag_uint16(payload, "setuptimeTimefence");
    deviceConfig.runtimeTimeFence = assign_json_tag_uint16(payload, "runtimeTimeFence");
    deviceConfig.watchdogTimeFence = assign_json_tag_uint16(payload, "watchDogTimer");
    deviceConfig.lastTimeType = assign_json_tag_uint16(payload, "lastTimeType");
    deviceConfig.dayShiftEndSec = assign_json_tag_uint16(payload, "DayShiftEndSec");    
    

    // clean up memory
    cJSON_Delete(payload);
}

char * assign_json_tag_str(cJSON *pl, const char * tag_name)
{
    char *ret;
    cJSON *jsonTag = cJSON_GetObjectItem(pl, tag_name);

    ret = (char *) malloc(strlen(jsonTag->valuestring)+1);
    ESP_LOGW(TAG,"malloc assign_json_tag_str"); //todo
    memcpy(ret, jsonTag->valuestring, strlen(jsonTag->valuestring));
    //memcpy(&ret[strlen(jsonTag->valuestring)+1], "\0",1);
    memcpy(&ret[strlen(jsonTag->valuestring)], "\0",1);
    
    return ret;
}

float assign_json_tag_float(cJSON *pl, const char * tag_name)
{
    double ret;
    cJSON *jsonTag = cJSON_GetObjectItem(pl, tag_name);

    ret = jsonTag->valuedouble;

    
    return (float) ret;
}

uint16_t assign_json_tag_uint16(cJSON *pl, const char * tag_name)
{
    uint16_t ret;
    cJSON *jsonTag = cJSON_GetObjectItem(pl, tag_name);

    ret = jsonTag->valueint;

    
    return ret;
}

uint8_t assign_json_tag_uint8(cJSON *pl, const char * tag_name)
{
    uint8_t ret;

    ESP_LOGI(TAG, "Deserializing uint8 %s", tag_name);

    cJSON *jsonTag = cJSON_GetObjectItem(pl, tag_name);

    if (jsonTag == NULL)
    {
        ESP_LOGE(TAG, "Invalid JSON tag. Cannot convert to uint8.");
        return 0;
    }

    ret = jsonTag->valueint;

    
    return ret;
}

uint32_t assign_json_tag_uint32(cJSON *pl, const char * tag_name)
{
    uint32_t ret;

     ESP_LOGI(TAG, "Deserializing uint32 %s", tag_name);

    cJSON *jsonTag = cJSON_GetObjectItem(pl, tag_name);

    if (jsonTag == NULL)
    {
        ESP_LOGE(TAG, "Invalid JSON tag. Cannot convert tu UINT32.");
        return 0;
    }

    ret = (uint32_t) jsonTag->valuedouble;

    
    return ret;
}

DeviceConfig get_device_config_data()
{
    return deviceConfig;
}
