#ifndef _MC_MCP23017_H_
#define _MC_MCP23017_H_

#include "esp_log.h"
#include "esp_types.h"
#include "mc_i2c_bus.h"

// MCP23008 display shield
#define MCP23008D_ADDR  			 0x20    			// I2C slave address for MCP23008 chip (7-bit)
#define MCP23008D_AZURE_PIN          0
#define MCP23008D_MC_PIN             1
#define MCP23008D_ENVS_PIN           2
#define MCP23008D_WDE_PIN            3
#define MCP23008D_WDT_PIN            4
#define MCP23008D_LOWBAT_GREEN       6
#define MCP23008D_PLCOUT_PIN         5
#define MCP23008D_LOWBAT_RED         7

// MCP23008 main board
#define MCP23008B_ADDR               0x24                // I2C slave address for MCP23008 chip (7-bit)
#define MCP23008B_MC_PIN             0
#define MCP23008B_AZURE_PIN          1
#define MCP23008B_WDT_PIN            2
#define MCP23008B_PLCOUT_PIN         3


void mcp230xx_init();
void mcp23008d_init();
void mcp23008b_init();

void mcp23008d_set_pin(int _pin, int _pinValue);
void mcp23008b_set_pin(int _pin, int _pinValue);

void mcp23008d_signal_pin(int _pin, int _delay);
void mcp23008b_signal_pin(int _pin, int _delay);

void mcp23008_signal_pin(int _mcp23008d_pin, int _mcp23008b_pin, int _delay);
void mcp23008_signal_boot_end();

int8_t clear_port_bit(int8_t _register, int _bitNumber);
int8_t set_port_bit(int8_t _register, int _bitNumber);

#endif