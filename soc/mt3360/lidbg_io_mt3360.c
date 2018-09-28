
#include "lidbg.h"
#include "x_bim_83xx.h"

u8 soc_io_config_log[IO_LOG_NUM];

static struct mtk_ext_int ext_int_config[] =
{
    { PIN_35_EINT0, 0, VECTOR_EXT, EINT0_SEL},
    { PIN_36_EINT1, 1, VECTOR_EXT2, EINT1_SEL},
    { PIN_37_EINT2, 2, VECTOR_EXT3, EINT2_SEL},
    { PIN_38_EINT3, 3, VECTOR_EXT4, EINT3_SEL},
    { PIN_70_GPIO70, 4, VECTOR_EXT5, EINT4_SEL},
    { PIN_71_GPIO71, 5, VECTOR_EXT6, EINT5_SEL},
    { PIN_72_GPIO72, 6, VECTOR_EXT7, EINT6_SEL},
    { PIN_42_GPIO42, 7, VECTOR_EXT8, EINT7_SEL},
};

void soc_io_init(void)
{
    memset(soc_io_config_log, 0, IO_LOG_NUM);
}


void soc_irq_disable(unsigned int irq)
{
    int i = 0;
    int ext_int_flag = 0;
    int ext_int_number;

    for(i = 0; i < ARRAY_SIZE(ext_int_config); i++)
        if(irq == ext_int_config[i].ext_int_gpio_num)
        {
            ext_int_number = ext_int_config[i].ext_int_number;
            ext_int_flag = 1;
            break;
        }

    if(ext_int_flag)
    {
        BIM_DisableEInt(ext_int_number);
        ext_int_flag = 0;
    }
    BIM_ClearIrq(irq);
}

void soc_irq_enable(unsigned int irq)
{
    int i = 0;
    int ext_int_flag = 0;
    int ext_int_number;

    for(i = 0; i < ARRAY_SIZE(ext_int_config); i++)
        if(irq == ext_int_config[i].ext_int_gpio_num)
        {
            ext_int_number = ext_int_config[i].ext_int_number;
            ext_int_flag = 1;
            break;
        }

    if(ext_int_flag)
    {
        BIM_EnableEInt(ext_int_number);
        ext_int_flag = 0;
    }
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
    int ext_int_flag = 0;
    int i = 0;

    int ext_int_gpio_num;
    int ext_int_number;
    int vector_irq_num;
    int pinmux_function;

    for(i = 0; i < ARRAY_SIZE(ext_int_config); i++)
        if(pio_int_config->ext_int_num == ext_int_config[i].ext_int_gpio_num)
        {
            ext_int_gpio_num = ext_int_config[i].ext_int_gpio_num;
            ext_int_number = ext_int_config[i].ext_int_number;
            vector_irq_num = ext_int_config[i].vector_irq_num;
            pinmux_function = ext_int_config[i].pinmux_function;

            ext_int_flag = 1;
            break;
        }

    if(ext_int_flag)
    {
        BIM_SetEInt(ext_int_number, pio_int_config->irqflags, 10);
        GPIO_MultiFun_Set(ext_int_gpio_num, pinmux_function);

        //pio_int_config->irqflags = EINT_TYPE_DUALEDGE;
        ext_int_flag = 0;
        printk("***** set ext_int = %d, ext_int_num = %d, irq = %d, irq_flag = %d ******\n", ext_int_gpio_num, ext_int_number, vector_irq_num, (pio_int_config->irqflags >> 10));

        if (request_irq(vector_irq_num, pio_int_config->pisr , pio_int_config->irqflags, "lidbg_irq", pio_int_config->dev ))
        {
            lidbg("request_irq err!\n");
            return 0;
        }

        BIM_EnableEInt(ext_int_number);
    }
    else
    {
        GPIO_MultiFun_Set(pio_int_config->ext_int_num, PINMUX_LEVEL_GPIO_END_FLAG);
        pio_int_config->ext_int_num = GPIO_TO_INT(pio_int_config->ext_int_num);
        //	    pio_int_config->irqflags =  GPIO_IRQTYPE_RISINGEDGE;
        printk("***** set gpio int: gpio_num = %d, irq = %d, irq_flag = %d ******\n", pio_int_config->ext_int_num, pio_int_config->ext_int_num, pio_int_config->irqflags);
        if (request_gpio_irq(pio_int_config->ext_int_num , pio_int_config->pisr , pio_int_config->irqflags , "lidbg_irq", pio_int_config->dev))
        {
            lidbg("request_irq err!\n");
            return 0;
        }
    }
    return 1;
}

