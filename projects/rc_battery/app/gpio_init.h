#ifndef __GPIO_INIT_H
#define __GPIO_INIT_H

#define pair_port 0
#define pair_pin 2

#include <stdint.h>
void pair_gpio_init(void);
extern uint16_t cnt;

#endif /* __GPIO_INIT_H */