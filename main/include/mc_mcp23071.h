#ifndef _MC_MCP23017_H_
#define _MC_MCP23017_H_

#include "esp_log.h"
#include "esp_types.h"
#include "mc_i2c_bus.h"

// MCP23017 display shield
#define MCP23017_ADDR  				0x20    			// I2C slave address for MCP23017 chip (7-bit)
#define MCP23017_AZURE_PIN          0
#define MCP23017_MC_PIN             1
#define MCP23017_WDT_PIN            4
#define MCP23017_WDE_PIN            3
#define MCP23017_LOWBAT_RED         7
#define MCP23017_LOWBAT_GREEN       6

// MCP23008 main board
#define MCP23008_ADDR               0x24                // I2C slave address for MCP23008 chip (7-bit)
#define MCP23008_AZURE_PIN          1
#define MCP23008_MC_PIN             0
#define MCP23008_WDT_PIN            2
#define MCP23008_PLCOUT_PIN         3


void mcp230xx_init();
void mcp23017_init();
void mcp23008_init();

void mcp23017_set_pin(int _pin, int _pinValue);
void mcp23008_set_pin(int _pin, int _pinValue);

void mcp23017_signal_pin(int _pin, int _delay);
void mcp23008_signal_pin(int _pin, int _delay);

void mcp230xx_signal_pin(int _mcp23017_pin, int _mcp23008_pin, int _delay);

int8_t clear_port_bit(int8_t _register, int _bitNumber);
int8_t set_port_bit(int8_t _register, int _bitNumber);

#endif