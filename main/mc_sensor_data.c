/**
 * Name:            mc_sensor_data.c
 * Description:     Functions to collect all sensor data
 * Initial Date:    2020-08-10
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include "string.h"
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "mc_sensor_data.h"
#include "mc_data.h"
#include "mc_stm32f04.h"
#include "mc_mcp23071.h"
#include "mc_gpio.h"
#include "mc_device.h"
#include "mc_i2c_bus.h"
#include "iot_bme280.h"
#include "mc_ds3231.h"
#include "mc_iis328dq.h"
#include "mc_common.h"

#define TAG "SENSOR"
#define BME280_DELAY    300
#define MAX_STATUS_LEN  100
#define MAX_SERIAL_LEN  30

const float BME280_SEALEVEL_PRESSURE = 1014.7f;

static bme280_handle_t dev = NULL;
static time_t lastTimeTypeCange;
static TimeType currentTimeType;

void init_sensor_hardware()
{
    esp_err_t ret;

    // initialize BME280 sesnor
    dev = iot_bme280_create(i2c_bus, BME280_I2C_ADDRESS_DEFAULT);
    ret = iot_bme280_init(dev);
    if (ret == ESP_OK)
    {
        mcp23017_set_pin(MCP23017_ENVS_PIN, 1);
        ESP_LOGI(TAG, "BME280 initialized");
    }
    else
    {
        mcp23017_set_pin(MCP23017_ENVS_PIN, 0);
        ESP_LOGE(TAG, "BME280 failed to initialize: return code %d", ret);
    }    

    // initialize IIS328DQ accelerometer / vibration sensor
    iis328dq_init();
    
    // initilize VL6180X gesture / proximity sensor

}

void init_sensor_data(SamplingMessage *msg, char *deviceId, char *msgType)
{
    msg->iotDeviceId = malloc(strlen(deviceId));
    strcpy(msg->iotDeviceId, deviceId);

    msg->msgType = malloc(strlen(msgType));
    strcpy(msg->msgType, msgType);        
}

void init_devicve_status(DeviceStatus *status, char *deviceId, char *msgType)
{
    status->iotDeviceId = malloc(strlen(deviceId));
    strcpy(status->iotDeviceId, deviceId);

    status->msgType = malloc(strlen(msgType));
    strcpy(status->msgType, msgType);

    status->statusMsg = malloc(MAX_STATUS_LEN);
    status->serialNo = malloc(MAX_SERIAL_LEN);
}

void create_status_msg(DeviceStatus *status, SamplingMessage *samplingMsg, char *statusTxt)
{
    time_t now;
    uint32_t otaTime;
    uint32_t freeHeapSize;
    uint8_t macAdress[6];
    const esp_app_desc_t *ota_app_desc;


    // get current app version from ota app description
    ota_app_desc = esp_ota_get_app_description();

    time(&now);
    nvs_read_uint32_value("lastOTAUpdate", &otaTime);
    status->timeStamp = now;
    status->msgNum++;
    status->upTime = esp_timer_get_time()/ 1000 / 1000;
    status->azureMsgSent = samplingMsg->msgNum;
    status->hwRevision = stm32_hw_revision_uart();  
    status->appVersion = ota_app_desc->version;
    status->lastOTAUpdate = otaTime;
    
    // serial number is MAC adress of the esp32 chip
    esp_read_mac(macAdress, 0);
    sprintf(status->serialNo, "%x:%x:%x:%x:%x:%x", macAdress[0], macAdress[1], macAdress[2], macAdress[3], macAdress[4], macAdress[5]);
    
    strncpy(status->statusMsg, statusTxt, MAX_STATUS_LEN-1);

    freeHeapSize = esp_get_free_heap_size();
    status->heapSize = freeHeapSize;

    status->processorTemperature = ds3231_get_temperature();

    status->rtcBatteryLow = !ds3231_get_rtcBatteryStatus();
    signal_bat_low_status(status->rtcBatteryLow);
}

void signal_bat_low_status(bool _batLow)
{
    if (_batLow)
    {
        mcp23017_set_pin(MCP23017_LOWBAT_RED, 1);
        mcp23017_set_pin(MCP23017_LOWBAT_GREEN, 0);
    }
    else
    {
        mcp23017_set_pin(MCP23017_LOWBAT_RED, 0);
        mcp23017_set_pin(MCP23017_LOWBAT_GREEN, 1);            
    }
    
}

void read_sensor_data(SamplingMessage *msg, DeviceConfig devConfig)
{
    CounterReading currentReading;
    time_t now;
    uint32_t freeHeapSize;

    // get counter readings in sampling message
    //signal_pin(MC328_LED_PIN);
    //mcp23017_signal_pin(MCP23017_MC_PIN, SIGNAL_PIN_DELAY);
    mcp230xx_signal_pin(MCP23017_MC_PIN, MCP23008_MC_PIN, SIGNAL_PIN_DELAY);

    time(&now);
    msg->timeStamp      = now;

    stm32_read_counters_uart(&currentReading);

    msg->msgNum++;

    // absolute counters
    msg->tickCount1     = currentReading.counter0;
    msg->tickCount2     = currentReading.counter1;
    msg->tickCount3     = currentReading.counter2;
    msg->tickCount4     = currentReading.counter3;
    msg->tickCount5     = currentReading.counter4;
    msg->tickCount6     = currentReading.counter5;

    // delta counts
    msg->counterDelta1  = currentReading.delta0;
    msg->counterDelta2  = currentReading.delta1;
    msg->counterDelta3  = currentReading.delta2;
    msg->counterDelta4  = currentReading.delta3;
    msg->counterDelta5  = currentReading.delta4;
    msg->counterDelta6  = currentReading.delta5;

    msg->elapseTime     = currentReading.elapsedMS;

    msg->logicLevel1    = currentReading.ttl0;
    msg->logicLevel2    = currentReading.ttl1;
    msg->logicLevel3    = currentReading.ttl2;
    msg->logicLevel4    = currentReading.ttl3;
    msg->logicLevel5    = currentReading.ttl4;
    msg->logicLevel6    = currentReading.ttl5;

    msg->speed          = calc_speed(msg->counterDelta1, msg->elapseTime, devConfig.speedFactor);

    msg->timeType       = calc_timeType(msg->speed, devConfig);

    if (msg->timeType == Downtime)
    {
        mcp23008_set_pin(MCP23008_PLCOUT_PIN, 1);
        mcp23017_set_pin(MCP23017_PLCOUT_PIN, 1);
    }
    else
    {
        mcp23008_set_pin(MCP23008_PLCOUT_PIN, 0);
        mcp23017_set_pin(MCP23017_PLCOUT_PIN, 0);
    }
    
    
    read_bme280_data(msg);

    iis328dq_read_data(msg);

     freeHeapSize = esp_get_free_heap_size();
     msg->freeHeapSize = freeHeapSize;

     msg->rtd_temperature = ds3231_get_temperature();

}

void read_bme280_data(SamplingMessage *msg)
{
    msg->temperature = iot_bme280_read_temperature(dev);
    vTaskDelay(BME280_DELAY / portTICK_RATE_MS);

    msg->humidity = iot_bme280_read_humidity(dev);
    vTaskDelay(BME280_DELAY / portTICK_RATE_MS);

    msg->pressure = iot_bme280_read_pressure(dev);
    vTaskDelay(BME280_DELAY / portTICK_RATE_MS);

    msg->altitude = iot_bme280_read_altitude(dev, BME280_SEALEVEL_PRESSURE);
    vTaskDelay(BME280_DELAY / portTICK_RATE_MS);
}

float calc_speed(long counterDelta, long elapsedMS, float speedFactor)
{
    float retVal;

    ESP_LOGI(TAG, "Speed calculation: counterDelta: %ld, elapsedMS: %ld, speedFactor: %f", counterDelta, elapsedMS, speedFactor);

    retVal = 3600.0f / elapsedMS * 1000.0f * counterDelta * speedFactor; 
    return retVal;
}

TimeType calc_timeType(float speed, DeviceConfig devCfg)
{
    int timeTypeDuration;
    time_t currentTime;

    time(&currentTime);
    if (lastTimeTypeCange == 0)
    {
        timeTypeDuration = 0;
        lastTimeTypeCange = currentTime;
    }
    else
    {
        timeTypeDuration = currentTime - lastTimeTypeCange;
    }
    
    

    ESP_LOGI(TAG, "TimeTypeDuration is: %d, current time type: %d", timeTypeDuration, currentTimeType);

    switch (currentTimeType)
    {
    case Stopped:
    case Downtime:
        if ((speed >= devCfg.setuptimeRate && speed <= devCfg.runtimeRate) && (timeTypeDuration > devCfg.setuptimeTimeFence))
        {
            lastTimeTypeCange = currentTime;
            currentTimeType = Setuptime;
            return currentTimeType;
        }
        else if ((speed >= devCfg.runtimeRate) && (timeTypeDuration > devCfg.runtimeTimeFence))
        {
            lastTimeTypeCange = currentTime;
            currentTimeType = Runtime;
            return currentTimeType;
        }
        else if ((currentTimeType == Stopped) && (timeTypeDuration > devCfg.stoppedTimeFence))
        {
            currentTimeType = Downtime;
            return currentTimeType;
        }
        break;
    case Setuptime:
        if ((speed < devCfg.setuptimeRate) && (timeTypeDuration > devCfg.setuptimeTimeFence))
        {
            lastTimeTypeCange = currentTime;
            currentTimeType = Stopped;
            return currentTimeType;
        }
        else if ((speed >= devCfg.runtimeRate) && (timeTypeDuration > devCfg.runtimeTimeFence))
        {
            lastTimeTypeCange = currentTime;
            currentTimeType = Runtime;
            return currentTimeType;
        }
        break;
    case Runtime:
        if ((speed < devCfg.runtimeRate && speed > devCfg.setuptimeRate) && (timeTypeDuration > devCfg.runtimeTimeFence))
        {
            lastTimeTypeCange = currentTime;
            currentTimeType = Setuptime;
            return currentTimeType;
        }
        else if (speed < devCfg.setuptimeRate && timeTypeDuration > devCfg.stoppedTimeFence)
        {
            lastTimeTypeCange = currentTime;
            currentTimeType = Stopped;
            return currentTimeType;
        }
        break;
    default:
        return currentTimeType;
        break;
    }

    return currentTimeType;
}
