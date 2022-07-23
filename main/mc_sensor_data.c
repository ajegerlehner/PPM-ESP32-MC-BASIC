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
#include "mc_mcp23008.h"
#include "mc_gpio.h"
#include "mc_device.h"
#include "mc_i2c_bus.h"
#include "iot_bme280.h"
#include "mc_si7021.h"
#include "mc_ds3231.h"
#include "mc_iis328dq.h"
#include "mc_vl6180x.h"
#include "mc_common.h"
#include "mc_eth.h"
#include "mc_io_expander_hal.h"
#include "mc_data.h"

#define TAG "SENSOR"
#define BME280_DELAY        300
#define SI7021_DELAY        50
#define MAX_STATUS_LEN      100
#define MAX_SERIAL_LEN      30
#define MAX_IP_ADDRESS_LEN  16

const float BME280_SEALEVEL_PRESSURE = 1014.7f;

static bme280_handle_t dev = NULL;
static time_t lastTimeTypeCange;
static TimeType currentTimeType;
static uint8_t bme280_present;
static uint8_t si7021_present;
extern uint8_t wifiConnected;

extern DeviceHWStatus deviceHWStatus;

// flag to track setup time or new operation 
uint8_t setupTimeForced = 1;

void init_sensor_hardware()
{
    esp_err_t ret;

    // initialize BME280 sesnor
    dev = iot_bme280_create(i2c_bus, BME280_I2C_ADDRESS_DEFAULT);
    ret = iot_bme280_init(dev);
    if (ret == ESP_OK)
    {
        io_expander_set_pin(SIGNAL_PIN_ENVS, 1);
        ESP_LOGI(TAG, "BME280 initialized");
        deviceHWStatus.bme280_present = 1;
    }
    else
    {
        io_expander_set_pin(SIGNAL_PIN_ENVS, 0);
        ESP_LOGE(TAG, "BME280 failed to initialize: return code %d. Disabling device.", ret);
        deviceHWStatus.bme280_present = 0;
    }    

    if (!bme280_present)
    {
        ret = si7021_init();
        if (ret == ESP_OK)
        {   
            io_expander_set_pin(SIGNAL_PIN_ENVS, 1);
            ESP_LOGI(TAG, "SI7021 initialized");
            deviceHWStatus.si7021_present = 1;
        }
        else
        {
            io_expander_set_pin(SIGNAL_PIN_ENVS, 0);
            ESP_LOGE(TAG, "SI7021 failed to initialize: return code %d. Disabling device.", ret);
            deviceHWStatus.si7021_present = 0;
        }    
    }


    // initialize IIS328DQ accelerometer / vibration sensor
    iis328dq_init();
    
    // initilize VL6180X gesture / proximity sensor
    vl6180x_init();
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

    status->deviceIPAddress = malloc(MAX_IP_ADDRESS_LEN);
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
    status->peripherialStatus = deviceHWStatusToNum();
    status->appVersion = ota_app_desc->version;
    status->lastOTAUpdate = otaTime;
    
    // serial number is MAC adress of the esp32 chip
    esp_read_mac(macAdress, 0);
    sprintf(status->serialNo, "%x:%x:%x:%x:%x:%x", macAdress[0], macAdress[1], macAdress[2], macAdress[3], macAdress[4], macAdress[5]);

    sprintf(status->deviceIPAddress, "%15s", getDeviceIPAddress());
    status->wifiConnected = wifiConnected;
    
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
        io_expander_set_pin(SIGNAL_PIN_LOWBAT_GREEN, 0);
        io_expander_set_pin(SIGNAL_PIN_LOWBAT_RED, 1);
    }
    else
    {
        io_expander_set_pin(SIGNAL_PIN_LOWBAT_GREEN, 1);
        io_expander_set_pin(SIGNAL_PIN_LOWBAT_RED, 0);
    }
    
}

void read_sensor_data(SamplingMessage *msg, DeviceConfig devConfig)
{
    CounterReading currentReading;
    time_t now;
    uint32_t freeHeapSize;

    // get counter readings in sampling message
    io_expander_signal_pin(SIGNAL_PIN_MC);
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
        io_expander_set_pin(SIGNAL_PIN_PLCOUT, 1);
    }
    else
    {
        io_expander_set_pin(SIGNAL_PIN_PLCOUT, 0);
    }
    
    
    // if BME280 is installed, use this sensor, otherwise try if SI7021 ist installed
    if (deviceHWStatus.bme280_present)
    {
        ESP_LOGI(TAG, "Reading BME280 environmental data...");
        read_bme280_data(msg);
    }
    else if (deviceHWStatus.si7021_present)
    {
        ESP_LOGI(TAG, "Reading SI7021 environmental data...");
        read_si7021_data(msg);
    }
    else
    {
        ESP_LOGI(TAG, "No environmental data sensor available...");
        msg->pressure = 0.0;
        msg->temperature = 0.0;
        msg->altitude = 0.0;
        msg->humidity = 0.0;
    }
    

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

void read_si7021_data(SamplingMessage *msg)
{
    msg->humidity = si7021_read_humidity();
    vTaskDelay(SI7021_DELAY / portTICK_RATE_MS);

    msg->temperature = si7021_read_temperature();

    msg->pressure = 0.0;
    msg->altitude = 0.0;
}

uint8_t deviceHWStatusToNum()
{
    // returns a coded value for the startup peripherial device status
    return (deviceHWStatus.mcp23008d_present << 7)      // MSB
        + (deviceHWStatus. mcp23008b_present << 6) 
        + (deviceHWStatus.pca9554a_present << 5) 
        + (deviceHWStatus.bme280_present << 4) 
        + (deviceHWStatus.si7021_present << 3) 
        + (deviceHWStatus.iis328dq_present << 2) 
        + (deviceHWStatus.vl6180x_present << 1) 
        + deviceHWStatus.wifi_connected;                // LSB
}    

float calc_speed(long counterDelta, long elapsedMS, float speedFactor)
{
    float retVal;

    ESP_LOGI(TAG, "Speed calculation: counterDelta: %ld, elapsedMS: %ld, speedFactor: %f", counterDelta, elapsedMS, speedFactor);

    if (elapsedMS > 0)
    {
        retVal = 3600.0f / elapsedMS * 1000.0f * counterDelta * speedFactor; 
    }
    else 
    {
        retVal = 0.0;
    }
    return decRound3(retVal);
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

    ESP_LOGI(TAG, "TimeTypeDuration is: %d, current time type: %d, setupTimeForced: %d", timeTypeDuration, currentTimeType, setupTimeForced);

    // if setupTimeForced is true, we always return Setuptime, unless Runtime was entered
    if (setupTimeForced && (speed >= devCfg.runtimeRate) && (timeTypeDuration > devCfg.runtimeTimeFence))
    {
        setSetupTimeForced(0);
        lastTimeTypeCange = currentTime;
        currentTimeType = Runtime;
        ESP_LOGI(TAG, "Setuptime forced reset: TimeTypeDuration is: %d, current time type: %d", timeTypeDuration, currentTimeType);
        return currentTimeType;
    }            
    else if (setupTimeForced)
    {        
        currentTimeType = Setuptime;
        return currentTimeType;
    }

    // if Setuptime is not forced, follow state transition logic
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
            else if ((currentTimeType == Stopped) && (timeTypeDuration > devCfg.downtimeTimeFence))
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

void setSetupTimeForced(uint8_t _val)
{
    setupTimeForced = _val;
}

uint8_t read_tof_gesture()
{
    uint8_t ret;

    ret = vl6180x_gesture();

    // ret = 1 - gesture detected, else ret = 0
    return ret;
}
