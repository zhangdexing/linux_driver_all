#include "dsi83.h"
#include "fly_private.h"

static struct i2c_gpio_dev *dsi83_dev = NULL;

static void dsi83_i2c_config()
{
    int ret = 0;
    int i = 0;
    dprintf(INFO, "><><>< config dsi83 i2c bus ><><><\n");

    gpio_set_direction(DSI83_SDA, GPIO_OUTPUT);
    gpio_set_direction(DSI83_SCL, GPIO_OUTPUT);
    gpio_set_val(DSI83_SDA, 1);
    gpio_set_val(DSI83_SCL, 1);
    dsi83_dev = malloc(sizeof(struct i2c_gpio_dev));
    if (!dsi83_dev)
    {
        return NULL;
    }
    dsi83_dev = memset(dsi83_dev, 0, sizeof(struct i2c_gpio_dev));

    dsi83_dev->name = "dsi83_i2c_gpio";
    dsi83_dev->scl_pin = DSI83_SCL;
    dsi83_dev->sda_pin = DSI83_SDA;
    dsi83_dev->retries = 5;
    dsi83_dev->udelay = 4;
}


uint8_t DSI83_read(char *buf,  unsigned int sub_addr, unsigned int size)
{
    int ret;
    sub_addr = sub_addr & 0xff;
    struct i2c_msg msg_buf[] =
    {
        {DSI83_I2C_ADDR, I2C_M_WR, 1, &sub_addr},
        {DSI83_I2C_ADDR, I2C_M_RD, size, buf}
    };
    ret = bit_xfer(dsi83_dev, msg_buf, 2);
    return ret;
}

uint8_t DSI83_write( char *buf, unsigned int size)
{
    int ret;
    struct i2c_msg msg_buf[] = { {
            DSI83_I2C_ADDR,
            I2C_M_WR, size, buf
        }
    };
    ret  = bit_xfer(dsi83_dev, msg_buf, 1);
    return ret;
}

static int SN65_register_read(unsigned char sub_addr, char *buf)
{
    int ret;
    ret = DSI83_read(buf, sub_addr, 1);
    return ret;
}
static int SN65_register_write( char *buf)
{
    int ret;
    ret = DSI83_write(buf, 2);
    return ret;
}
static int SN65_Sequence_seq4(void)
{
    int ret = 0, i;
    char buf2[2];
    char *buf_piont = NULL;
    buf_piont = dsi83_conf;
    buf2[0] = 0x00;
    buf2[1] = 0x00;
    dprintf(INFO, "dsi83:Sequence 4\n");
    for(i = 0; buf_piont[i] != 0xff ; i += 2)
    {
        ret = SN65_register_write(&buf_piont[i]);
        buf2[0] = buf_piont[i];
        ret = SN65_register_read(buf2[0], &buf2[1]);
        //dprintf(INFO,"register 0x%x=0x%x\n", buf_piont[i], buf_piont[i+1]);

        if(buf2[1] != buf_piont[i + 1])
        {
            if((buf2[0] == 0x0a) && ((buf2[1] & 0x7f) == buf_piont[i + 1]) )
            {

                dprintf( INFO, " Warning regitster(0x%.2x),write(0x%.2x) and read back(0x%.2x) \n", buf_piont[i], buf_piont[i + 1], buf2[1]);

                continue;

            }

            dprintf( INFO, "Warning regitster(0x%.2x),write(0x%.2x) and read back(0x%.2x) Unequal\n", buf_piont[i], buf_piont[i + 1], buf2[1]);
            return -1;
        }
    }
    return ret;
}

static int SN65_Sequence_seq6(void)
{
    int ret;
    char buf2[2];
    buf2[0] = 0x0d;
    buf2[1] = 0x01;
    dprintf(INFO, "dsi83:Sequence 6\n");
    ret = SN65_register_write(buf2);
    return ret;
}
static int SN65_Sequence_seq7(void)
{
    int ret;
    char buf2[2];
    dprintf(INFO, "dsi83:Sequence 7\n");

    buf2[0] = 0x0a;
    buf2[1] = 0x00;
    ret = SN65_register_read(buf2[0], &buf2[1]);
    dprintf("read(0x0a) = 0x%.2x\n", buf2[1]);

    {
        unsigned char k, i = 0, j = 0;
        k = buf2[1] & 0x80;
        while(!k)
        {
            ret = SN65_register_read(buf2[0], &buf2[1]);
            k = buf2[1] & 0x80;
            //dprintf( INFO,"dsi83:Wait for %d,r = 0x%.2x\n",i,buf2[1]);
            i++;
            if(i > 100)
            {
                j++;
                dprintf(INFO, "dsi83:Warning wait time out .. break, N = %d\n", j);
                if(j >= 10)
                    break;
            }
            mdelay(10);;
        }
    }
    return ret;
}

static int SN65_Sequence_seq8(void)  /*seq 8 the bit must be set after the CSR`s are updated*/
{
    int ret;
    char buf2[2];

#if 0
    dsi83_dump_reg();
#endif
    dprintf(INFO, "dsi83:Sequence 8 SKIP SOFT RESET\n");
    buf2[0] = 0x09;
    buf2[1] = 0x01;
//    ret = SN65_register_write(buf2);
    //dprintf(INFO,"write(0x09) = 0x%.2x\n",buf2[1]);

#ifdef DSI83_DEBUG
    mdelay(100);
    dsi83_dump_reg();
#endif
    return ret;
}


