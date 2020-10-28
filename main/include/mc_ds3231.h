#ifndef _MC_DS3231_H_
#define _MC_DS3231_H_

#include "esp_log.h"
#include "esp_types.h"
#include "mc_i2c_bus.h"

#define DS3231_ADDR     0x68    			// slave address for DS3231 rtc chip

esp_err_t ds3231_init(void);
float ds3231_get_temperature(void);
float ConvertTemperatureReading(uint8_t *reading);
esp_err_t ds3231_set_time(struct tm *time);
esp_err_t ds3231_set_time_now(void);
esp_err_t ds3231_get_time(struct tm *time);
esp_err_t ds3231_reset_osf(uint8_t statusReg);
bool ds3231_get_rtcBatteryStatus(void);
uint8_t IntToBCD(uint8_t i);
uint8_t BCDtoInt(uint8_t bcd);
void ds3231_init_time(void);

#endif