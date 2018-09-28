#include "../soc.h"
#include <asm/io.h>
#include <asm/gpio.h>



void gpio_set_direction(int pin, int dir)
{
	mxc_gpio_direction(pin,dir);
/*
	if(dir)
	{
		gpio_direction_output(pin,1);
		//gpio_set_value(pin,1);
	}
	else
		gpio_direction_input(pin);
*/
}

void gpio_set_val(int pin, int val)
{
	gpio_set_value(pin,val);
}

int gpio_get_val(int pin)
{
	return gpio_get_value(pin);
}

