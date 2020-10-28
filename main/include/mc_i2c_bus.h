#ifndef _MC_I2C_BUS_H_
#define _MC_I2C_BUS_H_

#include "iot_i2c_bus.h"

#define ACK_CHECK_EN   				0x1
#define I2C_MASTER_NUM 				I2C_NUM_0   		// I2C port number for master dev */
#define I2C_MASTER_SCL_IO    		32    				// gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    		33    				// gpio number for I2C master data  */
#define I2C_MASTER_FREQ_HZ    		100000     			// I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0   				// I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   				// I2C master do not need buffer */
#define WRITE_BIT  					I2C_MASTER_WRITE 	// I2C master write */
#define READ_BIT   					I2C_MASTER_READ  	// I2C master read */
#define ACK_VAL    					0x0         		// I2C ack value */
#define NACK_VAL   					0x1         		// I2C nack value */
#define I2CDEV_TIMEOUT              1000

typedef struct {
    i2c_port_t port;            // I2C port number
    uint8_t addr;               // I2C address 7-bit
} i2c_dev_t;

extern i2c_bus_handle_t i2c_bus;

void i2c_bus_init();
esp_err_t i2c_dev_read(const i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size);
esp_err_t i2c_dev_write(const i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size);

#endif