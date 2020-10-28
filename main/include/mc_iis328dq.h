#ifndef _MC_IIS328DQ_H_
#define _MC_IIS328DQ_H_

#include "esp_log.h"
#include "esp_types.h"
#include "mc_i2c_bus.h"
#include "mc_data.h"

#define IIS328DQ_ADDR     0x19      // IIS328DQ accelerometer adress (7-bit)

esp_err_t iis328dq_init(void);
void iis328dq_read_data(SamplingMessage *msg);
float get_result_value(uint8_t lowByte, uint8_t highByte);
#endif