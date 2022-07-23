#ifndef _MC_IO_EXPANDER_HAL_H_
#define _MC_IO_EXPANDER_HAL_H_

#define SIGNAL_PIN_AZURE        0
#define SIGNAL_PIN_MC           1
#define SIGNAL_PIN_ENVS         2
#define SIGNAL_PIN_WDE          3
#define SIGNAL_PIN_WDT          4
#define SIGNAL_PIN_PLCOUT       5
#define SIGNAL_PIN_LOWBAT_GREEN 6
#define SIGNAL_PIN_LOWBAT_RED   7


void io_expander_init();
void io_expander_signal_boot_end();
void io_expander_signal_pin(int pin);
void io_expander_set_pin(int _pin, int _val);

#endif