#ifdef DSI83_DEBUG
void dsi83_dump_reg(void)
{
    int i;
    unsigned char reg;

    char buf1[2];
    buf1[0] = 0xe5;
    buf1[1] = 0xff;

    for (i = 0; i < 0x3d; i++)
    {
        SN65_register_read(i, &reg);
        dprintf(INFO, "dsi83:Read reg-0x%x=0x%x\n", i, reg);
    }


    /*
    	int ret;
    	SN65_register_read(0xe1, &reg);
    	printk(KERN_CRIT "[LSH]:reg-0xE1=0x%x.\n",reg);
    	SN65_register_read(0xe5, &reg);
    	printk(KERN_CRIT "[LSH]:reg-0xE5=0x%x.\n",reg);
    	printk(KERN_CRIT "*****************************\n");

    	for(i = 0; i < 20; ++i)
    	{

    		ret = SN65_register_write(buf1);
    		if(!ret)
    			printk(KERN_CRIT "[LSH]:write reg 0xe5 error.\n");
    		msleep(50);

    		SN65_register_read(0xe1, &reg);
    		printk(KERN_CRIT "[LSH]:reg-0xE1=0x%x.\n",reg);
    		SN65_register_read(0xe5, &reg);
    		printk(KERN_CRIT "[LSH]:reg-0xE5=0x%x.\n",reg);

    		printk(KERN_CRIT "------------------------\n");
    		msleep(10);
    	}
    */

}
#endif


static int SN65_devices_read_id(void)
{
    // addr:0x08 - 0x00
    uint8_t id[9] = {0x01, 0x20, 0x20, 0x20, 0x44, 0x53, 0x49, 0x38, 0x35};
    uint8_t chip_id[9];
    int i, j;

    for (i = 0x8, j = 0; i >= 0; i--, j++)
    {
        if(SN65_register_read(i, &chip_id[j]) < 0)
            return -1;
        else
            dprintf(INFO, "%s():reg 0x%x = 0x%x\n", __func__, i, chip_id[j]);
    }

    return memcmp(id, chip_id, 9);

}

static void dsi83_reset(void)
{
    dprintf(INFO, "dsi83_reset\n");
    gpio_set_direction(DSI83_GPIO_EN, GPIO_OUTPUT);
    gpio_set_val(DSI83_GPIO_EN, 0);
    mdelay(200);
    gpio_set_val(DSI83_GPIO_EN, 1);
    mdelay(200);
}

static void dsi83_enable(void)
{
    //SOC_IO_Output(0, DSI83_GPIO_EN, 1);
    //gpio_tlmm_config(DSI83_GPIO_EN, 0, GPIO_OUTPUT, GPIO_NO_PULL,
    //GPIO_8MA, GPIO_ENABLE);
    // gpio_set_value(DSI83_GPIO_EN,1);
    //gpio_set_direction(DSI83_GPIO_EN, GPIO_OUTPUT);
    //gpio_set_val(DSI83_GPIO_EN, 0);
    //mdelay(100);
    //gpio_set_val(DSI83_GPIO_EN, 1);
    //mdelay(50);
    //dsi83_reset();
}

void dsi83_gpio_init()
{
    dsi83_enable();
}


int check_dsi83_init(void)
{
    int ret = 0;
    unsigned char reg = 0;

    char buf[2];
    //dsi83_err_clean();

    ret = SN65_register_read(0xe5, &reg);

    if(((reg != 0x31) && (reg != 0x3d) && (reg != 0x01) && (reg != 0x00)) || ( (ret < 0)))
    {
        dprintf(INFO, "Error::: dis83 check error, reg[0xe5]=0x%x\n", reg);
        return 0;
    }
    else
    {
        dprintf(INFO, "Dis83 check successfully, reg[0xe5]=0x%x\n", reg);
        return 1;
    }

}


void dsi83_err_clean(void)
{
    int ret = 0;
    char buf[2];
    buf[0] = 0xe5;
    buf[1] = 0xff;
    ret = SN65_register_write(buf);
}



void dsi83_init()
{
    int ret = 0;
    int i;
    int cnt = 0;
    int init_cnt = 0;
    dprintf(INFO, "dsi83_init.\n");

    dsi83_i2c_config();
    dsi83_gpio_init();

dsi83_config_start:
    dsi83_reset();
    
    for(i = 0; i < 3; ++i)
    {
        ret = SN65_devices_read_id();
        if (!ret)
            break;
        else
        {
            dprintf(INFO, "dsi83:DSI83 match ID falied,num:%d.\n", i + 1);
            continue;
        }
    }

    if(i == 3)
        return;
    else
        dprintf(INFO, "dsi83:DSI83 match ID success!\n");

    dsi83_err_clean();
    mdelay(100);

    check_dsi83_init();
    ret = SN65_Sequence_seq4();
    if(ret < 0)
    {
        dsi83_reset();
        if(cnt < 3)
        {
            cnt++;
            goto dsi83_config_start;
        }
        else
            dprintf(INFO, "dsi83:DSI83 config failed, something wrong, cnt = %d !\n", cnt);
    }
    mdelay(100);
#ifdef BOOTLOADER_MSM8909
	dprintf(INFO, "Display Init: +\n");
    	target_display_init(device.display_panel);
	dprintf(INFO, "Display Init: -\n");
#endif
    mdelay(100);
    check_dsi83_init();
    ret = SN65_Sequence_seq6();
    if(ret < 0)
        dprintf(INFO, "dsi83:SN65_Sequence_seq6(),err,ret = %d.\n", ret);

    check_dsi83_init();
    SN65_Sequence_seq7();
    mdelay(100);
    ret = SN65_Sequence_seq8();
    if(ret < 0)
        dprintf(INFO, "dsi83:SN65_Sequence_seq8(),err,ret = %d.\n", ret);

    check_dsi83_init();
    dsi83_err_clean();
    check_dsi83_init();
}

