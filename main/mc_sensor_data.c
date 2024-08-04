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
#include "mc_mcp23008.h"
#include "mc_gpio.h"
#include "mc_device.h"
#include "mc_i2c_bus.h"
#include "iot_bme280.h"
#include "mc_si7021.h"
#include "mc_vl6180x.h"
#include "mc_common.h"
#include "mc_eth.h"
#include "mc_io_expander_hal.h"
#include "mc_data.h"
#include "mc_esp32counter.h"
#include "mc_esp32_gpio.h"
#include "mc_mcp23017.h"

#define TAG "SENSOR"
#define BME280_DELAY        300
#define SI7021_DELAY        50
#define MAX_STATUS_LEN      100
#define MAX_SERIAL_LEN      30
#define MAX_APPVERSION_LEN  20
#define MAX_IP_ADDRESS_LEN  16
#define MAX_C2D_LEN         20
#define UNPROD_RUNTIM_FENCE 300

const float BME280_SEALEVEL_PRESSURE = 1014.7f;

static bme280_handle_t dev = NULL;
static time_t lastTimeTypeChange;
static time_t rtTimeStart;
static TimeType currentTimeType;
static float lastSpeed;
static uint8_t bme280_present;
extern uint8_t wifiConnected;

static CounterReading currentReading;

extern DeviceHWStatus deviceHWStatus;

// flag to track setup time or new operation; at startup, no setup time should be forced. 
uint8_t setupTimeForced = 0;

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

    // initialize ESP32 counter gpio pins
    esp32_init_counter_gpio();
    
    // initilize VL6180X gesture / proximity sensor
    vl6180x_init();
}

void init_sensor_data(SamplingMessage *msg, char *deviceId, char *msgType)
{
    msg->iotDeviceId = malloc(strlen(deviceId)+1);
    strcpy(msg->iotDeviceId, deviceId);

    msg->msgType = malloc(strlen(msgType)+1);
    strcpy(msg->msgType, msgType);        
}

void init_devicve_status(DeviceStatus *status, char *deviceId, char *msgType)
{
    status->iotDeviceId = malloc(strlen(deviceId)+1);
    strcpy(status->iotDeviceId, deviceId);

    status->msgType = malloc(strlen(msgType)+1);
    strcpy(status->msgType, msgType);

    status->statusMsg = malloc(MAX_STATUS_LEN+1);
    status->serialNo = malloc(MAX_SERIAL_LEN+1);
    status->appVersion = malloc(MAX_APPVERSION_LEN+1);

    status->lastCloudToDeviceMsg = malloc(MAX_C2D_LEN+1);
    
    status->deviceIPAddress = malloc(MAX_IP_ADDRESS_LEN+1);
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
    status->hwRevision = esp32cnt_hw_revision();  
    status->peripherialStatus = deviceHWStatusToNum();    
    status->lastOTAUpdate = otaTime;

    sprintf(status->appVersion, "%s", ota_app_desc->version);

    // serial number is MAC adress of the esp32 chip
    esp_read_mac(macAdress, 0);
    sprintf(status->serialNo, "%x:%x:%x:%x:%x:%x", macAdress[0], macAdress[1], macAdress[2], macAdress[3], macAdress[4], macAdress[5]);

    sprintf(status->deviceIPAddress, "%15s", getDeviceIPAddress());
    status->wifiConnected = wifiConnected;
    
    strncpy(status->statusMsg, statusTxt, MAX_STATUS_LEN);

    freeHeapSize = esp_get_free_heap_size();
    status->heapSize = freeHeapSize;

    status->processorTemperature = (float)temprature_sens_read();

    status->rtcBatteryLow = 0;
}

