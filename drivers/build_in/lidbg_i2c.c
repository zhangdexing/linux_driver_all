#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kthread.h>


int cmn_i2c_write(int i2c_bus_id, u8 dev_address_7bit, u8 *data, int len)
{
    int ret;
    struct i2c_msg msgs[] =
    {
        {
            .addr = dev_address_7bit,
            .flags = 0,//write
            .len = len,
            .buf = data,
        },
    };
    //    for(ret = 0; ret < len; ret++)
    //        printk("=========data[%d][%x]\n", ret, *(data + ret));
    ret = i2c_transfer(i2c_get_adapter(i2c_bus_id), msgs, 1);
    if (ret < 0)
        printk(KERN_CRIT"=========i2c_transfer.err\n");
    return ret;
}
int lpc_send_cmd(u8 *writebuf, int writelen)
{
    int cmdlen = 3 + 1 + writelen;
    int i;
    u8 cmd[cmdlen];

    cmd[cmdlen - 1] = 0;
    printk("%d,%d\n", writelen, sizeof(cmd));
    cmd[0] = 0xff;
    cmd[1] = 0x55;
    cmd[2] = writelen + 1;
    memcpy(&cmd[3], writebuf, writelen);

    for(i = 2; i < cmdlen - 1; i++)
        cmd[cmdlen - 1] += cmd[i];
    return cmn_i2c_write(LPC_I2C_BUS, 0xa0 >> 1, cmd, cmdlen);
}


void lcd_on(void)
{
    u8 buff[] = {0x02, 0x0d, 0x1};
    printk(KERN_CRIT"===lcd_on===\n");
    lpc_send_cmd(buff, sizeof(buff));
}

void lcd_off(void)
{
    u8 buff[] = {0x02, 0x0d, 0x0};
    printk(KERN_CRIT"===lcd_off===\n");
    lpc_send_cmd(buff, sizeof(buff));
}
void lcd_reset_power(void)
{
    u8 buff[] = {0x00, 0xfb};
    printk(KERN_CRIT"===lcd_reset_power===\n");
    lpc_send_cmd(buff, sizeof(buff));
}

void soc_io_output(u32 group, u32 index, bool status)
{
    int err;
    printk(KERN_CRIT"===soc_io_output===\n");
    index += GPIO_MAP_OFFSET;
    err = gpio_request(index, "lidbg_io_output");
    gpio_direction_output(index, status);
    gpio_set_value(index, status);
}


static int thread_wait_i2c(void *data)
{
    int loop = 0;
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);
    
    while(++loop < 3)
    {
        ssleep(1);
        lcd_on();
	soc_io_output(0,GPIO_WP,0);
    }
    return 1;
}
void lidbg_i2c_start(void)
{
    kthread_run(thread_wait_i2c, NULL, "lidbg_i2c_start");
}

static int thread_lcd_off(void *data)
{
    printk(KERN_CRIT"thread_lcd_off+\n");
    lcd_off();
    msleep(1500);
    lcd_reset_power();
    msleep(300);
    lcd_on();
    printk(KERN_CRIT"thread_lcd_off-\n");
    return 1;
}


#define PROC_READ_CHECK {static int len_check = 0;if ((len_check++)%2) return 0;}
int lcd_set_pattern_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{

    PROC_READ_CHECK;
    kthread_run(thread_lcd_off, NULL, "lidbg_lcd_off");
    return 1;
}
static const struct file_operations lcd_p_fops =
{
    .read  = lcd_set_pattern_proc,
};








