#ifndef _MC_DEVICE_H_
#define _MC_DEVICE_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "parson.h"


enum performanceUnit {
    metersPerMinute ,
    sheetsPerHour
};

typedef struct IOT_DEVICE_CONFIG
{
    char* iotDeviceId;
    char * deviceConnectionString;
    uint16_t deviceToCloudInterval;
    uint16_t cloudToDeviceInterval;
    float speedFactor;
    float counterFactor;
    uint16_t stoppedTimeFence;
    float setuptimeRate;
    uint16_t setuptimeTimeFence;
    float runtimeRate;
    uint16_t runtimeTimeFence;
    uint16_t watchdogTimeFence;
    uint16_t downtimeTimeFence;
    uint16_t lastTimeType;
} DeviceConfig;



enum counterConfig {
    Counter1,
    Counter12
};


void deserializeDeviceConfig(char *jsonStr);
DeviceConfig get_device_config_data();

#endif
