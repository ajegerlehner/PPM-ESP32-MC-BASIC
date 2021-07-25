/**
 * Name:            mc_vl6081x.c
 * Description:     Driver for the VL6081X proximity time of flight sensor chip
 * Initial Date:    2021-05-15
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/i2c.h"
#include "mc_vl6180x.h"

#define TAG "VL6180X"
#define VL6180X_DEVICE_ID 0xb4

 const int16_t IDENTIFICATION__MODEL_ID                 = 0x0000;   // Identification model id register, should always return 0xB4
 const int16_t RESULT__RANGE_VAL                        = 0x0062;   // Range value in mm
 const int16_t SYSRANGE__START                          = 0x0018;   // Start range measurement
 const int16_t RESULT__RANGE_STATUS                     = 0x004D;   // Range measurement status
 const int16_t SYSRANGE__THRESH_HIGH                    = 0x0019;   // Sys range threshold high
 const int16_t SYSRANGE__THRESH_LOW                     = 0x0020;   // Sys range threshold low  
 const int16_t RESULT__INTERRUPT_STATUS_GPIO            = 0x004F;
 const int16_t SYSTEM__INTERRUPT_CLEAR                  = 0x0015;
 const int16_t SYSRANGE__CROSSTALK_VALID_HEIGHT         = 0x0021;
 const int16_t SYSRANGE__CROSSTALK_COMPENSATION_RATE    = 0x001E;
 const int16_t SYSRANGE_PART_TO_PART_RANGE_OFFSET       = 0x0024;
 const int16_t RESULT__RANGE_RAW                        = 0x0064;   // 8 bit     
 const int16_t RESULT__RANGE_RETURN_RATE                = 0x0066;   // 9.7 format
 const int16_t GESTURE_THRESHOLD_MIN                    = 20;       // calibarated gesture detection threshold
 const int16_t GESTURE_THRESHOLD_MAX                    = 50;       // calibarated gesture detection threshold

i2c_dev_t	vl6180x_dev;

void vl6180x_init()
{
	int ret;
    uint8_t statusReg;

	// initialize vl6081x i2c device
	vl6180x_dev.port = I2C_MASTER_NUM;
	vl6180x_dev.addr = VL6180X_ADDR;
	
	ESP_LOGI(TAG, "Device initializing...");

	// read device identification
	ret = i2c_dev_read(&vl6180x_dev, &IDENTIFICATION__MODEL_ID , 2, &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not retrieve device identification.");
	}

    if (statusReg != VL6180X_DEVICE_ID)
    {
        ESP_LOGE(TAG, "Returned device identification is: %x, expected identification is: %x", statusReg, VL6180X_DEVICE_ID);
        return;
    }

    ESP_LOGI(TAG, "Device correctly identified, id: %x hex", statusReg);

    ESP_LOGI(TAG, "Device initialization completed.");

    vl61980x_setup();
}

void vl61980x_setup()
{
    ESP_LOGI(TAG, "Setting startup device registers...");

    // setup device registers according to ST application note
    // this applies the standard range settings
    write_data_byte(0x0207, 0x01);
    write_data_byte(0x0208, 0x01);
    write_data_byte(0x0096, 0x00);
    write_data_byte(0x0097, 0xfd);
    write_data_byte(0x00e3, 0x00);
    write_data_byte(0x00e4, 0x04);
    write_data_byte(0x00e5, 0x02);
    write_data_byte(0x00e6, 0x01);
    write_data_byte(0x00e7, 0x03);
    write_data_byte(0x00f5, 0x02);
    write_data_byte(0x00d9, 0x05);
    write_data_byte(0x00db, 0xce);
    write_data_byte(0x00dc, 0x03);
    write_data_byte(0x00dd, 0xf8);
    write_data_byte(0x009f, 0x00);
    write_data_byte(0x00a3, 0x3c);
    write_data_byte(0x00b7, 0x00);
    write_data_byte(0x00bb, 0x3c);
    write_data_byte(0x00b2, 0x09);
    write_data_byte(0x00ca, 0x09);
    write_data_byte(0x0198, 0x01);
    write_data_byte(0x01b0, 0x17);
    write_data_byte(0x01ad, 0x00);
    write_data_byte(0x00ff, 0x05);
    write_data_byte(0x0100, 0x05);
    write_data_byte(0x0199, 0x05);
    write_data_byte(0x01a6, 0x1b);
    write_data_byte(0x01ac, 0x3e);
    write_data_byte(0x01a7, 0x1f);
    write_data_byte(0x0030, 0x00);

    // Recommended : Public registers - See ST application notes

    write_data_byte(0x0011, 0x10);       // Enables polling for 'New Sample ready' when measurement completes
    write_data_byte(0x010a, 0x30);       // Set the averaging sample period (compromise between lower noise and increased execution time)
    write_data_byte(0x003f, 0x46);       // Sets the light and dark gain (upper nibble). Dark gain should not be changed.
    write_data_byte(0x0031, 0xFF);       // sets the # of range measurements after which auto calibration of system is performed
    write_data_byte(0x0040, 0x63);       // Set ALS integration time to 100ms
    write_data_byte(0x002e, 0x01);       // perform a single temperature calibration of the ranging sensor
    write_data_byte(0x001b, 0x09);       // Set default ranging inter-measurement period to 100ms
    write_data_byte(0x003e, 0x31);       // Set default ALS inter-measurement period to 500ms
    write_data_byte(0x0014, 0x24);       // Configures interrupt on 'New Sample Ready threshold event'
}

uint8_t vl6180x_gesture()
{
    uint8_t response;
    uint8_t raw_range;

    while ((read_data_byte(RESULT__RANGE_STATUS) & 0x01) == 0);
    
    // start range measurement
    write_data_byte(SYSRANGE__START, 0x01);

    // wait for operation to complete
    while ((read_data_byte(RESULT__INTERRUPT_STATUS_GPIO) & 0x04) == 0);

    response = read_data_byte(RESULT__RANGE_VAL);
    raw_range = read_data_byte(RESULT__RANGE_RAW);


    // clear interrupt
    write_data_byte(SYSTEM__INTERRUPT_CLEAR, 0x07);
    
    if ((raw_range > GESTURE_THRESHOLD_MIN) && (raw_range < GESTURE_THRESHOLD_MAX))
    {
        ESP_LOGI(TAG, "Raw range mesurement is: %d", raw_range);
        ESP_LOGI(TAG, "Response range mesurement is: %d", response);
        return 1;
    }

    return 0;
}

void write_data_byte(uint16_t reg, uint8_t val)
{
    uint8_t cmd[3];
    uint8_t statusReg;
    int ret;

    cmd[0] = (reg >> 8) & 0xff;
    cmd[1] = (reg & 0xff);
    cmd[2] = val;

	ret = i2c_dev_read(&vl6180x_dev, cmd , sizeof(cmd), &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not write data %x", reg);
	}
}

uint8_t read_data_byte(uint16_t reg)
{
    uint8_t cmd[2];
    uint8_t statusReg;
    int ret;

    cmd[0] = (reg >> 8) & 0xff;
    cmd[1] = (reg & 0xff);

	ret = i2c_dev_read(&vl6180x_dev, cmd , sizeof(cmd), &statusReg, sizeof(statusReg));
	if (ret == ESP_FAIL) 
	{
		ESP_LOGE(TAG, "Could not read data %x", reg);
	}

    return statusReg;
}
