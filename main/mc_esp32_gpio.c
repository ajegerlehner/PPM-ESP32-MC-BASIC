/**
 * Name:            mc_esp32_gpio.c
 * Description:     Setup and interface for ESP32 direct GPIO pins
 *                  
 * Initial Date:    2023-01-15
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/


#include "driver/gpio.h"
#include "mc_esp32_gpio.h"
#include "mc_common.h"
#include "mc_esp32counter.h"

#define TAG "ESP32GPIO"
#define SIGNAL_PIN_DELAY  200

extern CounterMsg counterMsg;

static void IRAM_ATTR mc_isr_handler(void *args)
{
    int pinNumber = (int)args;

    switch (pinNumber)
    {
    case ESP32_MCIN0:
        counterMsg.c0++;
        break;
    case ESP32_MCIN1:
        counterMsg.c1++;
        break;
    case ESP32_MCIN2:
        counterMsg.c2++;
        break;
    case ESP32_MCIN3:
        counterMsg.c3++;
        break;
    case ESP32_MCIN4:
        counterMsg.c4++;
        break;
    case ESP32_MCIN5:
        counterMsg.c5++;
        break;    
    default:
        break;
    }
}

void esp32_init_signal_gpio()
{
    gpio_pad_select_gpio(ESP32_PLCOUT_PIN);
    gpio_set_direction(ESP32_PLCOUT_PIN, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(ESP32_AZURE_PIN);
    gpio_set_direction(ESP32_AZURE_PIN, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(ESP32_WDT_PIN);
    gpio_set_direction(ESP32_WDT_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "ESP32 gpio signal pins setup completed.");

}

void esp32_init_counter_gpio()
{
    gpio_config_t config;

    // setup input pins for counter signals
    config.intr_type = GPIO_INTR_POSEDGE;
    config.mode = GPIO_MODE_INPUT;
    config.pull_down_en = 1;
    config.pull_up_en = 0;
    config.pin_bit_mask = ((1ULL<<ESP32_MCIN0) | (1ULL<<ESP32_MCIN1) |(1ULL<<ESP32_MCIN2) |(1ULL<<ESP32_MCIN3) |(1ULL<<ESP32_MCIN4) |(1ULL<<ESP32_MCIN5));

    gpio_config(&config);

    // setup ISR for counter values
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ESP32_MCIN0, mc_isr_handler, (void *)ESP32_MCIN0);
    gpio_isr_handler_add(ESP32_MCIN1, mc_isr_handler, (void *)ESP32_MCIN1);
    gpio_isr_handler_add(ESP32_MCIN2, mc_isr_handler, (void *)ESP32_MCIN2);
    gpio_isr_handler_add(ESP32_MCIN3, mc_isr_handler, (void *)ESP32_MCIN3);
    gpio_isr_handler_add(ESP32_MCIN4, mc_isr_handler, (void *)ESP32_MCIN4);
    gpio_isr_handler_add(ESP32_MCIN5, mc_isr_handler, (void *)ESP32_MCIN5);

    ESP_LOGI(TAG, "ESP32 gpio counter pins setup completed.");
}

void esp32_set_pin(int _pin, int _pinValue)
{
    gpio_set_level(_pin, _pinValue);
    ESP_LOGI(TAG, "ESP32 gpio set pin: %d -> %d", _pin, _pinValue);

}

void esp32_signal_pin(int _pin, int _delay)
{
    esp32_set_pin(_pin, 1);
	vTaskDelay (_delay / portTICK_PERIOD_MS);
	esp32_set_pin(_pin, 0);
}

void esp32_signal_boot_end()
{
    esp32_signal_pin(ESP32_AZURE_PIN, SIGNAL_PIN_DELAY);
    esp32_signal_pin(ESP32_WDT_PIN, SIGNAL_PIN_DELAY);
    esp32_signal_pin(ESP32_PLCOUT_PIN, SIGNAL_PIN_DELAY);
}
