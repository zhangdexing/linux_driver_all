
#include "lidbg.h"
#define GPIO_OFFSET 160

struct io_status
{
    u32 gpio;
    u32 direction;
    u32 pull;
    bool force_reconfig;
    bool out_mod;
    u32 suspend_mod;
};
static struct io_status io_config[IO_LOG_NUM];

void soc_irq_enable(unsigned int irq)
{
    enable_irq(irq);
}

void soc_irq_disable(unsigned int irq)
{
    disable_irq(irq);

}

int soc_io_irq(struct io_int_config *pio_int_config)//need set to input first?
{
    if (request_irq(pio_int_config->ext_int_num, pio_int_config->pisr, pio_int_config->irqflags /*IRQF_ONESHOT |*//*IRQF_DISABLED*/, "lidbg_irq", pio_int_config->dev ))
    {
        lidbg("request_irq err!\n");
        return 0;
    }
    return 1;
}


int soc_io_suspend(void)
{
    int i;
    DUMP_FUN;
    for( i = 0; i < IO_LOG_NUM; i++)
        if(io_config[i].gpio != 0)
        {
            gpio_direction_input(io_config[i].gpio);
            gpio_pull_updown(io_config[i].gpio, GPIO_CFG_NO_PULL);
        }
    return 0;
}

int soc_io_resume(void)
{
    int i;
    DUMP_FUN;
    for(i = 0; i  < IO_LOG_NUM; i++)
        if(io_config[i].gpio != 0)
        {
            if(io_config[i].direction == GPIO_CFG_OUTPUT)
                soc_io_output(0, io_config[i].gpio, io_config[i].out_mod);
            else
            {
                gpio_direction_input(io_config[i].gpio);
                gpio_pull_updown(io_config[i].gpio, io_config[i].pull);
            }
        }
    return 0;
}


void soc_io_init(void)
{
    memset(io_config, 0, sizeof(struct io_status)*IO_LOG_NUM);
}


int soc_io_suspend_config(u32 index, u32 direction, u32 pull, u32 drive_strength)
{
    return -1;
}

int soc_io_config(u32 index, int func, u32 direction, u32 pull, u32 drive_strength, bool force_reconfig)
{
    bool is_first_init = 0;
    is_first_init = (io_config[index - GPIO_OFFSET].gpio == 0) ? 1 : 0;
    if(force_reconfig == 1)
        lidbg("soc_io_config:force_reconfig %d\n" , index);

    if(!is_first_init && (force_reconfig == 0))
    {
        return 1;
    }
    else
    {
        int err;

        if (!gpio_is_valid(index))
            return 0;


        lidbg("gpio_request:index %d\n" , index);

        err = gpio_request(index, "lidbg_io");
        if (err)
        {
            lidbg("\n\nerr: gpio request failed1!!!!!!\n\n\n");
            gpio_free(index);
            err = gpio_request(index, "lidbg_io");
            lidbg("\n\nerr: gpio request failed2!!!!!!\n\n\n");
        }


        if(direction == GPIO_CFG_INPUT)
        {
            err = gpio_direction_input(index);
            if (err)
            {
                lidbg("gpio_direction_set failed\n");
                goto free_gpio;
            }
        }
        index = index - GPIO_OFFSET;
        io_config[index].gpio = index + GPIO_OFFSET;
        io_config[index].direction = direction;
        io_config[index].pull = pull;
        io_config[index].suspend_mod = 0;
        return 1;

free_gpio:
        if (gpio_is_valid(index))
            gpio_free(index);
        return 0;
    }
}



int soc_io_output(u32 group, u32 index, bool status)
{
    gpio_direction_output(index, status);
    gpio_set_value(index, status);
    index = index - GPIO_OFFSET;
    io_config[index].out_mod = status;
    return 1;
}

bool soc_io_input( u32 index)
{
    gpio_direction_input(index);
    return gpio_get_value(index);
}


EXPORT_SYMBOL(soc_io_output);
EXPORT_SYMBOL(soc_io_input);
EXPORT_SYMBOL(soc_io_irq);
EXPORT_SYMBOL(soc_irq_enable);
EXPORT_SYMBOL(soc_irq_disable);
EXPORT_SYMBOL(soc_io_config);
EXPORT_SYMBOL(soc_io_suspend_config);
EXPORT_SYMBOL(soc_io_suspend);
EXPORT_SYMBOL(soc_io_resume);

