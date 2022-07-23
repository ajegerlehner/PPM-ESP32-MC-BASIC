/*Based on Espressif ESP32 Azure IoT Firmware code/example*/

#include "ctype.h"
#include "sys/time.h"
#include "esp_log.h"
#include "mc_oled_display.h"
#include "iot_i2c_bus.h"
#include "mc_i2c_bus.h"
#include "mc_data.h"
#include "mc_common.h"

static const char *timeTypeStr[] = {
    "Stopped",
    "Setuptime",
    "Runtime",
    "Downtime"
};

static ssd1306_handle_t ssd1306_dev = NULL;

extern uint8_t wifiConnected;
extern uint8_t setupTimeForced;

const uint8_t c_chEth816[16] = //ethernet signal
        { 0xFF, 0xFF, 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#define TAG "OLED"

/**SSD1306 - OLED Display**/
/******************************************************************************
 * FunctionName : ssd1306_show_signs()
 * Description  : Function to write symbols to OLED sensor as screen header  
 * Parameters   : dev (ssd1306_handle_t) 
 * Returns      : void
*******************************************************************************/
esp_err_t ssd1306_show_signs(ssd1306_handle_t dev)
{
    ESP_LOGI(TAG, "Writing Header Symbols to SSD1306 OLED sensor");

    //iot_ssd1306_draw_bitmap(dev, 111, 2, &c_chSingal816[0], 16, 8);
    //iot_ssd1306_draw_bitmap(dev, 111, 2, &c_chBluetooth88[0], 8, 8);
    //iot_ssd1306_draw_bitmap(dev, 40, 2, &c_chMsg816[0], 16, 8);
    //iot_ssd1306_draw_bitmap(dev, 64, 2, &c_chGPRS88[0], 8, 8);
    iot_ssd1306_draw_bitmap(dev, 111, 2, &c_chSingal816[0], 16, 8);
    iot_ssd1306_draw_bitmap(dev, 64, 2, &c_chEth816[0], 16, 8);

    return iot_ssd1306_refresh_gram(dev);
}

/******************************************************************************
 * FunctionName : ssd1306_init()
 * Description  : Function to init SSD1306 Sensor  
 * Parameters   : none
 * Returns      : void
*******************************************************************************/
void ssd1306_init()
{
    ssd1306_dev = iot_ssd1306_create(i2c_bus, SSD1306_I2C_ADDRESS);
    iot_ssd1306_refresh_gram(ssd1306_dev);
    iot_ssd1306_clear_screen(ssd1306_dev, 0x00);
    //ssd1306_show_signs(ssd1306_dev);

    ESP_LOGI(TAG, "SSD1306 initialization completed");
}

/******************************************************************************
 * FunctionName : sensors_init
 * Description  : Function to initialize all of the onboard sensors
 *                button, i2c bus, bh1750, hts221, ssd1306, mpu6050
 * Parameters   : none
 * Returns      : void
*******************************************************************************/
void oled_init()
{

    ssd1306_init();

    ESP_LOGI(TAG, "OLED initialization completed.");
}

void oled_write_data(SamplingMessage* msg, DeviceStatus* stat, int screenNo)
{
    char oled_line[20] = {0};  
    char timeBuf[30] = {0};
    struct tm *OTAtime = NULL;
    char strBuffer[12];
    int upTimeDays, upTimeHrs, upTimeMin;


    switch (screenNo)
    {
    case 0:
        // write device id        
        if (msg->iotDeviceId)
        {
            sprintf(oled_line, "%s", msg->iotDeviceId);
            iot_ssd1306_draw_string(ssd1306_dev, 0, 0, (const uint8_t *) oled_line, 16, 1);
        }
        // write connection type symbol
        if (wifiConnected)
        {
            iot_ssd1306_draw_bitmap(ssd1306_dev, 111, 2, &c_chSingal816[0], 16, 8);
        }
        else
        {
            // to do: iot_ssd1306_draw_bitmap(ssd1306_dev, 111, 2, &c_chEth816[0], 16, 8);
            iot_ssd1306_draw_bitmap(ssd1306_dev, 111, 2, &c_chMsg816[0], 16, 8);            
        }
        sprintf(oled_line, "MsgNo:%9ld", msg->msgNum);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Speed:%9.0f", msg->speed);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
        if (setupTimeForced)
        {
            sprintf(oled_line, "Time*:%9s", timeTypeStr[msg->timeType]);
        }
        else
        {
            sprintf(oled_line, "TimeT:%9s", timeTypeStr[msg->timeType]);
        }
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);
        break;
    case 1:
        sprintf(oled_line, "Cnt 1:%9ld", msg->tickCount1);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Cnt 2:%9ld", msg->tickCount2);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Cnt 3:%9ld", msg->tickCount3);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);    
        break;
    case 2:
        sprintf(oled_line, "Delt1:%9ld", msg->counterDelta1);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Delt2:%9ld", msg->counterDelta2);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Delt3:%9ld", msg->counterDelta3);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);    
        break;
    case 3:
        sprintf(oled_line, "LogL1:%9d", msg->logicLevel1);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "LogL2:%9d", msg->logicLevel2);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "LogL3:%9d", msg->logicLevel2);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);    
        break;
    case 4:
        sprintf(oled_line, "Humid:%9.1f", msg->humidity);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Press:%9.1f", msg->pressure);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);    
        sprintf(oled_line, "Altit:%9.1f", msg->altitude);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);
        break;
    case 5:
        sprintf(oled_line, "FreeH:%9ld", msg->freeHeapSize);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);    
        sprintf(oled_line, "TempC:%9.1f", msg->temperature);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Stm32:%9.1f", msg->rtd_temperature);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);
        break;
    case 6:
        sprintf(oled_line, "AzMsg:%9ld", stat->azureMsgSent);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);    
        sprintf(oled_line, "AzCtD:%9ld", stat->azureMsgReceived);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);    
        sprintf(oled_line, "RtcBt:%9d", stat->rtcBatteryLow);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);
        break;
    case 7:
        OTAtime = localtime(&stat->lastOTAUpdate);

        strftime(strBuffer, 12, "%d.%m.%Y", OTAtime);
        sprintf(oled_line, "AV:%s", stat->appVersion);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);

        sprintf(oled_line, "OTAD:%9s", strBuffer);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);

        strftime(strBuffer, 12, "%T", OTAtime);
        sprintf(oled_line, "OTAT:%10s", strBuffer);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);
        break;
    case 8:
        sprintf(oled_line, "HwRev:%9d", stat->hwRevision);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Flash:%9s", CONFIG_ESPTOOLPY_FLASHSIZE);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);

        upTimeDays = stat->upTime / (3600 * 24);
        upTimeHrs = (stat->upTime - (upTimeDays * 3600 * 24)) / 3600;
        upTimeMin = (stat->upTime % 3600) / 60;
        sprintf(timeBuf, "UpTim:%2dd%2dh%2dm", upTimeDays, upTimeHrs, upTimeMin);
        memset(oled_line, 0, 20);
        strncpy(oled_line, timeBuf, 20);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);    
        break;
    case 9:
        sprintf(oled_line, "Device IP addr:");
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "%15s", stat->deviceIPAddress);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
        #ifdef CONFIG_PPM_IOT_MQTT_WEBSOCKETS
        sprintf(oled_line, "MQTT_WEBSOCKETS");
        #endif
        #ifdef CONFIG_PPM_IOT_MQTT
        sprintf(oled_line, "MQTT");
        #endif
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);
        break;
    default:
        break;
    }

    iot_ssd1306_refresh_gram(ssd1306_dev);
}

void oled_wifi_config(char *ipStr, char *apStr)
{
    char oled_line[20] = {0};  

    sprintf(oled_line, "WIFI CONFIG");
    iot_ssd1306_draw_string(ssd1306_dev, 0, 0, (const uint8_t *) oled_line, 16, 1);
    sprintf(oled_line, "AP:%s", apStr);
    iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
    sprintf(oled_line, "IP:%s", ipStr);
    iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
    sprintf(oled_line, "to config WiFi");
    iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);

    iot_ssd1306_refresh_gram(ssd1306_dev);
}