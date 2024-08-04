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
    // setup NVS RESET input pin
    gpio_pad_select_gpio(NVS_RESET_PIN);
    gpio_set_direction(NVS_RESET_PIN, GPIO_MODE_INPUT);

}

int check_nvs_reset_pin()
{
    int ret;

    ret = gpio_get_level(NVS_RESET_PIN);

    return ret;
}

void signal_pin(int pin)
{
    gpio_set_level(pin, 1);
    vTaskDelay (SIGNAL_PIN_DELAY / portTICK_PERIOD_MS);
    gpio_set_level(pin, 0);
}