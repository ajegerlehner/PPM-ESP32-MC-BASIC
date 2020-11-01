/*Based on Espressif ESP32 Azure IoT Firmware code/example*/

#include "ctype.h"
#include "esp_log.h"
#include "mc_oled_display.h"
#include "iot_i2c_bus.h"
#include "mc_i2c_bus.h"
#include "mc_data.h"

static ssd1306_handle_t ssd1306_dev = NULL;

static const char* OLED_TAG = "OLED";

/**SSD1306 - OLED Display**/
/******************************************************************************
 * FunctionName : ssd1306_show_signs()
 * Description  : Function to write symbols to OLED sensor as screen header  
 * Parameters   : dev (ssd1306_handle_t) 
 * Returns      : void
*******************************************************************************/
esp_err_t ssd1306_show_signs(ssd1306_handle_t dev)
{
    ESP_LOGI(OLED_TAG, "Writing Header Symbols to SSD1306 OLED sensor");

    //iot_ssd1306_draw_bitmap(dev, 0, 2, &c_chSingal816[0], 16, 8);
    //iot_ssd1306_draw_bitmap(dev, 24, 2, &c_chBluetooth88[0], 8, 8);
    //iot_ssd1306_draw_bitmap(dev, 40, 2, &c_chMsg816[0], 16, 8);
    //iot_ssd1306_draw_bitmap(dev, 64, 2, &c_chGPRS88[0], 8, 8);
    //iot_ssd1306_draw_bitmap(dev, 90, 2, &c_chSingal816[0], 16, 8);
    iot_ssd1306_draw_bitmap(dev, 112, 2, &c_chBat816[0], 16, 8);

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
    ssd1306_show_signs(ssd1306_dev);

    ESP_LOGI(OLED_TAG, "SSD1306 initialization completed");
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

    ESP_LOGI(OLED_TAG, "OLED initialization completed.");
}

void oled_write_data(SamplingMessage* msg, int screenNo)
{
    char oled_line[20] = {0};  

    switch (screenNo)
    {
    case 0:
        // write device id        
        if (msg->iotDeviceId)
        {
            sprintf(oled_line, "%s", msg->iotDeviceId);
            iot_ssd1306_draw_string(ssd1306_dev, 0, 0, (const uint8_t *) oled_line, 16, 1);
        }
        sprintf(oled_line, "MsgNo:%9ld", msg->msgNum);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 16, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "Cnt 1:%9ld", msg->tickCount1);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 32, (const uint8_t *) oled_line, 16, 1);
        sprintf(oled_line, "TimeT:%9s", timeTypeStr[msg->timeType]);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);
        break;
    case 1:
        sprintf(oled_line, "TempC:%9.1f", msg->temperature);
        iot_ssd1306_draw_string(ssd1306_dev, 0, 48, (const uint8_t *) oled_line, 16, 1);    
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