void read_sensor_data(SamplingMessage *msg, DeviceConfig devConfig)
{
    uint32_t freeHeapSize;
    

    // get counter readings in sampling message
    io_expander_signal_pin(SIGNAL_PIN_MC);
    time(&msg->timeStamp);

    esp32cnt_read_counters(&currentReading);
    
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

    if(devConfig.counterFactor != 1)
    {
        msg->counterDelta1 = calc_delta(msg->counterDelta1, devConfig.counterFactor);
    }

    msg->timeType       = calc_timeType(msg->speed, devConfig);

    if (msg->timeType == Auxiliary)
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
    
    freeHeapSize = esp_get_free_heap_size();
    msg->freeHeapSize = freeHeapSize;

    msg->rtd_temperature = 0.0;
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

float calc_delta(long counterDelta, float counterFactor)
{
    float retVal;

    ESP_LOGI(TAG, "Delta calculation: counterDelta: %ld, counterFactor: %f", counterDelta, counterFactor);

    if (counterFactor > 0)
    {
        retVal = counterDelta * counterFactor; 
    }
    else 
    {
        retVal = counterDelta;
    }
    return decRound3(retVal);
}


TimeType calc_timeType(float speed, DeviceConfig devCfg)
{
    int timeTypeDuration;
    int rtTimeDuration = 0;
    time_t currentTime;

    time(&currentTime);
    if (lastTimeTypeChange == 0)
    {
        timeTypeDuration = 0;
        lastTimeTypeChange = currentTime;
        lastSpeed = speed;
    }
    else
    {
        timeTypeDuration = currentTime - lastTimeTypeChange;
    }

    if ((speed > 0 && lastSpeed == 0) || (speed == 0 && lastSpeed > 0))
    {
        timeTypeDuration = 0;
        lastTimeTypeChange = currentTime;
        lastSpeed = speed;
    }

    ESP_LOGI(TAG, "TimeTypeDuration is: %d, current time type: %d, setupTimeForced: %d", timeTypeDuration, currentTimeType, setupTimeForced);

    // if setupTimeForced is true, we always return Setuptime, unless Runtime was entered
    if (setupTimeForced && (speed >= devCfg.runtimeRate) && (timeTypeDuration > devCfg.runtimeTimeFence))
    {
        setSetupTimeForced(0);
        lastTimeTypeChange = currentTime;
        lastSpeed = speed;
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
        case Pause:
        case Auxiliary:
        case Downtime:
            //if ((speed >= devCfg.setuptimeRate && speed <= devCfg.runtimeRate) && (timeTypeDuration > devCfg.setuptimeTimeFence))
            //{
            //    lastTimeTypeChange = currentTime;
            //    currentTimeType = Setuptime;
            //    return currentTimeType;
            //}
            //else 
            if ((speed >= devCfg.runtimeRate) && (timeTypeDuration > devCfg.runtimeTimeFence))
            {
                lastTimeTypeChange = currentTime;
                currentTimeType = Runtime;
                return currentTimeType;
            }
            else if ((currentTimeType == Stopped) && (timeTypeDuration > devCfg.downtimeTimeFence))
            {
                currentTimeType = Auxiliary;
                return currentTimeType;
            }
            break;
        case Unproductive:
            if (rtTimeStart)
            {
                rtTimeDuration = currentTime - rtTimeStart;
            }

            if ((speed >= devCfg.runtimeRate) && (rtTimeDuration == 0))
            {
                time(&rtTimeStart);
                rtTimeDuration = 1;
                ESP_LOGI(TAG, "Runtime during unproductive time start: rtTimeDuration: %d", rtTimeDuration);
            }
            else if ((speed >= devCfg.runtimeRate) && rtTimeDuration > 0)
            {
                ESP_LOGI(TAG, "Runtime during unproductive time cont: rtTimeDuration: %d", rtTimeDuration);
                if (rtTimeDuration > UNPROD_RUNTIM_FENCE)
                {
                    lastTimeTypeChange = currentTime;
                    currentTimeType = Runtime;
                    rtTimeDuration = 0;
                    return currentTimeType;                    
                }
            }            
            else if (speed == 0)
            {
                rtTimeDuration = 0;
                rtTimeStart = 0;
                ESP_LOGI(TAG, "Runtime during unproductive time stop: rtTimeDuration: %d", rtTimeDuration);
            }
            break;
        case Setuptime:
            if ((speed >= devCfg.runtimeRate) && (timeTypeDuration > devCfg.setuptimeTimeFence))
            {
                lastTimeTypeChange = currentTime;
                currentTimeType = Runtime;
                return currentTimeType;
            }
            else if ((speed < devCfg.runtimeRate))
            {
                lastTimeTypeChange = currentTime;
            }
            break;
        case Runtime:
            if (speed < devCfg.setuptimeRate && timeTypeDuration > devCfg.stoppedTimeFence)
            {
                lastTimeTypeChange = currentTime;
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

void setCurrentTimeType(TimeType _current)
{
    currentTimeType = _current;
}

void setStartupTimeType(DeviceConfig *cfg)
{
    if (cfg->lastTimeType == Stale)
        currentTimeType = Unproductive;
    else
        currentTimeType = cfg->lastTimeType;

    ESP_LOGI(TAG, "Setting startup time type to: %d", currentTimeType);
}

uint8_t read_tof_gesture()
{
    uint8_t ret;

    ret = vl6180x_gesture();

    // ret = 1 - gesture detected, else ret = 0
    return ret;
}

uint8_t read_button_press()
{
    uint8_t ret;

    if (mcp23017_pin_changed(&ret))
    {
        ESP_LOGI(TAG, "Pin pressed: %02x", ret);
    }


    // ret = 0 - no press
    // ret = 1 - Home button
    // ret = 2 - FWD button
    // ret = 4 - BCK button

    return ret;
}
