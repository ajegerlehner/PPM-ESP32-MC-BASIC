#ifndef _MC_SI7021_H_
#define _MC_SI7021_H_

#include "esp_log.h"
#include "esp_types.h"
#include "mc_i2c_bus.h"

#define SI7021_ADDR    0x40    // I2C slave address for SI7021 env sensor chip (7-bit)

esp_err_t si7021_init();
float si7021_read_humidity();
float si7021_read_temperature();
double convertHumidityReading(uint8_t*);
double convertTemperatureReading(uint8_t*);

#endif