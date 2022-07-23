#ifndef _MC_PCA9554A_H_
#define _MC_PCA9554A_H_

#include "mc_mcp23008.h"

#define PCA9554A_ADDR  			    0x38    			// I2C slave address for PCA9554A chip (7-bit)
#define PCA9554A_AZURE_PIN          7
#define PCA9554A_MC_PIN             6
#define PCA9554A_ENVS_PIN           5
#define PCA9554A_WDE_PIN            4
#define PCA9554A_WDT_PIN            3
#define PCA9554A_LOWBAT_GREEN       1
#define PCA9554A_PLCOUT_PIN         2
#define PCA9554A_LOWBAT_RED         0

#define PCA9554A_CONFIG_REG         0x03
#define PCA9554A_OUTPUT_REG         0x01
#define PCA9554A_PORT_CFG           0x00        // all outputs

int pca9554a_init();
void pca9554a_set_pin(int _pin, int _val);
void pca9554a_signal_pin(int _pin, int _delay);

#endif