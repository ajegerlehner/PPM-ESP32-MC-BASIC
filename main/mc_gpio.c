/**
 * Name:            mc_gpio.c
 * Description:     Manages GPIO related functions
 * Initial Date:    2020-06-09
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mc_gpio.h"

void gpio_init()
{
    // setup LED pins
    gpio_pad_select_gpio(OUTPUT_LED_PIN);
    gpio_set_direction(OUTPUT_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OUTPUT_LED_PIN, 0);

    gpio_pad_select_gpio(WATCHDOG_LED_PIN);
    gpio_set_direction(WATCHDOG_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(WATCHDOG_LED_PIN, 0);

    gpio_pad_select_gpio(AZURE_LED_PIN);
    gpio_set_direction(AZURE_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(AZURE_LED_PIN, 0);

    gpio_pad_select_gpio(MC328_LED_PIN);
    gpio_set_direction(MC328_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MC328_LED_PIN, 0);

    gpio_pad_select_gpio(RTC_BAT_RED_LED_PIN);
    gpio_set_direction(RTC_BAT_RED_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RTC_BAT_RED_LED_PIN, 0);

    gpio_pad_select_gpio(RTC_BAT_GREEN_LED_PIN);
    gpio_set_direction(RTC_BAT_GREEN_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RTC_BAT_GREEN_LED_PIN, 0);
}

void signal_pin(int pin)
{
    gpio_set_level(pin, 1);
    vTaskDelay (SIGNAL_PIN_DELAY / portTICK_PERIOD_MS);
    gpio_set_level(pin, 0);
}