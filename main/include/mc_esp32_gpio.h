#ifndef _MC_ESP32_GPIO_H_
#define _MC_ESP32_GPIO_H_

// ESP32 direct GPIO pins main board
#define ESP32_AZURE_PIN          13     // IO13
#define ESP32_WDT_PIN            12     // IO12
#define ESP32_PLCOUT_PIN         14     // IO14

// ESP32 direct GPIO pins for counter signals
#define ESP32_MCIN0             5       // IO5
#define ESP32_MCIN1             16      // IO16
#define ESP32_MCIN2             4       // IO4
#define ESP32_MCIN3             15      // IO15
#define ESP32_MCIN4             2       // IO2
#define ESP32_MCIN5             35      // IO35

void esp32_init_signal_gpio();
void esp32_init_counter_gpio();
void esp32_set_pin(int _pin, int _pinValue);
void esp32_signal_pin(int _pin, int _delay);
void esp32_signal_boot_end();

#endif  