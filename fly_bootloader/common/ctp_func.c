#include "i2c_gpio.h"
#include "../soc.h"
#include "fly_private.h"

static struct i2c_gpio_dev *devp = NULL;

#ifdef CTP_DBG
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_MAX_LENGTH 240
#define GTP_REG_CONFIG_DATA   0x8047

static u8 config_data[GTP_ADDR_LENGTH]
    = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

static u8 configed_data[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH]
    = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
#endif

int ctp_config_index = 0;
void gt811_cb(int config_index);
void gt911_cb(int config_index);
int ctp_la_reset(void);
int ctp_ha_reset(void);

ctp_chip_t ctp_chips[] =
{
    {"gt911", 0x14, 0x814e, ctp_ha_reset, gt911_cb},
    {"gt811", 0x5d, 0x0721, ctp_ha_reset, gt811_cb}
};

unsigned char ctp_read(unsigned char chip_addr, unsigned char *sub_addr, char *buf, unsigned int size)
{
    int ret;
    //	sub_addr = sub_addr & 0xff;
    struct i2c_msg msg_buf[] =
    {
        {chip_addr, I2C_M_WR, 2, sub_addr},
        {chip_addr, I2C_M_RD, size, buf}
    };
    ret = bit_xfer(devp, msg_buf, 2);
    return ret;
}

unsigned char ctp_write(unsigned char chip_addr, char *buf, unsigned int size)
{
    int ret;
    struct i2c_msg msg_buf[] =
    {
        {chip_addr, I2C_M_WR, size, buf},
    };
    ret  = bit_xfer(devp, msg_buf, 1);
    return ret;
}

static void ctp_i2c_config()
{
    int ret = 0;
    int i = 0;
    dprintf(INFO, "><><>< config ctp i2c bus ><><><\n");

    gpio_set_direction(g_bootloader_hw.ctp_info.ctp_sda, GPIO_OUTPUT);
    gpio_set_direction(g_bootloader_hw.ctp_info.ctp_scl, GPIO_OUTPUT);
    devp = malloc(sizeof(struct i2c_gpio_dev));
    if (!devp)
    {
        return NULL;
    }
    devp = memset(devp, 0, sizeof(struct i2c_gpio_dev));

    devp->name = "ctp_i2c_gpio";
    devp->scl_pin = g_bootloader_hw.ctp_info.ctp_scl;
    devp->sda_pin = g_bootloader_hw.ctp_info.ctp_sda;
    devp->retries = 5;
    devp->udelay = 4;
}

void gt811_cb(int config_index)
{
    dprintf(CRITICAL, "Send config for gt811.\n");

    u8 ctp_config[] =
    {
        0x06, 0xA2,
        0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00, 0x01, 0x11, 0x11, 0x11, 0x21, 0x11,
        0x31, 0x11, 0x41, 0x11, 0x51, 0x11, 0x61, 0x11, 0x71, 0x11, 0x81, 0x11, 0x91, 0x11, 0xA1, 0x11,
        0xB1, 0x11, 0xC1, 0x11, 0xD1, 0x11, 0xE1, 0x11, 0xF1, 0x11, 0x07, 0x03, 0x10, 0x10, 0x10, 0x20,
        0x20, 0x20, 0x10, 0x10, 0x0A, 0x48, 0x30, 0x07, 0x03, 0x00, 0x05, 0x58, 0x02, 0x00, 0x04, 0x00,
        0x00, 0x3C, 0x35, 0x38, 0x32, 0x00, 0x00, 0x23, 0x14, 0x05, 0x0A, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x14, 0x10, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
    };

    ctp_write(ctp_chips[config_index].ctp_slave_add, ctp_config, sizeof(ctp_config) / sizeof(ctp_config[0]));
}

void gt911_cb(int config_index)
{
    dprintf(CRITICAL, "Do nothing for gt911.\n");
}

