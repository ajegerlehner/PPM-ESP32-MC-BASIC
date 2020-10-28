#ifndef _MC_DATA_H_
#define _MC_DATA_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "parson.h"

#define AZURE_MSG_SIZE_MAX  1000

typedef struct IOT_DEVICE_DATA
{
    char* chip_type;
    uint32_t free_heap_size;
    float temperature;
    float humidity;
    float luminance;
} DeviceData;

typedef struct SAMPLING_MESSAGE
{
    long msgNum;
    char *iotDeviceId;
    char *msgType;
    time_t timeStamp;
    float speed;
    float elapseTime;
    float temperature;
    float humidity;
    float pressure;
    float altitude;
    float rtd_temperature;    
    uint8_t timeType;
    long tickCount1;
    long tickCount2;   
    long tickCount3;
    long tickCount4;   
    long tickCount5;
    long tickCount6;   
    long counterDelta1;
    long counterDelta2;
    long counterDelta3;
    long counterDelta4;
    long counterDelta5;
    long counterDelta6;
    uint8_t logicLevel1;
    uint8_t logicLevel2;
    uint8_t logicLevel3;
    uint8_t logicLevel4;
    uint8_t logicLevel5;
    uint8_t logicLevel6;
    float maxAmplitudeX;
    float frequencyMaxX;
    float maxAmplitudeY;
    float frequencyMaxY;
    float maxAmplitudeZ;
    float frequencyMaxZ;
    long freeHeapSize;
} SamplingMessage;

typedef struct DEVICE_STATUS
{
    long msgNum;    
    char* iotDeviceId;
    char* msgType;
    time_t timeStamp;
    time_t lastBootTime;
    long upTime;
    long azureMsgSent;
    long azureMsgReceived;
    char* lastCloudToDeviceMsg;
    char* statusMsg;
    char* appVersion;
    char* serialNo;
    int hwRevision;
    int rtcBatteryLow;
    float processorTemperature;
    long heapSize;
    time_t lastOTAUpdate;

} DeviceStatus;

typedef struct COUNTER_READING
{
    uint32_t counter0;
    uint32_t counter1;
    uint32_t counter2;
    uint32_t counter3;
    uint32_t counter4;
    uint32_t counter5;
    uint32_t delta0;
    uint32_t delta1;
    uint32_t delta2;
    uint32_t delta3;
    uint32_t delta4;
    uint32_t delta5;
    uint32_t elapsedMS;
    uint8_t ttl0;
    uint8_t ttl1;
    uint8_t ttl2;
    uint8_t ttl3;
    uint8_t ttl4;
    uint8_t ttl5;
} CounterReading;

typedef struct BME280_READING
{
    float temperature;
    float humidity;
    float pressure;
    float altitude;
} BME280Reading;

typedef enum TIME_TYPE
{
    Stopped,
    Setuptime,
    Runtime,
    Downtime
} TimeType;

static const char *timeTypeStr[] = {
    "Stopped",
    "Setuptime",
    "Runtime",
    "Downtime"
};

int serializeSamplingMessage(SamplingMessage* msg, char* jsonString);
int serializeStatusgMessage(DeviceStatus *status, char *jsonString);
void deserializeCounterReading(char *jsonStr, CounterReading *reading);

#endif