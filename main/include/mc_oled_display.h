/**
 * Name:            mc_oled_display.h
 * Description:     Header file for machine counter OLED display functions
 * Initial Date:    2020-06-04
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#ifndef _MC_OLED_DISPLAY_H_
#define _MC_OLED_DISPLAY_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mc_data.h"

#include "iot_ssd1306.h"
#include "ssd1306_fonts.h"

#define I2C_MASTER_SCL_IO           32          /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           33          /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0           /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0           /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ          100000      /*!< I2C master clock frequency */


void i2c_bus_init();
esp_err_t ssd1306_show_signs(ssd1306_handle_t dev);
void ssd1306_init();
void oled_write_temp_header();
void oled_write_temp_data();
void oled_init();
void oled_write_data(SamplingMessage* msg, int screenNo);
void oled_wifi_config(char *ipStr, char *apStr);

#endif
