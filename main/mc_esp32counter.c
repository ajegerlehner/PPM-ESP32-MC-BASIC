/**
 * Name:            mc_esp32counter.c
 * Description:     Counter and interrupt module to replace STM32 microcontroller
 *                  
 * Initial Date:    2023-01-15
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include "sys/time.h"
#include "mc_data.h"
#include "mc_esp32counter.h"

#define TAG "ESP32CNT"

static int msgCount = 0;
IRAM_ATTR CounterMsg counterMsg;
static CounterMsg prevCounterMsg;

int  esp32cnt_hw_revision(void)
{
    int ret = CONFIG_PPM_IOT_HARDWARE_VERSION;

    ESP_LOGI(TAG, "ESP32CNT Hardware revision: %d", ret);

	return ret;
}

void esp32cnt_initialize_counters()
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t timeValms_now = timeValToMs(tv_now);

    prevCounterMsg.c0 = 0;
    prevCounterMsg.c1 = 0;
    prevCounterMsg.c2 = 0;
    prevCounterMsg.c3 = 0;
    prevCounterMsg.c4 = 0;
    prevCounterMsg.c5 = 0;
    prevCounterMsg.elapseMS = 0;
    prevCounterMsg.ttlLevel = 0;
    prevCounterMsg.timeStamp = timeValms_now;
   
    counterMsg.c0 = 0;
    counterMsg.c1 = 0;
    counterMsg.c2 = 0;
    counterMsg.c3 = 0;
    counterMsg.c4 = 0;
    counterMsg.c5 = 0;
    counterMsg.elapseMS = 0;
    counterMsg.ttlLevel = 0;
    counterMsg.timeStamp = timeValms_now;
}

void esp32cnt_read_counters(CounterReading *reading)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t timeValms_now = timeValToMs(tv_now);

	msgCount++;

	reading->counter0 = counterMsg.c0;
	reading->counter1 = counterMsg.c1;
	reading->counter2 = counterMsg.c2;
	reading->counter3 = counterMsg.c3;
	reading->counter4 = counterMsg.c4;
	reading->counter5 = counterMsg.c5;
	reading->delta0 = counterMsg.c0 - prevCounterMsg.c0;
	reading->delta1 = counterMsg.c1 - prevCounterMsg.c1;
	reading->delta2 = counterMsg.c2 - prevCounterMsg.c2;
	reading->delta3 = counterMsg.c3 - prevCounterMsg.c3;
	reading->delta4 = counterMsg.c4 - prevCounterMsg.c4;
	reading->delta5 = counterMsg.c5 - prevCounterMsg.c5;
	reading->elapsedMS = (timeValms_now - prevCounterMsg.timeStamp) / 1000;

    ESP_LOGI(TAG, "ESP32 counter reading: Timestamp: %lld, Prev TimeStamp: %lld, Delta ms: %lld", timeValms_now, prevCounterMsg.timeStamp, (timeValms_now - prevCounterMsg.timeStamp) / 1000);

    prevCounterMsg.c0 = counterMsg.c0;
    prevCounterMsg.c1 = counterMsg.c1;
    prevCounterMsg.c2 = counterMsg.c2;
    prevCounterMsg.c3 = counterMsg.c3;
    prevCounterMsg.c4 = counterMsg.c4;
    prevCounterMsg.c5 = counterMsg.c5;
    prevCounterMsg.timeStamp = timeValms_now;
}

uint64_t timeValToMs(struct timeval tv)
{
    return (int64_t)tv.tv_sec * 1000000L + (int64_t)tv.tv_usec;
}