/**
 * Name:            mc_pca9554a.c
 * Description:     Driver for port expander chip PCA9554A, replacement for MCP23008 chip
 * Initial Date:    2021-11-09
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_common.h"
#include "mc_pca9554a.h"

#define TAG "PCA9554A"

extern DeviceHWStatus deviceHWStatus;

static int8_t pca9554a_port_register;			// display shield PCA9554A port register status

int pca9554a_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "PCA9554A display shield initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCA9554A_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, PCA9554A_CONFIG_REG, ACK_CHECK_EN);					// PCA9554A config register
	i2c_master_write_byte(cmd, PCA9554A_PORT_CFG, ACK_CHECK_EN);					//Set all ports as outputs
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "PCA9554A display shield init failed.");
		ESP_LOGE(TAG, "PCA9554A device disabled.");
		return 0;
	}
	
	pca9554a_port_register = 0x00;

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "PCA9554A display shield initialized.");
	}

	return 1;
}

void pca9554a_set_pin(int _pin, int _pinValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	if(!deviceHWStatus.pca9554a_present)
		return;

	if (_pinValue == 0)
	{
		pca9554a_port_register = clear_port_bit(pca9554a_port_register, _pin); 
	}
	else
	{
		pca9554a_port_register = set_port_bit(pca9554a_port_register, _pin);
	}
	
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCA9554A_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, PCA9554A_OUTPUT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, pca9554a_port_register, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "PCA9554A display shield failed to set pin %d", _pin);
	}
}

void pca9554a_signal_pin(int _pin, int _delay)
{
	ESP_LOGI(TAG, "PCA9554A io expander display present: %d", deviceHWStatus.pca9554a_present);

	if(!deviceHWStatus.pca9554a_present)
		return;

	pca9554a_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	pca9554a_set_pin(_pin, 0);
}
