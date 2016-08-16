#include "../soc.h"
#include <platform/mt_gpio.h>
/*
* dir: 0 in, 1 out
*/
void gpio_set_direction(int pin, int dir)
{
	mt_set_gpio_mode_chip(pin, 0);
        mt_set_gpio_dir_chip(pin, dir);
}

void gpio_set_val(int pin, int val)
{
	mt_set_gpio_out_chip(pin, val);
}

int gpio_get_val(int pin)
{
    return mt_get_gpio_in_chip(pin);
}
