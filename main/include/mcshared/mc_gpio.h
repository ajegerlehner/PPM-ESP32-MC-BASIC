#ifndef _MC_GPIO_H_
#define _MC_GPIO_H_

#define NVS_RESET_PIN  36
#define AC2_INT_PIN 39

#define SIGNAL_PIN_DELAY 250

void gpio_init();
void signal_pin(int pin);
int check_nvs_reset_pin();

#endif