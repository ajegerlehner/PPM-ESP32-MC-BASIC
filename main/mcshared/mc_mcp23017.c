/**
 * Name:            mc_mcp23017.c
 * Description:     Driver for the MCP23017 i/o expander chip
 * Initial Date:    2024-07-28
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_common.h"
#include "mc_io_expander_hal.h"
#include "mc_mcp23017.h"

#define TAG "MCP23017"

const int8_t MCP23017_OLATA_REG = 0x14;
const int8_t MCP23017_OLATB_REG = 0x15;
const int8_t MCP23017_IODIRA_REG = 0x00;
const int8_t MCP23017_IODIRA_INIT = 0x00;		// all port to output	
const int8_t MCP23017_IODIRB_REG = 0x01;
const int8_t MCP23017_IODIRB_INIT = 0x07;		// ports 1-3 to input
const int8_t MCP23017_OLAT_CLEAR = 0x00;
const int8_t MCP23017_GPINTENB_REG = 0x05;		// interrup enable register
const int8_t MCP23017_GPINTENB_VAL = 0x07;		// enable pins 0 - 2
const int8_t MCP23017_DEFVALB_REG = 0x07;		// pin comparison for interrupt register
const int8_t MCP23017_DEFVALB_VAL = 0x07;		// compare against pin set (pins 0 - 2)
const int8_t MCP23017_INTCONB_REG = 0x09;		// interrup compare register
const int8_t MCP23017_INTCONB_VAL = 0x07;		// interrup compare register, enable pins 0 - 2
const int8_t MCP23017_INTFB_REG = 0x0F;			// interrup flag register
const int8_t MCP23017_INTCAPB_REG = 0x11;		// interrup capture register

static int8_t mcp23017d_olata_register;			// display shield MCP23017 output pins
static int8_t mcp23017d_olatb_register;			// display shield MCP23017 input pins

extern DeviceHWStatus deviceHWStatus;

int mcp23017d_init()
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	ESP_LOGI(TAG, "MCP23017 display shield initializing...");

	// Initialize IODIRA register, set all ports to output
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_IODIRA_REG, ACK_CHECK_EN);				// IODIRA
	i2c_master_write_byte(cmd, MCP23017_IODIRA_INIT, ACK_CHECK_EN);				//Set all ports directions
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 display shield init IODIRA failed.");
		ESP_LOGE(TAG, "MCP23017 display device disabled.");
		return 0;
	}
	ESP_LOGI(TAG, "MCP23017 display shield PORT A initialized.");

	// Initialize IODIRB register, set all ports to output, ports 0-2 inputs
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_IODIRB_REG, ACK_CHECK_EN);				// IODIRB
	i2c_master_write_byte(cmd, MCP23017_IODIRB_INIT, ACK_CHECK_EN);				//Set first three ports to input
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 display shield init IODIRB failed.");
		ESP_LOGE(TAG, "MCP23017 display device disabled.");
		return 0;
	}
	ESP_LOGI(TAG, "MCP23017 display shield PORT B initialized.");
	
	vTaskDelay(30 / portTICK_PERIOD_MS);

	// initialize OLATA register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLATA_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLAT_CLEAR, ACK_CHECK_EN);					// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 display shield init OLATA failed.");
		ESP_LOGE(TAG, "MCP23017 display device disabled.");
		return 0;
	}

	// initialize OLATB register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLATB_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLAT_CLEAR, ACK_CHECK_EN);					// Clear OLAT register
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 display shield init OLATB failed.");
		ESP_LOGE(TAG, "MCP23017 display device disabled.");
		return 0;
	}

	mcp23017d_olata_register = 0x00;
	mcp23017d_olatb_register = 0x00;

	// enable interrups on port B - pin mapping is in GPINTENB_VAL
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_GPINTENB_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_GPINTENB_VAL, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 display shield init GPINTENB failed.");
		ESP_LOGE(TAG, "MCP23017 display device disabled.");
		return 0;
	}

	// set default comparison values on port B - pin mapping is in DEFVALB_VAL
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_DEFVALB_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_DEFVALB_VAL, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 display shield init DEFVALB failed.");
		ESP_LOGE(TAG, "MCP23017 display device disabled.");
		return 0;
	}

	// set pins for interrupt comparison mode on port B - pin mapping is in INTCONB_VAL
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_INTCONB_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_INTCONB_VAL, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "MCP23017 display shield init INTCONB failed.");
		ESP_LOGE(TAG, "MCP23017 display device disabled.");
		return 0;
	}

	ESP_LOGI(TAG, "MCP23017 display shield PORT B interrupts initialized.");

	if (ret == ESP_OK)
	{
		ESP_LOGI(TAG, "MCP23017 display shield initialized.");
	}

	return 1;
}


void mcp23017d_set_pin(int _pin, int _pinValue)
{
	if (!deviceHWStatus.mcp23017d_present)
		return;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	int ret;

	if (_pinValue == 0)
	{
		mcp23017d_olata_register = clear_port_bit(mcp23017d_olata_register, _pin); 
	}
	else
	{
		mcp23017d_olata_register = set_port_bit(mcp23017d_olata_register, _pin);
	}
	
	// Update OLATA register
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_OLATA_REG, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, mcp23017d_olata_register, ACK_CHECK_EN);								
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23017 display shield failed to set pin %d on OLATA", _pin);
	}
}


void mcp23017d_signal_pin(int _pin, int _delay)
{
	if (!deviceHWStatus.mcp23017d_present)
		return;
	
	mcp23017d_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	mcp23017d_set_pin(_pin, 0);
}

void mcp23017_signal_pin(int _mcp23017d_pin, int _delay)
{
	ESP_LOGI(TAG, "MCP23017 io expander display present: %d", deviceHWStatus.mcp23017d_present);

	if (deviceHWStatus.mcp23017d_present)
		mcp23017d_set_pin(_mcp23017d_pin, 1);

	vTaskDelay (_delay / portTICK_PERIOD_MS);

	if (deviceHWStatus.mcp23017d_present)
		mcp23017d_set_pin(_mcp23017d_pin, 0);
}

bool mcp23017_pin_changed(uint8_t *pin)
{
	i2c_cmd_handle_t cmd;
	uint8_t regValue = 0;
	uint8_t capValue = 0;	
	int ret;

	// get interrupt capture register value - this will also reset INTFB register
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_INTCAPB_REG, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	// get interrupt capture register value
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, &capValue, 0x01);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23017 display shield failed to get INTCAPB value");
	}

	// get interrupt flag register value
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, MCP23017_INTFB_REG, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23017 display shield failed to set INTFB");
	}

	// get interrupt flag register value
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MCP23017D_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, &regValue, 0x00);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret == ESP_FAIL)
	{
		ESP_LOGE(TAG, "MCP23017 display shield failed to read INTFB");
	}

	if (regValue > 0)
	{
		ESP_LOGI(TAG, "MCP23017 display shield INTFB value %02x", regValue);
	}

	if (regValue > 0 && capValue > 0)
	{
		ESP_LOGI(TAG, "MCP23017 display shield INTCAPB value %02x", capValue);
	}

	*pin = regValue;

	return (regValue > 0);
}


