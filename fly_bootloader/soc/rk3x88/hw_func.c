#include "../soc.h"
#include "fly_private.h"

#define PMIC_ADDR 0x5a
#define TP_POWER_REG 0x71
#define TP_POWER_REG_VALUE 0xcd
#define PMIC_I2C_SDA 0x011a
#define PMIC_I2C_SCL 0x011b

static struct i2c_gpio_dev *devp = NULL;

unsigned char pmic_write(unsigned char chip_addr, char *buf, unsigned int size)
{
    int ret;
    struct i2c_msg msg_buf[] =
    {
        {chip_addr, I2C_M_WR, size, buf},
    };
    ret  = bit_xfer(devp, msg_buf, 1);
    return ret;
}

unsigned char pmic_read(unsigned char chip_addr, unsigned char *sub_addr, char *buf, unsigned int size)
{
    int ret;
    struct i2c_msg msg_buf[] =
    {
        {chip_addr, I2C_M_WR, 1, sub_addr},
        {chip_addr, I2C_M_RD, size, buf}
    };
    ret = bit_xfer(devp, msg_buf, 2);
    return ret;
}

int tp_power_enable()
{
    u8 buff[] = {0, 0};
    u8 reg_buff = TP_POWER_REG;

    devp = malloc(sizeof(struct i2c_gpio_dev));
    if (!devp)
    {
        return -1;
    }

    devp = memset(devp, 0, sizeof(struct i2c_gpio_dev));
    devp->name = "pmic_i2c_gpio";
    devp->scl_pin = PMIC_I2C_SCL;
    devp->sda_pin = PMIC_I2C_SDA;
    devp->retries = 5;
    devp->udelay = 4;

    buff[0] = TP_POWER_REG;
    buff[1] = TP_POWER_REG_VALUE;
    pmic_write(PMIC_ADDR, buff, 2);

    return 0;
}

void flyaudio_hw_init(void)
{
    int ret;
    ret = tp_power_enable();
    if(ret)
        dprintf(INFO, "tp power init error! \n");

    return;
}
