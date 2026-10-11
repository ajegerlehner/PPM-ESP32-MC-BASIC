// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _IOT_I2C_BUS_H_
#define _IOT_I2C_BUS_H_
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void* i2c_bus_handle_t;

/**
 * @brief Create and init I2C master bus and return a I2C bus handle
 *
 * @param port I2C port number
 * @param sda_io gpio number of the SDA signal
 * @param scl_io gpio number of the SCL signal
 * @param clk_speed_hz I2C clock frequency
 *
 * @return
 *     - NULL Fail
 *     - Others Success
 */
i2c_bus_handle_t iot_i2c_bus_create(i2c_port_num_t port, int sda_io, int scl_io, uint32_t clk_speed_hz);

/**
 * @brief Delete and release the I2C bus object
 *
 * @param bus I2C bus handle
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t iot_i2c_bus_delete(i2c_bus_handle_t bus);

/**
 * @brief Write data to a device: start - address - data - stop
 *
 * @param bus I2C bus handle
 * @param dev_addr 7-bit I2C address of the device
 * @param data Data to write
 * @param len Number of bytes to write
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail, e.g. device doesn't ACK the transfer
 */
esp_err_t iot_i2c_bus_write(i2c_bus_handle_t bus, uint16_t dev_addr, const uint8_t *data, size_t len);

/**
 * @brief Read data from a device: start - address - data - stop
 *
 * @param bus I2C bus handle
 * @param dev_addr 7-bit I2C address of the device
 * @param data Buffer for the data read
 * @param len Number of bytes to read
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail, e.g. device doesn't ACK the transfer
 */
esp_err_t iot_i2c_bus_read(i2c_bus_handle_t bus, uint16_t dev_addr, uint8_t *data, size_t len);

/**
 * @brief Write data to a device and read from it without releasing the bus:
 *        start - address - write data - start - address - read data - stop
 *
 * @param bus I2C bus handle
 * @param dev_addr 7-bit I2C address of the device
 * @param write_data Data to write
 * @param write_len Number of bytes to write
 * @param read_data Buffer for the data read
 * @param read_len Number of bytes to read
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail, e.g. device doesn't ACK the transfer
 */
esp_err_t iot_i2c_bus_write_read(i2c_bus_handle_t bus, uint16_t dev_addr, const uint8_t *write_data, size_t write_len,
                                 uint8_t *read_data, size_t read_len);
#ifdef __cplusplus
}
#endif

#endif

