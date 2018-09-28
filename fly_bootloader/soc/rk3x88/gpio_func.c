#include "../soc.h"

void gpio_set_direction(int pin, int dir)
{
    int group = 0;
    int index = 0;
    int val = 1;

    group = (pin & 0xff00) >> 8;
    index = pin & 0x00ff;

    if(dir == GPIO_INPUT)
        GetPortInput(group, index);
    else
        SetPortOutput(group, index, val);
}

void gpio_set_val(int pin, int val)
{
    int group = 0;
    int index = 0;

    group = (pin & 0xff00) >> 8;
    index = pin & 0x00ff;

    SetPortOutput(group, index, val);
}

int gpio_get_val(int pin)
{
    int group = 0;
    int index = 0;

    group = (pin & 0xff00) >> 8;
    index = pin & 0x00ff;

    static key_config io_config;

    /* set key type to gpio */
    io_config.type = KEY_GPIO;
    io_config.key.gpio.valid = 1;
    io_config.key.gpio.group = group;
    io_config.key.gpio.index = index;
    setup_gpio(&io_config.key.gpio);

    GetPortInput(group, index);

    return GetPortState(&io_config);
}

