
#include "lidbg.h"
#ifndef PLATFORM_msm8996		
struct msm_gpiomux_config soc_io_config_log[IO_LOG_NUM];
#endif
static bool io_ready = 1;


int soc_io_suspend(void)
{
#ifndef PLATFORM_msm8996		
    int i;
    io_ready = 0;
    for(i = 0; i < IO_LOG_NUM; i++)
    {
        if(soc_io_config_log[i].gpio != 0xffffffff)
        {
#if 0
            int val1, val2;
            val1 = gpio_get_value(soc_io_config_log[i].gpio);
            gpio_free(i);
            val2 = gpio_get_value(soc_io_config_log[i].gpio);

            lidbg("gpio_free:%d,%d,%d\n", soc_io_config_log[i].gpio, val1, val2);
#else
            SWAP2(struct gpiomux_setting *, soc_io_config_log[i].settings[GPIOMUX_ACTIVE], soc_io_config_log[i].settings[GPIOMUX_SUSPENDED]);
            msm_gpiomux_install(&soc_io_config_log[i], 1);
            lidbg("gpio_suspend_set:%d,%d\n", soc_io_config_log[i].gpio, soc_io_config_log[i].settings[GPIOMUX_ACTIVE]->dir);

#endif

        }
    }
#endif

    return 0;
}
int soc_io_resume(void)
{
#ifndef PLATFORM_msm8996	
    int i;
    lidbg("soc_io_resume\n");	
    for(i = 0; i < IO_LOG_NUM; i++)
    {
#if 0
        if(soc_io_config_log[i].gpio != 0xffffffff)
            gpio_request(soc_io_config_log[i].gpio, "lidbg_io");
#else
        if(soc_io_config_log[i].gpio != 0xffffffff)
        {
            SWAP2(struct gpiomux_setting *, soc_io_config_log[i].settings[GPIOMUX_ACTIVE], soc_io_config_log[i].settings[GPIOMUX_SUSPENDED]);
            msm_gpiomux_install(&soc_io_config_log[i], 1);
        }

#endif
    }
#endif		
    io_ready = 1;
    return 0;
}

ssize_t io_free_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    lidbg("%s:enter\n", __func__);
    soc_io_suspend();
    return 1;
}

ssize_t io_request_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    lidbg("%s:enter\n", __func__);
    soc_io_resume();
    return 1;
}

static const struct file_operations io_free_fops =
{
    .read  = io_free_proc,
};

static const struct file_operations io_request_fops =
{
    .read  = io_request_proc,
};

void soc_io_init(void)
{
#ifndef PLATFORM_msm8996		
    memset(soc_io_config_log, 0xff, sizeof(soc_io_config_log));
#endif
    proc_create("io_free", 0, NULL, &io_free_fops);
    proc_create("io_request", 0, NULL, &io_request_fops);
}


void soc_irq_disable(unsigned int irq)
{
    disable_irq(irq);

}

void soc_irq_enable(unsigned int irq)
{
    enable_irq(irq);
}


#if 0
// for test
static irqreturn_t interrupt_isr(int irq, void *dev_id)
{

    soc_irq_enable(irq);
    lidbg("come into IRQ [%d]!\n", irq);

    return 1;
}
#endif


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
#ifndef PLATFORM_msm8996		
    if((soc_io_config_log[index].gpio == 0xffffffff))
    {
        lidbgerr("soc_io_suspend_config ,gpio not config:index %d\n" , index);
        return 1;
    }
    else
    {
        struct gpiomux_setting *lidbg_setting_suspend;

        lidbg_setting_suspend = soc_io_config_log[index].settings[GPIOMUX_SUSPENDED];

        lidbg_setting_suspend->func = GPIOMUX_FUNC_GPIO;
        lidbg_setting_suspend->drv = drive_strength;
        lidbg_setting_suspend->pull = pull;
        lidbg_setting_suspend->dir = direction;

        lidbg("soc_io_suspend_config:index %d\n" , index);
        lidbg("soc_io_suspend_config %d %d %d\n",
              lidbg_setting_suspend->drv,
              lidbg_setting_suspend->pull,
              lidbg_setting_suspend->dir
             );

        //msm_gpiomux_install(&soc_io_config_log[index], 1);

        return 0;
    }
