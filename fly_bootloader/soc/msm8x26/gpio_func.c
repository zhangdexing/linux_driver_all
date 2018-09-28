#include "../soc.h"
#include <platform/gpio.h>
/*
* dir: 0 in, 1 out
*/
void gpio_set_direction(int pin, int dir)
{
    if(dir == GPIO_INPUT)
        gpio_tlmm_config(pin, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA, GPIO_ENABLE);
    else
        gpio_tlmm_config(pin, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_DISABLE);
}

void gpio_set_val(int pin, int val)
{
#if (defined BOOTLOADER_MSM8909) || (defined BOOTLOADER_MSM8996)
    if(val == 0)
        gpio_set(pin, 0);
    else
        gpio_set(pin, 2);
#else
    gpio_set_value(pin, val);
#endif
}

int gpio_get_val(int pin)
{
    return gpio_status(pin);
}
