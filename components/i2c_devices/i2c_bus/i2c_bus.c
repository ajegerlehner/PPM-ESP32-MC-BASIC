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
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "iot_i2c_bus.h"

#define I2C_BUS_MAX_DEVICES     (16)
#define I2C_BUS_TIMEOUT_MS      (1000)
#define I2C_BUS_GLITCH_IGNORE   (7)

typedef struct {
    uint16_t dev_addr;                  /*!<I2C device address */
    i2c_master_dev_handle_t handle;     /*!<I2C device handle */
} i2c_bus_device_t;

typedef struct {
    i2c_master_bus_handle_t bus_handle; /*!<I2C master bus */
    uint32_t clk_speed_hz;              /*!<I2C clock frequency */
    SemaphoreHandle_t mutex;            /*!<protects the device list */
    i2c_bus_device_t devices[I2C_BUS_MAX_DEVICES];
    int device_num;
} i2c_bus_t;

static const char* I2C_BUS_TAG = "i2c_bus";
#define I2C_BUS_CHECK(a, str, ret)  if(!(a)) {                                             \
    ESP_LOGE(I2C_BUS_TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);      \
    return (ret);                                                                   \
    }

i2c_bus_handle_t iot_i2c_bus_create(i2c_port_num_t port, int sda_io, int scl_io, uint32_t clk_speed_hz)
{
    I2C_BUS_CHECK(port < I2C_NUM_MAX, "I2C port error", NULL);
    i2c_bus_t* bus = (i2c_bus_t*) calloc(1, sizeof(i2c_bus_t));
    I2C_BUS_CHECK(bus != NULL, "No memory", NULL);
    i2c_master_bus_config_t bus_conf = {
        .i2c_port = port,
        .sda_io_num = sda_io,
        .scl_io_num = scl_io,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_BUS_GLITCH_IGNORE,
        .flags.enable_internal_pullup = false,
    };
    bus->clk_speed_hz = clk_speed_hz;
    bus->mutex = xSemaphoreCreateMutex();
    if(bus->mutex == NULL) {
        goto error;
    }
    esp_err_t ret = i2c_new_master_bus(&bus_conf, &bus->bus_handle);
    if(ret != ESP_OK) {
        goto error;
    }
    return (i2c_bus_handle_t) bus;

    error:
    if(bus->mutex) {
        vSemaphoreDelete(bus->mutex);
    }
    free(bus);
    return NULL;
}

esp_err_t iot_i2c_bus_delete(i2c_bus_handle_t bus)
{
    I2C_BUS_CHECK(bus != NULL, "Handle error", ESP_FAIL);
    i2c_bus_t* i2c_bus = (i2c_bus_t*) bus;
    for (int i = 0; i < i2c_bus->device_num; i++) {
        i2c_master_bus_rm_device(i2c_bus->devices[i].handle);
    }
    i2c_del_master_bus(i2c_bus->bus_handle);
    vSemaphoreDelete(i2c_bus->mutex);
    free(bus);
    return ESP_OK;
}

// devices are attached to the bus on first use
static i2c_master_dev_handle_t i2c_bus_get_device(i2c_bus_t* i2c_bus, uint16_t dev_addr)
{
    i2c_master_dev_handle_t handle = NULL;
    xSemaphoreTake(i2c_bus->mutex, portMAX_DELAY);
    for (int i = 0; i < i2c_bus->device_num; i++) {
        if (i2c_bus->devices[i].dev_addr == dev_addr) {
            handle = i2c_bus->devices[i].handle;
            break;
        }
    }
    if (handle == NULL && i2c_bus->device_num < I2C_BUS_MAX_DEVICES) {
        i2c_device_config_t dev_conf = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = dev_addr,
            .scl_speed_hz = i2c_bus->clk_speed_hz,
        };
        if (i2c_master_bus_add_device(i2c_bus->bus_handle, &dev_conf, &handle) == ESP_OK) {
            i2c_bus->devices[i2c_bus->device_num].dev_addr = dev_addr;
            i2c_bus->devices[i2c_bus->device_num].handle = handle;
            i2c_bus->device_num++;
        } else {
            handle = NULL;
        }
    }
    xSemaphoreGive(i2c_bus->mutex);
    return handle;
}

esp_err_t iot_i2c_bus_write(i2c_bus_handle_t bus, uint16_t dev_addr, const uint8_t *data, size_t len)
{
    I2C_BUS_CHECK(bus != NULL, "Handle error", ESP_FAIL);
    i2c_master_dev_handle_t dev = i2c_bus_get_device((i2c_bus_t*) bus, dev_addr);
    I2C_BUS_CHECK(dev != NULL, "I2C device error", ESP_FAIL);
    return i2c_master_transmit(dev, data, len, I2C_BUS_TIMEOUT_MS) == ESP_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t iot_i2c_bus_read(i2c_bus_handle_t bus, uint16_t dev_addr, uint8_t *data, size_t len)
{
    I2C_BUS_CHECK(bus != NULL, "Handle error", ESP_FAIL);
    i2c_master_dev_handle_t dev = i2c_bus_get_device((i2c_bus_t*) bus, dev_addr);
    I2C_BUS_CHECK(dev != NULL, "I2C device error", ESP_FAIL);
    return i2c_master_receive(dev, data, len, I2C_BUS_TIMEOUT_MS) == ESP_OK ? ESP_OK : ESP_FAIL;
}

esp_err_t iot_i2c_bus_write_read(i2c_bus_handle_t bus, uint16_t dev_addr, const uint8_t *write_data, size_t write_len,
                                 uint8_t *read_data, size_t read_len)
{
    I2C_BUS_CHECK(bus != NULL, "Handle error", ESP_FAIL);
    i2c_master_dev_handle_t dev = i2c_bus_get_device((i2c_bus_t*) bus, dev_addr);
    I2C_BUS_CHECK(dev != NULL, "I2C device error", ESP_FAIL);
    return i2c_master_transmit_receive(dev, write_data, write_len, read_data, read_len, I2C_BUS_TIMEOUT_MS) == ESP_OK ? ESP_OK : ESP_FAIL;
}
