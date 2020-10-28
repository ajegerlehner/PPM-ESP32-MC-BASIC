#ifndef _MC_STM32F04_H_
#define _MC_STM32F04_H_

#include "esp_log.h"
#include "esp_types.h"
#include "driver/gpio.h"

#define STM32F04_CMD_WHWREV	0x0F
#define STM32F04_CMD_COUNT	0x01
#define STM32F04_CMD_DEVID	0x02

typedef struct {
        long elapseMS;
		long c0;
		long c1;
		long c2;
		long c3;
		long c4;
		long c5;
		long delta0;
		long delta1;
		long delta2;
		long delta3;
		long delta4;
		long delta5;
        uint8_t ttlLevel;
		uint8_t	command;
		uint8_t term;
	} CounterMsg;


int stm32_hw_revision_uart(void);
uint32_t stm32_hw_unique_id_uart(void);
esp_err_t stm32_uart_install(void);
void stm32_read_counters_uart(CounterReading *reading);

#endif