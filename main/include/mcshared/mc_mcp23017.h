#ifndef _MC_MCP23017_H_
#define _MC_MCP23017_H_

#include "esp_log.h"
#include "esp_types.h"
#include "mc_i2c_bus.h"

// MCP23017 display shield
#define MCP23017D_ADDR  			 0x20    			// I2C slave address for MCP23017 chip (7-bit)
#define MCP23017D_AZURE_PIN          0                  // GPA0
#define MCP23017D_MC_PIN             1                  // GPA1
#define MCP23017D_ENVS_PIN           2                  // GPA2
#define MCP23017D_WDE_PIN            3                  // GPA3
#define MCP23017D_WDT_PIN            4                  // GPA4
#define MCP23017D_PLCOUT_PIN         5                  // GPA5
#define MCP23017D_LOWBAT_GREEN       6                  // GPA6
#define MCP23017D_LOWBAT_RED         7                  // GPA7

#define MCP23017D_SW1_HOME           0                  // GPB0
#define MCP23017D_SW2_FWD            1                  // GPB0
#define MCP23017D_SW3_BWD            2                  // GPB0


int mcp23017d_init();

void mcp23017d_set_pin(int _pin, int _pinValue);

void mcp23017d_signal_pin(int _pin, int _delay);

void mcp23017_signal_pin(int _mcp23017d_pin, int _delay);

bool mcp23017_pin_changed(uint8_t *pin);


#endif