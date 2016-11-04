
#include "lidbg.h"

#include "pca9634.h"

LIDBG_DEFINE;

static BYTE play_switch;

BOOL I2C_Read_PCA9634(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen);
BOOL I2C_Write_PCA9634(BYTE slaveAddr, UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen);
BOOL PCA9634_Init(void);
BOOL PCA9634_Sleep(void);
BOOL PCA9634_Reset(void);

/**
 * PCA9634_RGB_Output - send RGB values to change duty cycle
 * @red: red Individual Duty Cycle(max 255)
 * @green: green Individual Duty Cycle(max 255)
 * @blue: blue Individual Duty Cycle(max 255)
 *
 * send RGB values to change duty cycle.Duty Cycle Range:0~99.6%
 *
 */
void PCA9634_RGB_Output(BYTE red, BYTE green, BYTE blue)
{
    BYTE regRGB[6][2] = {{0x06, 0x00}, {0x07, 0x00}, //R1,R2
        {0x04, 0x00}, {0x05, 0x00}, //G1,G2
        {0x02, 0x00}, {0x03, 0x00}
    };//B1,B2
    BYTE i;

    //lidbg("PCA9634 Output:red=%d,green=%d,blue=%d\n", red,green,blue);

    for(i = 0; i < 6; i++)
    {
        if(i < 2) regRGB[i][1] = red;
        else if(i > 3) regRGB[i][1] = blue;
        else regRGB[i][1] = green;
        if (I2C_Write_PCA9634(PCA9634_I2C_ADDR, regRGB[i][0], &regRGB[i][1], 1) < 0)
        {
            lidbg("PCA9634 RGB_Output error!\n");
            return;
        }
    }
}


/**
 * thread_rgb_play - RGB shade test thread
 * @data: the pointer of data
 *
 * RGB play process,shade between seven main color.
 *
 */
int thread_rgb_play(void *data)
{
    BYTE r = 255, g = 255, b = 255;
    BYTE colorSel, i, colorNum, rgbFlag[3];
    BYTE	color[8][3] = {{255, 255, 255}, //white
        {255, 0, 0}, //red
        {255, 255, 0}, //yellow
        {0, 255, 0}, //green
        {0, 255, 255}, //cyan
        {0, 0, 255}, //blue
        {255, 0, 255}, //magenta
        {255, 255, 255}
    };//white
    //BYTE switch_bit = *(BYTE*)data;
    while(1)
    {
play_start:
        for(colorSel = 0; colorSel < 8; colorSel++)
        {
            PCA9634_Init();//fix i2c bug
            memset(rgbFlag, 0, sizeof(rgbFlag)); //clear flag
            pr_debug("-------colorSel = [%d]-------", colorSel);
            for(i = 0; i < 3; i++) //confirm changes
            {
                if(colorSel == 7) goto play_start;//index end
                if(color[colorSel][i] > color[colorSel + 1][i]) rgbFlag[i] = 1; //current > next
                else if(color[colorSel][i] < color[colorSel + 1][i]) rgbFlag[i] = 2; //current < next
            }
            for(colorNum = 0; colorNum < 255; colorNum++)
            {
                pr_debug("-------colorNum = [%d]-------", colorNum);
                if(rgbFlag[0] == 1) r--;
                else if(rgbFlag[0] == 2) r++;
                if(rgbFlag[1] == 1) g--;
                else if(rgbFlag[1] == 2) g++;
                if(rgbFlag[2] == 1) b--;
                else if(rgbFlag[2] == 2) b++;
                if(!play_switch) return 1;//stop
                if(colorNum % 3) continue; //output each 3 counts
                PCA9634_RGB_Output(r, g, b);
                msleep(5);//reduce system mem load
            }
        }
    }
    return 0;
}

/**
 * pca9634_open - open node function
 * @inode:inode
 * @filp:node file struct
 *
 * open node function.
 *
 */
int pca9634_open (struct inode *inode, struct file *filp)
{
    return 0;
}

/**
 * pca9634_write - write node function
 * @filp:node file struct
 * @buf:user buffer
 * @count:bytes count
 * @f_pos:file pos
 *
 * write node function.Accpet user space cmd.
 *
 */
ssize_t pca9634_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char *cmd[8] = {NULL};
    int cmd_num  = 0;
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';
    lidbg("-----FLYSTEP------------------[%s]---\n", cmd_buf);

    cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;

    lidbg("-----cmd_num------------[%d]---\n", cmd_num);

    if(!strcmp(cmd[0], "rgb"))
    {
        int red, green, blue;
        lidbg("case:[%s]\n", cmd[0]);
        if(cmd_num < 4)
        {
            lidbg("RGB args error!");
            return size;
        }
        red = simple_strtoul(cmd[1], 0, 0);
        green = simple_strtoul(cmd[2], 0, 0);
        blue = simple_strtoul(cmd[3], 0, 0);
        PCA9634_RGB_Output(red, green, blue);
    }

    if(!strcmp(cmd[0], "play"))
    {
        play_switch = 0;
        PCA9634_Reset();
        play_switch = 1;
        CREATE_KTHREAD(thread_rgb_play, NULL);
    }

    if(!strcmp(cmd[0], "stop"))
    {
        play_switch = 0;
        PCA9634_Sleep();
    }

    if(!strcmp(cmd[0], "init"))
    {
        PCA9634_Init();
    }

    if(!strcmp(cmd[0], "reset"))
    {
        PCA9634_Reset();
    }

    return size;
}

static  struct file_operations pca9634_fops =
{
    .owner = THIS_MODULE,
    .open = pca9634_open,
    .write = pca9634_write,
};


