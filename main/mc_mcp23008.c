/**
 * Name:            mc_mcp23017.c
 * Description:     Driver for the DS3231 realtime clock chip
 * Initial Date:    2020-06-12
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_mcp23008.h"

#define TAG "MCP23008"

const int8_t MCP23008_OLAT_REG = 0x0A;
const int8_t MCP23008_IODIRA_REG = 0x00;
const int8_t MCP23008_IODIRA_CLEAR = 0x00;	
const int8_t MCP23008_OLAT_CLEAR = 0x00;

static int8_t mcp23008b_olat_register;			// onboard MCP23008
static int8_t mcp23008d_olat_register;			// display shield MCP23008

void mcp230xx_init()
{
	mcp23008d_init();							// display shield MCP23008
	mcp23008b_init();							// onboard MCP23008
}

void mcp23008d_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "MCP23008 display shield initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_IODIRA_REG, ACK_CHECK_EN);								// IODIRA
	i2c_master_write_byte(cmd, MCP23008_IODIRA_CLEAR, ACK_CHECK_EN);								//Set all as outputs on A
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 display shield init failed.");
	}

	
	vTaskDelay(30 / portTICK_PERIOD_MS);
	// initialize OLAT register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_CLEAR, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 display shield init failed.");
	}

	mcp23008d_olat_register = 0x00;

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MCP23008 display shield initialized.");
	}
}

void mcp23008b_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "MCP23008 onboard initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008B_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_IODIRA_REG, ACK_CHECK_EN);								// IODIRA
	i2c_master_write_byte(cmd, MCP23008_IODIRA_CLEAR, ACK_CHECK_EN);							//Set all as outputs on A
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 onboard init failed.");
	}

	
	vTaskDelay(30 / portTICK_PERIOD_MS);
	// initialize OLAT register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008B_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_CLEAR, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 onboard init failed.");
	}

	mcp23008b_olat_register = 0x00;

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MCP23008 onboard initialized.");
	}
}

void mcp23008d_set_pin(int _pin, int _pinValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	if (_pinValue == 0)
	{
		mcp23008d_olat_register = clear_port_bit(mcp23008d_olat_register, _pin); 
	}
	else
	{
		mcp23008d_olat_register = set_port_bit(mcp23008d_olat_register, _pin);
	}
	
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, mcp23008d_olat_register, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23008 display shield failed to set pin %d", _pin);
	}
}

void mcp23008b_set_pin(int _pin, int _pinValue)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	if (_pinValue == 0)
	{
		mcp23008b_olat_register = clear_port_bit(mcp23008b_olat_register, _pin); 
	}
	else
	{
		mcp23008b_olat_register = set_port_bit(mcp23008b_olat_register, _pin);
	}
	
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008B_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, mcp23008b_olat_register, ACK_CHECK_EN);								// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23008 onboard failed to set pin %d", _pin);
	}
}

void mcp23008d_signal_pin(int _pin, int _delay)
{
	mcp23008d_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23008d_set_pin(_pin, 0);
}

void mcp23008b_signal_pin(int _pin, int _delay)
{
	mcp23008b_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23008b_set_pin(_pin, 0);
}

void mcp23008_signal_pin(int _mcp23008d_pin, int _mcp23008b_pin, int _delay)
{
	mcp23008d_set_pin(_mcp23008d_pin, 1);
	mcp23008b_set_pin(_mcp23008b_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23008d_set_pin(_mcp23008d_pin, 0);
	mcp23008b_set_pin(_mcp23008b_pin, 0);
}

void mcp23008_signal_boot_end()
{
	mcp23008b_signal_pin(MCP23008B_MC_PIN, 250);
	mcp23008b_signal_pin(MCP23008B_AZURE_PIN, 250);
	mcp23008b_signal_pin(MCP23008B_WDT_PIN, 250);
	mcp23008b_signal_pin(MCP23008B_PLCOUT_PIN, 250);
}

int8_t clear_port_bit(int8_t _register, int _bitNumber)
{
	return (int8_t)(_register & ~(1 << _bitNumber));
}

int8_t set_port_bit(int8_t _register, int _bitNumber)
{
	return (int8_t)(_register | (1 << _bitNumber));
}
