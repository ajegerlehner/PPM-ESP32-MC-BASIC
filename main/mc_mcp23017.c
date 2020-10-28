/**
 * Name:            mc_mcp23017.c
 * Description:     dDriver for the DS3231 realtime clock chip
 * Initial Date:    2020-06-12
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_mcp23071.h"

#define TAG "MCP230xx"

const int8_t MCP23017_OLAT_REG = 0x14;
const int8_t MCP23017_IODIRA_REG = 0x00;
const int8_t MCP23017_IODIRA_CLEAR = 0x00;	
const int8_t MCP23017_OLAT_CLEAR = 0x00;

const int8_t MCP23008_OLAT_REG = 0x0A;
const int8_t MCP23008_IODIRA_REG = 0x00;
const int8_t MCP23008_IODIRA_CLEAR = 0x00;	
const int8_t MCP23008_OLAT_CLEAR = 0x00;

static int8_t mcp23017_olat_register;
static int8_t mcp23008_olat_register;

void mcp230xx_init()
{
	mcp23017_init();
	mcp23008_init();
}

void mcp23017_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "MCP23017 Initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_IODIRA_REG, ACK_CHECK_EN);								// IODIRA
	i2c_master_write_byte(cmd, MCP23017_IODIRA_CLEAR, ACK_CHECK_EN);								//Set all as outputs on A
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 Init failed.");
	}

	
	vTaskDelay(30 / portTICK_PERIOD_MS);
	// initialize OLAT register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLAT_CLEAR, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 Init failed.");
	}

	mcp23017_olat_register = 0x00;

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MCP23017 initialized.");
	}
}

void mcp23008_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "MCP23008 Initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_IODIRA_REG, ACK_CHECK_EN);								// IODIRA
	i2c_master_write_byte(cmd, MCP23008_IODIRA_CLEAR, ACK_CHECK_EN);							//Set all as outputs on A
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 Init failed.");
	}

	
	vTaskDelay(30 / portTICK_PERIOD_MS);
	// initialize OLAT register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_CLEAR, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 Init failed.");
	}

	mcp23008_olat_register = 0x00;

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MCP23008 initialized.");
	}
}

void mcp23017_set_pin(int _pin, int _pinValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	if (_pinValue == 0)
	{
		mcp23017_olat_register = clear_port_bit(mcp23017_olat_register, _pin); 
	}
	else
	{
		mcp23017_olat_register = set_port_bit(mcp23017_olat_register, _pin);
	}
	
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, mcp23017_olat_register, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23017 failed to set pin %d", _pin);
	}
}

void mcp23008_set_pin(int _pin, int _pinValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	if (_pinValue == 0)
	{
		mcp23008_olat_register = clear_port_bit(mcp23008_olat_register, _pin); 
	}
	else
	{
		mcp23008_olat_register = set_port_bit(mcp23008_olat_register, _pin);
	}
	
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, mcp23008_olat_register, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23008 failed to set pin %d", _pin);
	}
}

void mcp23017_signal_pin(int _pin, int _delay)
{
	mcp23017_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23017_set_pin(_pin, 0);
}

void mcp23008_signal_pin(int _pin, int _delay)
{
	mcp23008_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23008_set_pin(_pin, 0);
}

void mcp230xx_signal_pin(int _mcp23017_pin, int _mcp23008_pin, int _delay)
{
	mcp23017_set_pin(_mcp23017_pin, 1);
	mcp23008_set_pin(_mcp23008_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23017_set_pin(_mcp23017_pin, 0);
	mcp23008_set_pin(_mcp23008_pin, 0);
}


int8_t clear_port_bit(int8_t _register, int _bitNumber)
{
	return (int8_t)(_register & ~(1 << _bitNumber));
}

int8_t set_port_bit(int8_t _register, int _bitNumber)
{
	return (int8_t)(_register | (1 << _bitNumber));
}
