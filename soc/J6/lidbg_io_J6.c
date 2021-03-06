
#include "lidbg.h"

static bool is_first_init=0;
static bool io_ready=1;
static u32 count=0;
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

static struct io_status io_config[IO_LOG_NUM];
static u32 reg_value[REG_NUM];

void set_gpio_hz(u32 phy_reg_addr)
{

    volatile unsigned long *virt_reg_addr;
	u32 reg_vulue;
	if((phy_reg_addr==0x20E0650)|(phy_reg_addr==0x20E0654) |(phy_reg_addr==0x20E06DC)|(phy_reg_addr==0x20E06E0))
	{
		return;
	}
	virt_reg_addr=(unsigned long *)ioremap(phy_reg_addr,4);
	reg_vulue=ioread32(virt_reg_addr);
	reg_value[count++]=reg_vulue;
	reg_vulue&=(~(7<<3));
	iowrite32(reg_vulue,virt_reg_addr);
	iounmap((void *)virt_reg_addr);
}

void reset_gpio_status(u32 phy_reg_addr)
{

    volatile unsigned long *virt_reg_addr;
	if((phy_reg_addr==0x20E0650)|(phy_reg_addr==0x20E0654) |(phy_reg_addr==0x20E06DC)|(phy_reg_addr==0x20E06E0))
	{
		return;
	}
	virt_reg_addr=(unsigned long *)ioremap(phy_reg_addr,4);
	iowrite32(reg_value[count++],virt_reg_addr);
	iounmap((void *)virt_reg_addr);
}


void shutdown_lvds(void)
{

    volatile unsigned long *virt_reg_addr;
	u32 reg_vulue;
	virt_reg_addr=(unsigned long *)ioremap(0x20E0008 ,4);
	reg_vulue=ioread32(virt_reg_addr);
	reg_value[count++]=reg_vulue;
	reg_vulue&=(~(0x1f));
	iowrite32(reg_vulue,virt_reg_addr);
	reg_vulue=ioread32(virt_reg_addr);
	iounmap((void *)virt_reg_addr);
}

void poweron_lvds(void)
{

    volatile unsigned long *virt_reg_addr;
	u32 reg_vulue;
	virt_reg_addr=(unsigned long *)ioremap(0x20E0008 ,4);
	iowrite32(reg_value[count++],virt_reg_addr);
	reg_vulue=ioread32(virt_reg_addr);
	iounmap((void *)virt_reg_addr);
}


void shutdown_hdmi(void)
{

    volatile unsigned long *virt_reg_addr;
	u32 reg_vulue;
	virt_reg_addr=(unsigned long *)ioremap(0x123000 ,4);
	reg_vulue=ioread8(virt_reg_addr);
	lidbg("hdmi phy conf0 value is start 0x%x",reg_vulue);
	reg_vulue&=(~(0x04));
	iowrite8(reg_vulue,virt_reg_addr);
	reg_vulue=ioread8(virt_reg_addr);
	lidbg("hdmi phy conf0 value is stop 0x%x",reg_vulue);
	iounmap((void *)virt_reg_addr);
}

//set all gpio_hz except for memery and flash and cpu debug uart
void set_all_gpio_hz(void)
{
	u32 i;
	u32 size;
	size=(0x20E0508-0x20E004C)/4;
	for(i=0;i<size;i++)
		set_gpio_hz(0x20E004C+i*4);
	size=(0x20E0700-0x20E05C8)/4;
	for(i=0;i<size;i++)
		set_gpio_hz(0x20E05C8+i*4);
	size=(0x20E0744-0x20E0724)/4;
	for(i=0;i<size;i++)
		set_gpio_hz(0x20E0724+i*4);
}

void reset_all_gpio_status(void)
{
	u32 i;
	u32 size;
	size=(0x20E0508-0x20E004C)/4;
	for(i=0;i<size;i++)
		reset_gpio_status(0x20E004C+i*4);
	size=(0x20E0700-0x20E05C8)/4;
	for(i=0;i<size;i++)
		reset_gpio_status(0x20E05C8+i*4);
	size=(0x20E0744-0x20E0724)/4;
	for(i=0;i<size;i++)
		reset_gpio_status(0x20E0724+i*4);
}


int soc_io_suspend(void)
{
	int i;
    DUMP_FUN;
    for( i = 0; i < IO_LOG_NUM; i++)
    if(io_config[i].gpio != 0)
    {
      gpio_direction_input(io_config[i].gpio);
    }

	count=0;
	shutdown_lvds();
//	shutdown_hdmi();
	set_all_gpio_hz();
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
             // gpio_pull_updown(io_config[i].gpio, io_config[i].pull);
            }
        }
	count=0;
	poweron_lvds();
	reset_all_gpio_status();
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


    //    lidbg("gpio_request:index %d\n" , index);

        err = gpio_request(index, "lidbg_io");
        if (err)
        {
            lidbg("\n\nerr: gpio request failed1 %d!!!!!!\n\n\n",index);
            gpio_free(index);
            err = gpio_request(index, "lidbg_io");
            lidbg("\n\nerr: gpio request failed2 %d!!!!!!\n\n\n",index);
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
    if(io_ready == 0)  {lidbg("%d,%d io not ready\n",group,index);return 0;}
	
    gpio_direction_output(index, status);
    gpio_set_value(index, status);
    return 1;

}

bool soc_io_input( u32 index)
{
	if(io_ready == 0)  {lidbg("%d io not ready\n",index);return 1;}
	//lidbg("get imx6q gpio\n");
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

