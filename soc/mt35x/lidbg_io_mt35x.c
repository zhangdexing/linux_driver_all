
#include "lidbg.h"

#define GPIO_OFFSET 0
#define REG_NUM 450

struct io_status
{
    u32 gpio;
    u32 direction;
    u32 pull;
    bool force_reconfig;
    bool out_mod;
    u32 suspend_mod;
	u32 reg_addr;
};
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

unsigned int mt_gpio_remap_to_irq(unsigned int gpio)
{
	struct device_node *node;
	int node_irq=-1;
	char string[50];
	snprintf(string, 50, "lidbg,gpio_%d",gpio);
	node= of_find_compatible_node(NULL, NULL,string);
    if(node){
		node_irq=irq_of_parse_and_map(node, 0);
	}
	return node_irq;
}

static bool io_ready=1;
static struct io_status io_config[IO_LOG_NUM];

int soc_io_suspend(void)
{
	disable_irq(GPIO_TO_INT(6));
#if 0
	int i;
    DUMP_FUN;
	io_ready = 0;
    for( i = 0; i < IO_LOG_NUM; i++)
    if(io_config[i].gpio != 0)
    {
      gpio_direction_input(io_config[i].gpio);
    }
#endif
    return 0;
}

int soc_io_resume(void)
{
	enable_irq(GPIO_TO_INT(6));
#if 0
    int i;
    DUMP_FUN;
	io_ready = 1;
    for(i = 0; i  < IO_LOG_NUM; i++)
        if(io_config[i].gpio != 0)
        {
            if(io_config[i].direction == GPIO_CFG_OUTPUT)
                soc_io_output(0, io_config[i].gpio, io_config[i].out_mod);
            else
            {
                gpio_direction_input(io_config[i].gpio);
             // gpio_pull_updown(io_config[i].gpio, io_config[i].pull);
            }
        }
#endif
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


int soc_io_irq(struct io_int_config *pio_int_config)//need set to input first?
{
    if (request_irq(pio_int_config->ext_int_num, pio_int_config->pisr, pio_int_config->irqflags /*IRQF_ONESHOT |*//*IRQF_DISABLED*/, "lidbg_irq", pio_int_config->dev ))
    {
        lidbg("request_irq err!\n");
        return 0;
    }
    return 0;
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

        err = gpio_request(index, "lidbg_io");
        if (err)
        {
            lidbg("err: gpio request failed1 %d!!!!!!\n",index);
            gpio_free(index);
            err = gpio_request(index, "lidbg_io");
            lidbg("err: gpio request failed2 %d!!!!!!\n",index);
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
	return 0;
}


int soc_io_output(u32 group, u32 index, bool status)
{
    if(io_ready == 0)  
	{
		lidbg("%d,%d io not ready\n",group,index);
		return 0;
	}
	
    gpio_direction_output(index, status);
    gpio_set_value(index, status);

    return 0;
}

bool soc_io_input( u32 index)
{
	if(io_ready == 0)  
	{
		lidbg("%d io not ready\n",index);
		return 1;
	}

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
EXPORT_SYMBOL(mt_gpio_remap_to_irq);

