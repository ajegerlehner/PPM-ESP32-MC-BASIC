/**
 * Name:            mc_ds3231.c
 * Description:     Driver for the DS3231 realtime clock
 * Initial Date:    2020-08-18
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "sys/time.h"
#include "driver/i2c.h"
#include "mc_ds3231.h"

#define TAG  "DS3231"

const uint8_t DS3231_STATUS_REG 	= 0x0F;
const uint8_t DS3231_OSF_REG 		= (1<<7);
const uint8_t DS3231_TEMP_REG 		= 0x11;
const uint8_t DS3231_TIME_REG		= 0x00;
const uint8_t DS3231_MONTH_MASK 	= 0x1F;

i2c_dev_t	ds3231_dev;
bool rtcBatteryOK;

esp_err_t ds3231_init()
{	
	int ret;
    uint8_t statusReg;
	struct tm *currentTime = NULL;
	time_t now;
	char *asciiTime;
	struct timeval tv;

	// initialize ds3231 i2c device
	ds3231_dev.port = I2C_MASTER_NUM;
	ds3231_dev.addr = DS3231_ADDR;
	
	ESP_LOGI(TAG, "Initializing...");

	// read status register
	ret = i2c_dev_read(&ds3231_dev, &DS3231_STATUS_REG, 1, &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Init failed.");
	}

    if (statusReg & DS3231_OSF_REG) // OSF set - must set new time
    {
		rtcBatteryOK = false;

		return ESP_FAIL;
    }
	else
	{
		currentTime = gmtime(&now);
		ds3231_get_time(currentTime);
		tv.tv_sec = mktime(currentTime);

		// set system time from ds3231 rtc
		settimeofday(&tv, NULL);

		asciiTime = asctime(currentTime);
		ESP_LOGI(TAG, "System time set from DS3231 time: %s", asciiTime);

		rtcBatteryOK = true;

		return ESP_OK;
	}	

	return ESP_OK;
}

esp_err_t ds3231_reset_osf(uint8_t statusReg)
{
	int ret;

	ret = i2c_dev_write(&ds3231_dev, &DS3231_STATUS_REG, 1, (void *)&statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "Reset OSF failed.");
	}
	else
	{
		ESP_LOGI(TAG, "OSF flag has been reset.");
	}
	
	return ret;
}

float ds3231_get_temperature(void)
{
	int ret;
	float temperatureC;
    uint8_t tempVal[2];

	ESP_LOGI(TAG, "Reading temperature...");

	// read temp register	
	ret = i2c_dev_read(&ds3231_dev, &DS3231_TEMP_REG, 1, tempVal, sizeof(tempVal));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading temperature failed.");
	}

	ESP_LOGI(TAG, "Temperature registers: %x - %x", tempVal[1], tempVal[0]);

	temperatureC = ConvertTemperatureReading(tempVal);
	return temperatureC;
}

esp_err_t ds3231_set_time(struct tm *time)
{
    uint8_t data[7];
	int ret;

	ESP_LOGI(TAG, "Setting time...");

    /* time/date data */
    data[0] = IntToBCD(time->tm_sec);
    data[1] = IntToBCD(time->tm_min);
    data[2] = IntToBCD(time->tm_hour);
    /* The week data must be in the range 1 to 7, and to keep the start on the
     * same day as for tm_wday have it start at 1 on Sunday. */
    data[3] = IntToBCD(time->tm_wday + 1);
    data[4] = IntToBCD(time->tm_mday);
    data[5] = IntToBCD(time->tm_mon + 1);
    data[6] = IntToBCD(time->tm_year - 100);

	ret = i2c_dev_write(&ds3231_dev, &DS3231_TIME_REG, 1, data, sizeof(data));	
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Setting time failed.");
	}

    return ret;
}

esp_err_t ds3231_set_time_now()
{
    uint8_t data[7];
	int ret;
	struct tm *currentTime = NULL;
	time_t now;
	char *asciiTime;
	struct timeval tv;


	ESP_LOGI(TAG, "Setting time to now...");

	time(&now);	// current time
	currentTime = gmtime(&now);
	asciiTime = asctime(currentTime);

	ESP_LOGI(TAG, "Setting new time to %s", asciiTime);

	ret = ds3231_set_time(currentTime);	

    return ret;
}


esp_err_t ds3231_get_time(struct tm *time)
{
    uint8_t data[7];
	int ret;

	ESP_LOGI(TAG, "Reading current time...");

    /* read time */
	ret = i2c_dev_read(&ds3231_dev, &DS3231_TIME_REG, 1, data, sizeof(data));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading current time failed.");
	}

    /* convert to unix time structure */
    time->tm_sec = BCDtoInt(data[0]);
    time->tm_min = BCDtoInt(data[1]);
    time->tm_hour = BCDtoInt(data[2]); /* 24H */
    time->tm_wday = BCDtoInt(data[3]) - 1;
    time->tm_mday = BCDtoInt(data[4]);
    time->tm_mon  = BCDtoInt(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = BCDtoInt(data[6]) + 100;
    time->tm_isdst = 0;

    // apply a time zone (if you are not using localtime on the rtc or you want to check/apply DST)
    //applyTZ(time);

    return ESP_OK;
}

bool ds3231_get_rtcBatteryStatus(void)
{
	return rtcBatteryOK;
}

float ConvertTemperatureReading(uint8_t *reading)
{
    float tempCelsius = reading[0] + (reading[1] >> 6) * 0.25;

    return tempCelsius;
}

uint8_t IntToBCD(uint8_t i)
{
    return (((i / 10) << 4) | (i % 10));
}

uint8_t BCDtoInt(uint8_t bcd)
{
    return (((bcd >> 4) * 10) + (bcd & 0xF));
}

void ds3231_init_time(void)
{
	int ret;
    uint8_t statusReg;

	struct tm *currentTime = NULL;
	time_t now;
	char *asciiTime;
	struct timeval tv;

	// read status register
	ret = i2c_dev_read(&ds3231_dev, &DS3231_STATUS_REG, 1, &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Init time - read status register failed.");
	}
	
	time(&now);	// current time
	currentTime = gmtime(&now);
	asciiTime = asctime(currentTime);

	ESP_LOGI(TAG, "DS3231 has been reset, setting new time to %s", asciiTime);

	ds3231_set_time(currentTime);	
	ds3231_reset_osf(statusReg ^ DS3231_OSF_REG);	
}
