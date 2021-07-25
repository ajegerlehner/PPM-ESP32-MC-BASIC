/**
 * Name:            mc_sensor_data.h
 * Description:     Header file for sensor data collection fuctions
 * Initial Date:    2020-08-10
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#ifndef _MC_SENSOR_DATA_H_
#define _MC_SENSOR_DATA_H_

#include "iot_i2c_bus.h"
#include "mc_data.h"
#include "mc_device.h"

void init_sensor_hardware();
void init_sensor_data(SamplingMessage *msg, char *deviceId, char *msgType);
void init_devicve_status(DeviceStatus *status, char *deviceId, char *msgType);
void create_status_msg(DeviceStatus *status, SamplingMessage *samplingMsg, char *statusTxt);
void read_sensor_data(SamplingMessage *msg, DeviceConfig devConfig);
float calc_speed(long counterDelta, long elapsedMS, float speedFactor);
void read_bme280_data(SamplingMessage *msg);
TimeType calc_timeType(float speed, DeviceConfig devCfg);
void signal_bat_low_status(bool _batLow);
uint8_t read_tof_gesture();
void read_si7021_data(SamplingMessage *msg);
#endif

