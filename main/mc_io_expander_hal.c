/**
 * Name:            mc_io_expander_hal.c
 * Description:     Driver for the i/o expanders, different hardware designs exist for i/o expanders
 *                  This module acts as an abstaction layer for the MCP23008 and PCA9554A chips
 * Initial Date:    2021-11-11
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include "mc_common.h"
#include "mc_mcp23008.h"
#include "mc_pca9554a.h"
#include "mc_io_expander_hal.h"

#define TAG "IOEXP"
#define SIGNAL_PIN_DELAY  200

extern DeviceHWStatus deviceHWStatus;

void io_expander_init()
{
    // initialize and setup MCP230XX port expanders
    deviceHWStatus.mcp23008d_present = mcp23008d_init();   // display shield port expander MCP23008
	deviceHWStatus.mcp23008b_present = mcp23008b_init();   // main board port expander MCP23008

    // initialize and setup PCA9554A port expander
    deviceHWStatus.pca9554a_present = pca9554a_init();     // alternative display shield port expander PCA9554A    

    ESP_LOGI(TAG, "MCP23008 display present: %d", deviceHWStatus.mcp23008d_present);
    ESP_LOGI(TAG, "MCP23008 board present: %d", deviceHWStatus.mcp23008b_present);
    ESP_LOGI(TAG, "PCA9554A display present: %d", deviceHWStatus.pca9554a_present);
}

void io_expander_signal_boot_end()
{
    mcp23008_signal_boot_end();
}

void io_expander_signal_pin(int _pin)
{
    switch (_pin)
    {
    case SIGNAL_PIN_AZURE:
        mcp23008_signal_pin(MCP23008D_AZURE_PIN, MCP23008B_AZURE_PIN, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_AZURE_PIN, SIGNAL_PIN_DELAY);
        break;
    case SIGNAL_PIN_MC:
        mcp23008_signal_pin(MCP23008D_MC_PIN, MCP23008B_MC_PIN, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_MC_PIN, SIGNAL_PIN_DELAY);
        break;
    case SIGNAL_PIN_ENVS:
        mcp23008d_signal_pin(MCP23008D_ENVS_PIN, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_ENVS_PIN, SIGNAL_PIN_DELAY);
        break;
    case SIGNAL_PIN_WDE:
        mcp23008d_signal_pin(MCP23008D_WDE_PIN, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_WDE_PIN, SIGNAL_PIN_DELAY);
        break;
    case SIGNAL_PIN_WDT:
        mcp23008_signal_pin(MCP23008D_WDT_PIN, MCP23008B_WDT_PIN, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_WDT_PIN, SIGNAL_PIN_DELAY);
        break;
    case SIGNAL_PIN_PLCOUT:
        mcp23008_signal_pin(MCP23008D_PLCOUT_PIN, MCP23008B_PLCOUT_PIN, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_PLCOUT_PIN, SIGNAL_PIN_DELAY);
        break;
    case SIGNAL_PIN_LOWBAT_GREEN:
        mcp23008d_signal_pin(MCP23008D_LOWBAT_GREEN, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_LOWBAT_GREEN, SIGNAL_PIN_DELAY);
        break;
    case SIGNAL_PIN_LOWBAT_RED:
        mcp23008d_signal_pin(MCP23008D_LOWBAT_RED, SIGNAL_PIN_DELAY);
        pca9554a_signal_pin(PCA9554A_LOWBAT_RED, SIGNAL_PIN_DELAY);
        break;
    default:
        break;
    }

}

void io_expander_set_pin(int _pin, int _val)
{
    switch (_pin)
    {
    case SIGNAL_PIN_AZURE:
        mcp23008d_set_pin(MCP23008D_AZURE_PIN, _val);
        mcp23008b_set_pin(MCP23008B_AZURE_PIN, _val);
        pca9554a_set_pin(PCA9554A_AZURE_PIN, _val);
        break;
    case SIGNAL_PIN_MC:
        mcp23008d_set_pin(MCP23008D_MC_PIN, _val);
        mcp23008b_set_pin(MCP23008B_MC_PIN, _val);
        pca9554a_set_pin(PCA9554A_MC_PIN, _val);
        break;
    case SIGNAL_PIN_ENVS:
        mcp23008d_set_pin(MCP23008D_ENVS_PIN, _val);
        pca9554a_set_pin(PCA9554A_ENVS_PIN, _val);
        break;
    case SIGNAL_PIN_WDE:
        mcp23008d_set_pin(MCP23008D_WDE_PIN, _val);
        pca9554a_set_pin(PCA9554A_WDE_PIN, _val);
        break;
    case SIGNAL_PIN_WDT:
        mcp23008d_set_pin(MCP23008D_WDT_PIN, _val);
        mcp23008b_set_pin(MCP23008B_WDT_PIN, _val);
        pca9554a_set_pin(PCA9554A_WDT_PIN, _val);
        break;
    case SIGNAL_PIN_PLCOUT:
        mcp23008d_set_pin(MCP23008D_PLCOUT_PIN, _val);
        mcp23008b_set_pin(MCP23008B_PLCOUT_PIN, _val);
        pca9554a_set_pin(PCA9554A_PLCOUT_PIN, _val);
        break;
    case SIGNAL_PIN_LOWBAT_GREEN:
        mcp23008d_set_pin(MCP23008D_LOWBAT_GREEN, _val);
        pca9554a_set_pin(PCA9554A_LOWBAT_GREEN, _val);
        break;
    case SIGNAL_PIN_LOWBAT_RED:
        mcp23008d_set_pin(MCP23008D_LOWBAT_RED, _val);
        pca9554a_set_pin(PCA9554A_LOWBAT_RED, _val);
        break;
    default:
        break;
    }
}
