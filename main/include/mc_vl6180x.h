#ifndef _MC_VL6180X_H_
#define _MC_VL6180X_H_

#include "esp_log.h"
#include "esp_types.h"
#include "mc_i2c_bus.h"

#define VL6180X_ADDR    0x29    // // I2C slave address for VL6081X chip (7-bit)

void vl6180x_init();
void write_data_byte(uint16_t reg, uint8_t val);
void vl61980x_setup();
uint8_t read_data_byte(uint16_t reg);
uint8_t vl6180x_gesture();

#endif