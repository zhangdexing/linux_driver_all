
#include "lidbg.h"

static bool is_first_init = 0;
static bool io_ready = 1;

int soc_io_suspend(void)
{
    is_first_init = 0;
    return 0;
}
int soc_io_resume(void)
{
    is_first_init = 1;
    return 0;
}

int io_free_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    lidbg("%s:enter\n", __func__);
    return 1;
}

int io_request_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    lidbg("%s:enter\n", __func__);
    return 1;
}

void soc_io_init(void)
{

}


void soc_irq_disable(unsigned int irq)
{
    disable_irq(irq);

}

void soc_irq_enable(unsigned int irq)
{
    enable_irq(irq);
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


int soc_io_suspend_config(u32 index, u32 direction, u32 pull, u32 drive_strength)
{
    return -1;
}

int soc_io_config(u32 index, int func, u32 direction,  u32 pull, u32 drive_strength, bool force_reconfig)
{
    return -1;
}


int soc_io_output(u32 group, u32 index, bool status)
{

    if(io_ready == 0)
    {
        lidbg("%d,%d io not ready\n", group, index);
        return 0;
    }

    gpio_direction_output(index, status);
    gpio_set_value(index, status);
    return 1;

}

bool soc_io_input( u32 index)
{
    if(io_ready == 0)
    {
        lidbg("%d io not ready\n", index);
        return 1;
    }
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

