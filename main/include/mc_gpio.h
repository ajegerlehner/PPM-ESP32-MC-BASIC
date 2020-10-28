#ifndef _MC_GPIO_H_
#define _MC_GPIO_H_

#define OUTPUT_LED_PIN  5
#define WATCHDOG_LED_PIN 4
#define AZURE_LED_PIN 2
#define MC328_LED_PIN 15
#define RTC_BAT_RED_LED_PIN 19
#define RTC_BAT_GREEN_LED_PIN 21

#define SIGNAL_PIN_DELAY 250

void gpio_init();
void signal_pin(int pin);

#endif