int soc_io_suspend_config(u32 index, u32 direction, u32 pull, u32 drive_strength)
{
    return 0;

}
int soc_io_resume_config(u32 index, u32 direction, u32 pull, u32 drive_strength)
{
#define I2C1_SEL                  (192 + 26)
#define I2C0_SEL                  (3)

    int ext_int_flag = 0;
    int i = 0;

    int ext_int_gpio_num;
    int ext_int_number;
    int vector_irq_num;
    int pinmux_function;

    GPIO_MultiFun_Set(113, I2C1_SEL);// for I2C1 not use when resume
    GPIO_MultiFun_Set(118, I2C1_SEL);

    GPIO_MultiFun_Set(112, I2C0_SEL);// for I2C0 not use when resume
    GPIO_MultiFun_Set(117, I2C0_SEL);

    ext_int_gpio_num = ext_int_config[0].ext_int_gpio_num;
    ext_int_number = ext_int_config[0].ext_int_number;
    vector_irq_num = ext_int_config[0].vector_irq_num;
    pinmux_function = ext_int_config[0].pinmux_function;
    BIM_SetEInt(ext_int_number, EINT_TYPE_DUALEDGE, 10);
    GPIO_MultiFun_Set(ext_int_gpio_num, pinmux_function);
    printk("====== set ext_int = %d, ext_int_num = %d, irq = %d ======\n", ext_int_gpio_num, ext_int_number, vector_irq_num);
    BIM_EnableEInt(ext_int_number);

    ext_int_gpio_num = ext_int_config[1].ext_int_gpio_num;
    ext_int_number = ext_int_config[1].ext_int_number;
    vector_irq_num = ext_int_config[1].vector_irq_num;
    pinmux_function = ext_int_config[1].pinmux_function;
    BIM_SetEInt(ext_int_number, EINT_TYPE_DUALEDGE, 10);
    GPIO_MultiFun_Set(ext_int_gpio_num, pinmux_function);
    printk("====== set ext_int = %d, ext_int_num = %d, irq = %d ======\n", ext_int_gpio_num, ext_int_number, vector_irq_num);
    BIM_EnableEInt(ext_int_number);

    ext_int_gpio_num = ext_int_config[2].ext_int_gpio_num;
    ext_int_number = ext_int_config[2].ext_int_number;
    vector_irq_num = ext_int_config[2].vector_irq_num;
    pinmux_function = ext_int_config[2].pinmux_function;
    BIM_SetEInt(ext_int_number, EINT_TYPE_HIGHLEVEL, 500);
    GPIO_MultiFun_Set(ext_int_gpio_num, pinmux_function);
    printk("====== set ext_int = %d, ext_int_num = %d, irq = %d ======\n", ext_int_gpio_num, ext_int_number, vector_irq_num);
    BIM_EnableEInt(ext_int_number);

    ext_int_gpio_num = ext_int_config[4].ext_int_gpio_num;
    ext_int_number = ext_int_config[4].ext_int_number;
    vector_irq_num = ext_int_config[4].vector_irq_num;
    pinmux_function = ext_int_config[4].pinmux_function;
    BIM_SetEInt(ext_int_number, EINT_TYPE_DUALEDGE, 10);
    GPIO_MultiFun_Set(ext_int_gpio_num, pinmux_function);
    printk("====== set ext_int = %d, ext_int_num = %d, irq = %d ======\n", ext_int_gpio_num, ext_int_number, vector_irq_num);
    BIM_EnableEInt(ext_int_number);

    ext_int_gpio_num = ext_int_config[7].ext_int_gpio_num;
    ext_int_number = ext_int_config[7].ext_int_number;
    vector_irq_num = ext_int_config[7].vector_irq_num;
    pinmux_function = ext_int_config[7].pinmux_function;
    BIM_SetEInt(ext_int_number, EINT_TYPE_DUALEDGE, 10);
    GPIO_MultiFun_Set(ext_int_gpio_num, pinmux_function);
    printk("====== set ext_int = %d, ext_int_num = %d, irq = %d ======\n", ext_int_gpio_num, ext_int_number, vector_irq_num);
    BIM_EnableEInt(ext_int_number);

    return 0;
}
int soc_io_config(u32 index, int func, bool direction, u32 pull, u32 drive_strength, bool force_reconfig)
{

    if(soc_io_config_log[index] == 1)
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
            lidbg("\n\nerr: gpio request failed!!!!!!\n\n\n");
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
        soc_io_config_log[index] = 1;

        return 1;

free_gpio:
        if (gpio_is_valid(index))
            gpio_free(index);
        return 0;
    }
}


int soc_io_output(u32 group, u32 index, bool status)
{
    GPIO_MultiFun_Set(index, 0xFF);//config for gpio
    gpio_direction_output(index, status);
    gpio_set_value(index, status);
    return 1;

}

bool soc_io_input( u32 index)
{
    return gpio_get_value(index);
}


EXPORT_SYMBOL(soc_io_output);
EXPORT_SYMBOL(soc_io_input);
EXPORT_SYMBOL(soc_io_irq);
EXPORT_SYMBOL(soc_irq_enable);
EXPORT_SYMBOL(soc_irq_disable);
EXPORT_SYMBOL(soc_io_config);
EXPORT_SYMBOL(soc_io_suspend_config);
EXPORT_SYMBOL(soc_io_resume_config);