/**
 * I2C_Read_PCA9634 - PCA9634 I2C read
 * @ulRegAddr:I2C register address
 * @pRegValBuf:data for register
 * @uiValBufLen:data length
 *
 * PCA9634 I2C read.One byte register address.
 *
 */
BOOL I2C_Read_PCA9634(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
    BYTE i;
    lidbg("Enter %s().\n", __func__);
    if (SOC_I2C_Rec(PCA9634_I2C_BUS, PCA9634_I2C_ADDR, ulRegAddr, pRegValBuf, uiValBufLen) < 0)
    {

        lidbg("PCA9634 IIC read error!\n");
        return FALSE;
    }

    lidbg("Flyaudio PCA9634 IIC Read-->\n");
    for (i = 0; i < uiValBufLen + 3; i++)
    {
        lidbg("PCA9634 IIC Read:0x%x\n", pRegValBuf[i]);
    }

    return TRUE;
}


/**
 * I2C_Write_PCA9634 - PCA9634 I2C write
 * @slaveAddr:I2C slave address
 * @ulRegAddr:I2C register address
 * @pRegValBuf:data for register
 * @uiValBufLen:data length
 *
 * PCA9634 I2C write.One byte register address.
 *
 */
BOOL I2C_Write_PCA9634(BYTE slaveAddr, UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
    BYTE buff[100];

    buff[0] = ulRegAddr & 0xff;
    memcpy(&buff[1], pRegValBuf, uiValBufLen);
    if (SOC_I2C_Send(PCA9634_I2C_BUS, slaveAddr, buff, uiValBufLen + 1) < 0)
    {
        lidbg("PCA9634 IIC write error!\n");
        return FALSE;
    }
    return TRUE;
}



/**
 * PCA9634_Init - PCA9634 init
 *
 * 1.change SLEEP bit from Low power mode to Normal mode.
 * 2.change output state from off to PWMx register individual brightness control.(without GRPPWM)
 *
 */
BOOL PCA9634_Init(void)
{
    BYTE regInit[3][2] = {{0x00, 0x01}, {0x0C, 0xAA}, {0x0D, 0x0A}};
    BYTE i;

    pr_debug("PCA9634 Init!\n");

    for(i = 0; i < 3; i++)
    {
        if (I2C_Write_PCA9634(PCA9634_I2C_ADDR, regInit[i][0], &regInit[i][1], 1) < 0)
        {
            lidbg("PCA9634_Init error!\n");
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * PCA9634_Sleep - PCA9634 Sleep
 *
 * 1.change SLEEP bit from Normal mode to Low power mode.
 * 2.change output state to off.
 *
 */
BOOL PCA9634_Sleep(void)
{
    BYTE regInit[3][2] = {{0x0C, 0x0}, {0x0D, 0x0}, {0x00, 0x17}};
    BYTE i;

    lidbg("PCA9634 pwr down!\n");

    for(i = 0; i < 3; i++)
    {
        if (I2C_Write_PCA9634(PCA9634_I2C_ADDR, regInit[i][0], &regInit[i][1], 1) < 0)
        {
            lidbg("PCA9634_sleep error!\n");
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * PCA9634_Reset - PCA9634 Reset
 *
 * sends 2 bytes with 2 specific values.(SWRST data byte 1 and byte 2)
 *
 */
BOOL PCA9634_Reset(void)
{
    BYTE reg[3] = {0xA5, 0x5A};

    lidbg("PCA9634 soft reset!\n");

    if (I2C_Write_PCA9634(PCA9634_RST_I2C_ADDR, reg[0], &reg[1], 1) < 0)
    {
        lidbg("PCA9634_Reset error!\n");
        return FALSE;
    }
    return TRUE;
}

/**
 * rgb_led_probe - probe function
 * @pdev:platform_device
 *
 * probe function.
 *
 */
static int rgb_led_probe(struct platform_device *pdev)
{
    /*
        if((g_var.is_fly) || (g_var.recovery_mode))
        {
            lidbg("rgb_led_probe do nothing\n");
            return 0;
        }
    */
    //lidbg("%s:enter\n", __func__);
    lidbg("PCA9634 init start!\n");
    if(PCA9634_Init())
    {
        lidbg_new_cdev(&pca9634_fops, "lidbg_rgb_led0");//add cdev
        lidbg("lidbg_rgb_led init succeed!\n");
    }
    else
        lidbg("lidbg_rgb_led init failed!\n");

    return 0;

}


/**
 * rgb_led_remove - module remove function
 * @pdev:platform_device
 *
 * module remove function.
 *
 */
static int rgb_led_remove(struct platform_device *pdev)
{
    //gpio_free(TEF6638_GPIO_RST);
    return 0;
}


static struct platform_device rgb_led_devices =
{
    .name			= "rgb_led",
    .id 			= 0,
};

static struct platform_driver rgb_led_driver =
{
    .probe = rgb_led_probe,
    .remove = rgb_led_remove,
    .driver = {
        .name = "rgb_led",
        .owner = THIS_MODULE,
    },
};

/**
 * rgb_led_init - module init function
 *
 * module init function.
 *
 */
static int rgb_led_init(void)
{
    //lidbg("%s:enter\n", __func__);
    LIDBG_GET;
    platform_device_register(&rgb_led_devices);
    platform_driver_register(&rgb_led_driver);
    return 0;

}

/**
 * rgb_led_exit - module exit function
 *
 * module exit function.
 *
 */
static void __exit rgb_led_exit(void)
{
    platform_driver_unregister(&rgb_led_driver);
    platform_device_unregister(&rgb_led_devices);
}


module_init(rgb_led_init);
module_exit(rgb_led_exit);
MODULE_LICENSE("GPL");

