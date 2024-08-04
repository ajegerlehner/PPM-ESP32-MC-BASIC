#ifndef _MC_ESP32COUNTER_H_
#define _MC_ESP32COUNTER_H_

#include "esp_log.h"
#include "esp_types.h"
#include "driver/gpio.h"
#include "mc_data.h"
#include "sys/time.h"


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
        uint64_t timeStamp;
	} CounterMsg;

int  esp32cnt_hw_revision(void);    
void esp32cnt_initialize_counters();
void esp32cnt_read_counters(CounterReading *reading);
uint64_t timeValToMs(struct timeval _tv);

#endif
