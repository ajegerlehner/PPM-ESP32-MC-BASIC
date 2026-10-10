/**
 * Name:            mc_mcp23017.c
 * Description:     Driver for the DS3231 realtime clock chip
 * Initial Date:    2020-06-12
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_common.h"
#include "mc_io_expander_hal.h"
#include "mc_mcp23008.h"

#define TAG "MCP23008"

const int8_t MCP23008_OLAT_REG = 0x0A;
const int8_t MCP23008_IODIRA_REG = 0x00;
const int8_t MCP23008_IODIRA_CLEAR = 0x00;	
const int8_t MCP23008_OLAT_CLEAR = 0x00;

static int8_t mcp23008b_olat_register;			// onboard MCP23008
static int8_t mcp23008d_olat_register;			// display shield MCP23008

extern DeviceHWStatus deviceHWStatus;

int mcp23008d_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "MCP23008 display shield initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_IODIRA_REG, ACK_CHECK_EN);					// IODIRA
	i2c_master_write_byte(cmd, MCP23008_IODIRA_CLEAR, ACK_CHECK_EN);				//Set all as outputs on A
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 display shield init failed.");
		ESP_LOGE(TAG, "MCP23008 display device disabled.");
		return 0;
	}

	
	vTaskDelay(30 / portTICK_PERIOD_MS);
	// initialize OLAT register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_CLEAR, ACK_CHECK_EN);					// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 display shield init failed.");
		ESP_LOGE(TAG, "MCP23008 display device disabled.");
		return 0;
	}

	mcp23008d_olat_register = 0x00;

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MCP23008 display shield initialized.");
	}

	return 1;
}

int mcp23008b_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "MCP23008 onboard initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008B_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_IODIRA_REG, ACK_CHECK_EN);						// IODIRA
	i2c_master_write_byte(cmd, MCP23008_IODIRA_CLEAR, ACK_CHECK_EN);					//Set all as outputs on A
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 onboard init failed.");
		ESP_LOGE(TAG, "MCP23008 onboard device disabled.");
		return 0;
	}

	
	vTaskDelay(30 / portTICK_PERIOD_MS);
	// initialize OLAT register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23008B_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23008_OLAT_CLEAR, ACK_CHECK_EN);						// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23008 onboard init failed.");
		ESP_LOGE(TAG, "MCP23008 onboard device disabled.");
		return 0;
	}

	mcp23008b_olat_register = 0x00;

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MCP23008 onboard initialized.");
	}

	return 1;
}

void mcp23008d_set_pin(int _pin, int _pinValue)
{
	if (!deviceHWStatus.mcp23008d_present)
		return;

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
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23008 display shield failed to set pin %d", _pin);
	}
}

void mcp23008b_set_pin(int _pin, int _pinValue)
{
	if (!deviceHWStatus.mcp23008b_present)		
		return;

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
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23008 onboard failed to set pin %d", _pin);
	}
}

void mcp23008d_signal_pin(int _pin, int _delay)
{
	if (!deviceHWStatus.mcp23008d_present)
		return;
	
	mcp23008d_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23008d_set_pin(_pin, 0);
}

void mcp23008b_signal_pin(int _pin, int _delay)
{
	if (!deviceHWStatus.mcp23008b_present)
		return;

	mcp23008b_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23008b_set_pin(_pin, 0);
}

void mcp23008_signal_pin(int _mcp23008d_pin, int _mcp23008b_pin, int _delay)
{
	ESP_LOGI(TAG, "MCP23008 io expander display present: %d", deviceHWStatus.mcp23008d_present);
    ESP_LOGI(TAG, "MCP23008 io expander board present: %d", deviceHWStatus.mcp23008b_present);

	if (deviceHWStatus.mcp23008d_present)
		mcp23008d_set_pin(_mcp23008d_pin, 1);

	if (deviceHWStatus.mcp23008b_present)
		mcp23008b_set_pin(_mcp23008b_pin, 1);

	vTaskDelay (_delay / portTICK_PERIOD_MS);

	if (deviceHWStatus.mcp23008d_present)
		mcp23008d_set_pin(_mcp23008d_pin, 0);

	if (deviceHWStatus.mcp23008b_present)
		mcp23008b_set_pin(_mcp23008b_pin, 0);
}

void mcp23008_signal_boot_end()
{
	if (!deviceHWStatus.mcp23008b_present)
		return;

	mcp23008b_signal_pin(MCP23008B_MC_PIN, 250);
	mcp23008b_signal_pin(MCP23008B_AZURE_PIN, 250);
	mcp23008b_signal_pin(MCP23008B_WDT_PIN, 250);
	mcp23008b_signal_pin(MCP23008B_PLCOUT_PIN, 250);
}
