#include <platform/gpio.h>
#include "../soc.h"

/*
* dir: 0 in, 1 out
*/
void gpio_set_direction(int pin, int dir)
{
	if(dir == GPIO_INPUT)
		gpio_tlmm_config(pin, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_8MA, GPIO_ENABLE);
	else
		gpio_tlmm_config(pin, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_ENABLE);
}

void gpio_set_val(int pin, int val)
{
	gpio_set_value(pin, val);
}

int gpio_get_val(int pin)
{
	return gpio_status(pin);
}