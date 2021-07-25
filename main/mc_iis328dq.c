/**
 * Name:            mc_iis328dq.c
 * Description:     Driver for the IIS328DQ accelerometer
 * Initial Date:    2020-09-26
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_iis328dq.h"
#include "mc_data.h"

#define TAG  "IIS328DQ"

const uint8_t IIS328DQ_CTRL_REG1 = 0x20;                    // control register 1
const uint8_t IIS328DQ_WHOAMI_REG = 0x0F;                   // who am i register
const uint8_t IIS328DQ_WHOAMI_ID = 0x32;                    // iis328dq device id
const uint8_t IIS328DQ_CTRL_REG1_PM0 = 0x20;                // normal power mode
const uint8_t IIS328DQ_CTRL_REG1_DR0 = 0x08;                // 100 Hz output rate
const uint8_t IIS328DQ_CTRL_REG1_XEN = 0x01;                // X axis enable
const uint8_t IIS328DQ_CTRL_REG1_YEN = 0x02;                // Y axis enable
const uint8_t IIS328DQ_CTRL_REG1_ZEN = 0x04;                // Z axis enable
const uint8_t IIS328DQ_CTRL_REG4 = 0x23;                    // control register 4
const uint8_t IIS328DQ_CTRL_REG4_BDU = 0x80;                // block update
const uint8_t IIS328DQ_CTRL_REG4_FS = 0x00;                 // full scale +-2g
const uint8_t IIS328DQ_STATUS_REG = 0x27;                   // status register
const uint8_t IIS328DQ_OUT_X_L = 0x28;                      // output X lsb
const uint8_t IIS328DQ_OUT_X_H = 0x29;                      // output X msb
const uint8_t IIS328DQ_OUT_Y_L = 0x2A;                      // output Y lsb
const uint8_t IIS328DQ_OUT_Y_H = 0x2B;                      // output Y msb
const uint8_t IIS328DQ_OUT_Z_L = 0x2C;                      // output Z lsb
const uint8_t IIS328DQ_OUT_Z_H = 0x2D;                      // output Z msb
const double IIS328DQ_G_FORCE = 9.80665;                 	// constant g force

i2c_dev_t	iis328dq_dev;

esp_err_t iis328dq_init()
{	
	int ret;
    uint8_t statusReg;
    uint8_t ctrlReg;

	// initialize iis328dq i2c device
	iis328dq_dev.port = I2C_MASTER_NUM;
	iis328dq_dev.addr = IIS328DQ_ADDR;
	
	ESP_LOGI(TAG, "Device initializing...");

	// read who am i register
	ret = i2c_dev_read(&iis328dq_dev, &IIS328DQ_WHOAMI_REG, 1, &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Who am I read failed.");
	}

    if (statusReg != IIS328DQ_WHOAMI_ID)
    {
        ESP_LOGE(TAG, "Who am I returned wrong device id: %x", statusReg);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Device id: %x hex", statusReg);

    // setup registers
    ctrlReg = (IIS328DQ_CTRL_REG1_DR0 | IIS328DQ_CTRL_REG1_PM0 | IIS328DQ_CTRL_REG1_XEN | IIS328DQ_CTRL_REG1_YEN | IIS328DQ_CTRL_REG1_ZEN);
    ret = i2c_dev_write(&iis328dq_dev, &IIS328DQ_CTRL_REG1, 1, &ctrlReg, sizeof(ctrlReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Write REG1 failed.");
	}

    ctrlReg = (IIS328DQ_CTRL_REG4_BDU | IIS328DQ_CTRL_REG4_FS);
    ret = i2c_dev_write(&iis328dq_dev, &IIS328DQ_CTRL_REG4, 1, &ctrlReg, sizeof(ctrlReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Write REG4 failed.");
	}

    ESP_LOGI(TAG, "Device initialized.");

	return ESP_OK;
}

void iis328dq_read_data(SamplingMessage *msg)
{
	int ret;
    uint8_t resultLReg;
    uint8_t resultHReg;	

	ESP_LOGI(TAG, "Reading acceleration data...");

	// read x axis data
	ret = i2c_dev_read(&iis328dq_dev, &IIS328DQ_OUT_X_L, 1, &resultLReg, sizeof(resultLReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading register %x failed", IIS328DQ_OUT_X_L);
	}

	ret = i2c_dev_read(&iis328dq_dev, &IIS328DQ_OUT_X_H, 1, &resultHReg, sizeof(resultHReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading register %x failed", IIS328DQ_OUT_X_H);
	}

	msg->maxAmplitudeX = get_result_value(resultLReg, resultHReg);

	// read y axis data
	ret = i2c_dev_read(&iis328dq_dev, &IIS328DQ_OUT_Y_L, 1, &resultLReg, sizeof(resultLReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading register %x failed", IIS328DQ_OUT_Y_L);
	}

	ret = i2c_dev_read(&iis328dq_dev, &IIS328DQ_OUT_Y_H, 1, &resultHReg, sizeof(resultHReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading register %x failed", IIS328DQ_OUT_Y_H);
	}

	msg->maxAmplitudeY = get_result_value(resultLReg, resultHReg);

	// read z axis data
	ret = i2c_dev_read(&iis328dq_dev, &IIS328DQ_OUT_Z_L, 1, &resultLReg, sizeof(resultLReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading register %x failed", IIS328DQ_OUT_Z_L);
	}

	ret = i2c_dev_read(&iis328dq_dev, &IIS328DQ_OUT_Z_H, 1, &resultHReg, sizeof(resultHReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Reading register %x failed", IIS328DQ_OUT_Z_H);
	}

	msg->maxAmplitudeZ = get_result_value(resultLReg, resultHReg);

	ESP_LOGI(TAG, "Acceleration data: x: %f, y: %f, z: %f", msg->maxAmplitudeX, msg->maxAmplitudeY, msg->maxAmplitudeZ);
}

float get_result_value(uint8_t lowByte, uint8_t highByte)
{
	return (float)((highByte << 8 | lowByte) * 0.98 * IIS328DQ_G_FORCE / 1000);
}