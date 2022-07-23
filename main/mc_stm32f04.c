/**
 * Name:            mc_stm32f04.c
 * Description:     Driver for the STM32F303 chip
 * Initial Date:    2020-06-12
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/uart.h"
#include "mc_data.h"
#include "mc_stm32f04.h"

#define TAG  "STM32F303"
#define ONE_SECOND_DELAY (1000 / portTICK_PERIOD_MS)

#define UART_PIN_RX (GPIO_NUM_16)
#define UART_PIN_TX (GPIO_NUM_4)
#define UART_PIN_RTS (UART_PIN_NO_CHANGE)
#define UART_PIN_CTS (UART_PIN_NO_CHANGE)
#define UART_BUF_SIZE	1024

static int msgCount = 0;

esp_err_t stm32_uart_install(void)
{
	esp_err_t ret;	
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};

	
	ret = uart_param_config(UART_NUM_2, &uart_config);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set UART parm config. Error code:  %d STM32F303 UART.", ret);	
	}
	ret = uart_set_pin(UART_NUM_2, UART_PIN_TX, UART_PIN_RX, UART_PIN_RTS, UART_PIN_CTS);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set UART pins. Error code:  %d STM32F303 UART.", ret);	
	}

	ret = uart_driver_install(UART_NUM_2, UART_BUF_SIZE, 0, 0, NULL, 0);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to install UART driver. Error code:  %d STM32F303 UART.", ret);	
	}

	return ret;
}

int  stm32_hw_revision_uart(void)
{
	uint8_t cmdBuffer[1];
	uint8_t respBuffer[2];
	int len;
	int ret;

	cmdBuffer[0] = STM32F04_CMD_WHWREV;
	len = uart_write_bytes(UART_NUM_2, (const char *) cmdBuffer, sizeof(cmdBuffer));
	if (len == 0)
	{
		ESP_LOGE(TAG, "Failed to write %d to STM32F303 UART.", STM32F04_CMD_WHWREV);	
	}

	len = uart_read_bytes(UART_NUM_2, respBuffer, sizeof(respBuffer), 20 / portTICK_RATE_MS);
	if (len == 0)
	{
		ESP_LOGE(TAG, "Failed to read %d from STM32F303 UART.", STM32F04_CMD_WHWREV);	
	}
	
	ret = respBuffer[0] << 8 | respBuffer[1];

	msgCount++;
    ESP_LOGI(TAG, "STM32F303 Hardware revision: %d, Msg no. %d", ret, msgCount);

	return ret;
}

uint32_t stm32_hw_unique_id_uart(void)
{
	uint8_t cmdBuffer[1];
	uint8_t respBuffer[4];
	int len;

	cmdBuffer[0] = STM32F04_CMD_DEVID;
	len = uart_write_bytes(UART_NUM_2, (const char *) cmdBuffer, sizeof(cmdBuffer));
	if (len == 0)
	{
		ESP_LOGE(TAG, "Failed to write %d to STM32F303 UART.", STM32F04_CMD_DEVID);	
	}

	len = uart_read_bytes(UART_NUM_2, respBuffer, sizeof(respBuffer), 20 / portTICK_RATE_MS);
	if (len == 0)
	{
		ESP_LOGE(TAG, "Failed to read %d from STM32F303 UART.", STM32F04_CMD_DEVID);	
	}
	
	msgCount++;
    ESP_LOGI(TAG, "STM32F303 Hardware unique id: %d, Msg no. %d",(uint32_t) respBuffer, msgCount);

	return (uint32_t)((respBuffer[3]<<24) | (respBuffer[2]<<16) | (respBuffer[1]<<8) | respBuffer[0]);

}


void stm32_read_counters_uart(CounterReading *reading)
{
	int len;
	CounterMsg counterMsg;		
	uint8_t cmdBuffer[1];

	ESP_LOGI(TAG, "Reading STM32F04 sampling data... expecting %d bytes", sizeof(counterMsg));
	
	cmdBuffer[0] = STM32F04_CMD_COUNT;
	len = uart_write_bytes(UART_NUM_2, (const char *) cmdBuffer, sizeof(cmdBuffer));
	if (len == 0)
	{
		ESP_LOGE(TAG, "Failed to write %d to STM32F04 UART.", STM32F04_CMD_COUNT);	
	}

	len = uart_read_bytes(UART_NUM_2, (uint8_t *)&counterMsg, sizeof(counterMsg), 20 / portTICK_RATE_MS);
	if (len == 0)
	{
		ESP_LOGE(TAG, "Failed to read %d from STM32F04 UART.", STM32F04_CMD_COUNT);	
	}
	else
	{
		msgCount++;
        ESP_LOGI(TAG, "Counter Msg Count: %li, %li, %li, %li, %li, %li, Msg no. %d", counterMsg.c0, counterMsg.c1, counterMsg.c2, counterMsg.c3, counterMsg.c4, counterMsg.c5, msgCount);
        ESP_LOGI(TAG, "Counter Msg Delta: %li, %li, %li, %li, %li, %li, %li ms", counterMsg.delta0, counterMsg.delta1, counterMsg.delta2, counterMsg.delta3, counterMsg.delta4, counterMsg.delta5, counterMsg.elapseMS);
        ESP_LOGI(TAG, "Counter Msg TTL L: %d, %x %x", counterMsg.ttlLevel, counterMsg.command, counterMsg.term);
		reading->counter0 = counterMsg.c0;
		reading->counter1 = counterMsg.c1;
		reading->counter2 = counterMsg.c2;
		reading->counter3 = counterMsg.c3;
		reading->counter4 = counterMsg.c4;
		reading->counter5 = counterMsg.c5;
		reading->delta0 = counterMsg.delta0;
		reading->delta1 = counterMsg.delta1;
		reading->delta2 = counterMsg.delta2;
		reading->delta3 = counterMsg.delta3;
		reading->delta4 = counterMsg.delta4;
		reading->delta5 = counterMsg.delta5;
		reading->elapsedMS = counterMsg.elapseMS;
	}
}

