
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "mc_i2c_bus.h"

#define TAG "I2C"

i2c_bus_handle_t i2c_bus;

void i2c_bus_init()
{
    ESP_LOGI(TAG, "I2C initialization started.");

    i2c_config_t conf;
    conf.clk_flags = 0;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_bus = iot_i2c_bus_create(I2C_MASTER_NUM, &conf);
    //i2c_set_pin(I2C_MASTER_NUM, 33, 32, GPIO_PULLUP_DISABLE, GPIO_PULLUP_DISABLE, I2C_MODE_MASTER);
    ESP_LOGI(TAG, "I2C initialization with SCL pin %d and SDA pin %d.", conf.scl_io_num, conf.sda_io_num);
    ESP_LOGI(TAG, "I2C initialization completed.");
}

esp_err_t i2c_dev_read(const i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size)
{
    if (!dev || !in_data || !in_size) return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (out_data && out_size)
    {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, (void *)out_data, out_size, true);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | 1, true);
    i2c_master_read(cmd, in_data, in_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t res = i2c_master_cmd_begin(dev->port, cmd, I2CDEV_TIMEOUT / portTICK_PERIOD_MS);
    if (res != ESP_OK)
        ESP_LOGE(TAG, "Could not read from device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    i2c_cmd_link_delete(cmd);

    return res;
}

esp_err_t i2c_dev_write(const i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size)
{
    if (!dev || !out_data || !out_size) return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    if (out_reg && out_reg_size)
        i2c_master_write(cmd, (void *)out_reg, out_reg_size, true);
    i2c_master_write(cmd, (void *)out_data, out_size, true);
    i2c_master_stop(cmd);
    esp_err_t res = i2c_master_cmd_begin(dev->port, cmd, I2CDEV_TIMEOUT / portTICK_PERIOD_MS);
    if (res != ESP_OK)
        ESP_LOGE(TAG, "Could not write to device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    i2c_cmd_link_delete(cmd);

    return res;
}
