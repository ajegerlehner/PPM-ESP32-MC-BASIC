
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mc_i2c_bus.h"

#define TAG "I2C"

i2c_bus_handle_t i2c_bus;

void i2c_bus_init()
{
    ESP_LOGI(TAG, "I2C initialization started.");

    // external pull-ups on the board, internal pull-ups are not used
    i2c_bus = iot_i2c_bus_create(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
    ESP_LOGI(TAG, "I2C initialization with SCL pin %d and SDA pin %d.", I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO);
    ESP_LOGI(TAG, "I2C initialization completed.");
}

esp_err_t i2c_dev_read(const i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size)
{
    if (!dev || !in_data || !in_size) return ESP_ERR_INVALID_ARG;

    esp_err_t res;
    if (out_data && out_size)
    {
        res = iot_i2c_bus_write_read(i2c_bus, dev->addr, out_data, out_size, in_data, in_size);
    }
    else
    {
        res = iot_i2c_bus_read(i2c_bus, dev->addr, in_data, in_size);
    }

    if (res != ESP_OK)
        ESP_LOGE(TAG, "Could not read from device [0x%02x at %d]: %d", dev->addr, dev->port, res);

    return res;
}

esp_err_t i2c_dev_write(const i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size)
{
    if (!dev || !out_data || !out_size) return ESP_ERR_INVALID_ARG;

    // register and data are sent in one transfer
    size_t reg_size = (out_reg && out_reg_size) ? out_reg_size : 0;
    uint8_t buffer[reg_size + out_size];
    if (reg_size)
        memcpy(buffer, out_reg, reg_size);
    memcpy(buffer + reg_size, out_data, out_size);

    esp_err_t res = iot_i2c_bus_write(i2c_bus, dev->addr, buffer, sizeof(buffer));
    if (res != ESP_OK)
        ESP_LOGE(TAG, "Could not write to device [0x%02x at %d]: %d", dev->addr, dev->port, res);

    return res;
}