#endif		
	return 0;
}

int soc_io_config(u32 index, int func, u32 direction,  u32 pull, u32 drive_strength, bool force_reconfig)
{
#ifndef PLATFORM_msm8996	
    bool is_first_init = 0;	
    is_first_init = (soc_io_config_log[index].gpio == 0xffffffff) ? 1 : 0;

    if(force_reconfig == 1)
        lidbg("soc_io_config:force_reconfig %d\n" , index);

    if(!is_first_init && (force_reconfig == 0))
    {
        return 1;
    }
    else
    {
        int err;

        struct gpiomux_setting *lidbg_setting_active;
        struct gpiomux_setting *lidbg_setting_suspend;

        if(is_first_init)
        {
            lidbg_setting_active = kmalloc(sizeof(struct gpiomux_setting), GFP_KERNEL);
            lidbg_setting_suspend = kmalloc(sizeof(struct gpiomux_setting), GFP_KERNEL);
        }
        else
        {
            lidbg_setting_active = soc_io_config_log[index].settings[GPIOMUX_ACTIVE];
            lidbg_setting_suspend = soc_io_config_log[index].settings[GPIOMUX_SUSPENDED];
        }

        lidbg_setting_active->func = func;
        lidbg_setting_active->drv = drive_strength;
        lidbg_setting_active->pull = pull;
        lidbg_setting_active->dir = direction;


        lidbg_setting_suspend->func = GPIOMUX_FUNC_GPIO;
        lidbg_setting_suspend->drv = GPIOMUX_DRV_2MA;
        lidbg_setting_suspend->pull = GPIOMUX_PULL_NONE;
        lidbg_setting_suspend->dir = GPIOMUX_IN;


        soc_io_config_log[index].gpio = index;
        soc_io_config_log[index].settings[GPIOMUX_ACTIVE] = lidbg_setting_active ;
        soc_io_config_log[index].settings[GPIOMUX_SUSPENDED] = lidbg_setting_suspend ;

        msm_gpiomux_install(&soc_io_config_log[index], 1);
        index += GPIO_MAP_OFFSET;


        if (!gpio_is_valid(index))
        {
            lidbg("gpio_is_valid fail\n");
            return 0;
        }

        lidbg("gpio_request:index %d\n" , index);
        lidbg("soc_io_config %d %d %d\n",
              lidbg_setting_active->drv,
              lidbg_setting_active->pull,
              lidbg_setting_active->dir
             );

        if(func == GPIOMUX_FUNC_GPIO)
        {
            if(is_first_init)
            {
                err = gpio_request(index, "lidbg_io");
                if (err)
                {
                    lidbg("\n\nerr: gpio request failed1!!!!!!\n\n\n");
                    gpio_free(index);
                    err = gpio_request(index, "lidbg_io");
                    lidbg("\n\nerr: gpio request failed2!!!!!!\n\n\n");
                }
            }

            if(direction == GPIOMUX_IN)
            {
                err = gpio_direction_input(index);
                if (err)
                {
                    lidbg("gpio_direction_set failed\n");
                    goto free_gpio;
                }
            }
        }
        return 1;

free_gpio:
        if (gpio_is_valid(index))
            gpio_free(index);
        return 0;
    }
#endif		
	return 0;
}


int soc_io_output(u32 group, u32 index, bool status)
{

    if(io_ready == 0)
    {
        lidbg("%d,%d io not ready\n", group, index);
        return 0;
    }
#ifndef PLATFORM_msm8996		
    if(status == 1)
        soc_io_config_log[index].settings[GPIOMUX_ACTIVE]->dir = GPIOMUX_OUT_HIGH;
    else
        soc_io_config_log[index].settings[GPIOMUX_ACTIVE]->dir = GPIOMUX_OUT_LOW;
#endif
    index += GPIO_MAP_OFFSET;
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

    index += GPIO_MAP_OFFSET;
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

