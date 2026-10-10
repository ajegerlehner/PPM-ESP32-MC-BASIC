/**
 * Name:            mc_si7021.c
 * Description:     Driver for the SI7021 temperature and humidity sensor chip
 * Initial Date:    2021-06-11
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_si7021.h"

#define TAG "SI7021"

const uint8_t SI7021_HUMIDITY_REGISTER = 0xE5;  // Hold master mode
const uint8_t SI7021_TEMP_REGISTER = 0xE0;

i2c_dev_t	si7021_dev;

esp_err_t si7021_init()
{
    int ret;
    uint8_t snCommand1[2] = {0xFA, 0x0F};
    uint8_t snCommand2[2] = {0xFC, 0xC9};
    uint8_t firmWareVers[2] = {0x84, 0xB8};
    uint8_t statusReg[5]; // 4 bytes + CRC
    int64_t serialNo;
    uint8_t firmWareVersion;

	// initialize si7021 i2c device
	si7021_dev.port = I2C_MASTER_NUM;
	si7021_dev.addr = SI7021_ADDR;
	
	ESP_LOGI(TAG, "Device initializing...");

	// read device identification
	ret = i2c_dev_read(&si7021_dev, snCommand1 , 2, &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not retrieve device serial number part 1.");
        return ESP_FAIL;
	}

    serialNo = statusReg[0];
    serialNo = (serialNo << 8) | statusReg[1];
    serialNo = (serialNo << 8) | statusReg[2];
    serialNo = (serialNo << 8) | statusReg[3];

	ret = i2c_dev_read(&si7021_dev, snCommand2 , 2, &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not retrieve device serial number part 2.");
        return ESP_FAIL;
	}

    serialNo = (serialNo << 8) | statusReg[0];
    serialNo = (serialNo << 8) | statusReg[1];
    serialNo = (serialNo << 8) | statusReg[2];
    serialNo = (serialNo << 8) | statusReg[3];

	ret = i2c_dev_read(&si7021_dev, firmWareVers , 2, &firmWareVersion, sizeof(firmWareVersion));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not retrieve device firmware version.");
        return ESP_FAIL;
	}

    if (statusReg[0] == 0x15)
    {
        // SI7021 as per data sheet

        ESP_LOGI(TAG, "Device correctly identified, device id is 0: %x hex, firmware: %x hex", statusReg[0], firmWareVersion);

        ESP_LOGI(TAG, "Device initialization completed.");

        return ESP_OK;
    }

    return ESP_FAIL;
}

float si7021_read_humidity()
{
    esp_err_t ret;
    uint8_t reading[3];

    ESP_LOGI(TAG, "Humidity reading...");

	//ret = i2c_dev_read(&si7021_dev, &SI7021_HUMIDITY_REGISTER , 1, reading, sizeof(reading));
 
	// send the command
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, SI7021_HUMIDITY_REGISTER, true);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(si7021_dev.port, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not send humidity register value.");
        return 0.0;
	}


	// wait for the sensor (50ms)
	vTaskDelay(50 / portTICK_PERIOD_MS);

	// receive the answer
	uint8_t msb, lsb, crc;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_READ, true);
	i2c_master_read_byte(cmd, &msb, 0x00);
	i2c_master_read_byte(cmd, &lsb, 0x00);
	i2c_master_read_byte(cmd, &crc, 0x01);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(si7021_dev.port, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not read humidity value.");
        return 0.0;
	}

    reading[1] = msb;
    reading[0] = lsb;
    return convertHumidityReading(reading);
}

float si7021_read_temperature()
{
	int ret;
    uint8_t reading[2];

    ESP_LOGI(TAG, "Temperature reading...");

	// send the command
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, SI7021_TEMP_REGISTER, true);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(si7021_dev.port, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not send temperature register value.");
        return 0.0;
	}


	// wait for the sensor (50ms)
	vTaskDelay(50 / portTICK_PERIOD_MS);

	// receive the answer
	uint8_t msb, lsb, crc;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (SI7021_ADDR << 1) | I2C_MASTER_READ, true);
	i2c_master_read_byte(cmd, &msb, 0x00);
	i2c_master_read_byte(cmd, &lsb, 0x00);
	i2c_master_read_byte(cmd, &crc, 0x01);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(si7021_dev.port, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not read temperature value.");
        return 0.0;
	}

    reading[1] = msb;
    reading[0] = lsb;

    return convertTemperatureReading(reading);
}

double convertHumidityReading(uint8_t* reading)
{
    uint16_t rhCode = (reading[1] <<8) + reading[0];     // makes a short
    double rhPercent = (125.0 * rhCode / 65536.0) - 6.0;       // as per SI7021 data sheet

    ESP_LOGI(TAG, "Convert humidity reading: %d %f%%", rhCode, rhPercent);

    if (rhPercent < 0)
    {
        rhPercent = 0;
    } else if (rhPercent > 100)
    {
        rhPercent = 100;
    }

    return rhPercent;
}

double convertTemperatureReading(uint8_t* reading)
{
    uint16_t tempCode = (reading[1] <<8) + reading[0];          // makes a short
    double tempCelsius = (175.72 * tempCode / 65536.0) - 46.85;   // as per SI7021 data sheet

    ESP_LOGI(TAG, "Convert temperature reading: %d %fC", tempCode, tempCelsius);

    return tempCelsius;
}