int ctp_la_reset(void)
{
    //reset ctp
    gpio_set_direction(g_bootloader_hw.ctp_info.ctp_rst, GPIO_OUTPUT);
    //	gpio_set(rst_pin, GPIO_OUTPUT);
    gpio_set_val(g_bootloader_hw.ctp_info.ctp_rst, 0);

    gpio_set_direction(g_bootloader_hw.ctp_info.ctp_int, GPIO_OUTPUT);
    //	gpio_set(int_pin, GPIO_OUTPUT);

    mdelay(100);

    gpio_set_val(g_bootloader_hw.ctp_info.ctp_int, 0);
    udelay(200);
    gpio_set_val(g_bootloader_hw.ctp_info.ctp_rst, 1);
    mdelay(100);

    //pul down int_rst
    gpio_set_val(g_bootloader_hw.ctp_info.ctp_int, 0);
    //	gpio_set(gpio, 0);
}

int ctp_ha_reset(void)
{
    //reset ctp
    gpio_set_direction(g_bootloader_hw.ctp_info.ctp_rst, GPIO_OUTPUT);
    //	gpio_set(rst_pin, GPIO_OUTPUT);
    gpio_set_val(g_bootloader_hw.ctp_info.ctp_rst, 0);

    gpio_set_direction(g_bootloader_hw.ctp_info.ctp_int, GPIO_OUTPUT);
    //	gpio_set(int_pin, GPIO_OUTPUT);

    mdelay(100);

    gpio_set_val(g_bootloader_hw.ctp_info.ctp_int, 1);
    udelay(200);
    gpio_set_val(g_bootloader_hw.ctp_info.ctp_rst, 1);
    mdelay(100);

    //pul down int_rst
    gpio_set_val(g_bootloader_hw.ctp_info.ctp_int, 0);
    //	gpio_set(gpio, 0);
}

int ctp_points_get(void)
{
    u8 touch_points_reg[] = {0x81, 0x4E};
    u8 clear_data[] = {0x81, 0x4E, 0x00};

    int points = 0;
    int ctp_addr = 0;
    char points_data[2] = {0};

    ctp_addr = ctp_chips[ctp_config_index].ctp_slave_add;
    clear_data[0] = ctp_chips[ctp_config_index].point_data_add >> 8;
    clear_data[1] = ctp_chips[ctp_config_index].point_data_add & 0xff;
    touch_points_reg[0] = ctp_chips[ctp_config_index].point_data_add >> 8;
    touch_points_reg[1] = ctp_chips[ctp_config_index].point_data_add & 0xff;

    ctp_read(ctp_addr, touch_points_reg, points_data, 2);
    points = points_data[0] & 0xf;
#ifdef BOOTLOADER_IMX6Q
	if(points==0xf)
		points=0;
#endif
    ctp_write(ctp_addr, clear_data, sizeof(clear_data) / sizeof(clear_data[0]));
    //	dprintf(INFO, "<++ points = %d ++>\n", points);

    return points;
}

int ctp_type_get(void)
{
    int ret = 0;
    int i = 0;

    u8 buff0[] = {0, 0};
    u8 buff1[] = {0, 0};

    //mdelay(50);
    ctp_i2c_config();

    for(i = 0; i < (sizeof(ctp_chips) / sizeof(ctp_chips[0])); i++)
    {
        buff0[0] = ctp_chips[i].point_data_add >> 8;
        buff0[1] = ctp_chips[i].point_data_add & 0xff;

        ctp_chips[i].ctp_reset();
        mdelay(100);

        ret = ctp_read(ctp_chips[i].ctp_slave_add, buff0, buff1, 2);
        if(ret == 2)
        {
            dprintf(INFO, "Ctp info: i=%d, chip_add[0x%x], point_add[0x%x], name[%s]\n", \
                    i, ctp_chips[i].ctp_slave_add, ctp_chips[i].point_data_add, ctp_chips[i].name);

            ctp_chips[i].ctp_cb(i);
            ctp_config_index = i;

#ifdef CTP_DBG
            int j = 0;
            ret = ctp_read(ctp_chips[i].ctp_slave_add, config_data, configed_data, 186);

            for(j = 0; j < 188; j++)
                dprintf(INFO, " reg[%x] = %x\n", (0x8047 + j), configed_data[j + 2]);
#endif

            return i;
        }
    }
    dprintf(INFO, "Set ctp to gt911 as default config.\n");
    return 0;
}
