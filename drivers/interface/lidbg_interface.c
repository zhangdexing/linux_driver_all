
#include "lidbg.h"
#include "lidbg_target.c"
#define HAL_SO "/flysystem/lib/out/lidbg_loader.ko"
LIDBG_DEFINE;

static LIST_HEAD(flyhal_config_list);
#ifdef SOC_msm8x25
#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
#define FLYHAL_CONFIG_PATH "/flydata/flyhalconfig"
#else
#define FLYHAL_CONFIG_PATH "/flysystem/flyconfig/default/lidbgconfig/flylidbgconfig.txt"
#endif
#else
#define FLYHAL_CONFIG_PATH "/flysystem/flyconfig/default/lidbgconfig/flylidbgconfig.txt"
#endif

static int bl_event_handle(struct notifier_block *this,
                           unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch(event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_ON):
        g_var.led_hal_status = 1;
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_OFF):
        g_var.led_hal_status = 0;
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_ON):
        g_var.led_app_status = 1;
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_APP_OFF):
        g_var.led_app_status = 0;
        break;
    default:
        return NOTIFY_DONE;
    }
    lidbg("hal=%d,app=%d\n", g_var.led_hal_status, g_var.led_app_status);
    if((g_var.led_hal_status & g_var.led_app_status)&&(g_var.flyaudio_reboot==0))
    	{
        		lidbg("LCD_ON1\n");
        		LCD_ON;
	}
    else
        LCD_OFF;

    return NOTIFY_DONE;
}

static struct notifier_block bl_ctl_bn =
{
    .notifier_call = bl_event_handle,
};


//ad_key_map[ts_active_key].ad_value
void interface_func_tbl_default(void)
{
    lidbgerr("interface_func_tbl_default:this func not ready!\n");
    //print who call this
    dump_stack();
}

bool iSOC_IO_ISR_Add(u32 irq, u32  interrupt_type, pinterrupt_isr func, void *dev)
{
    bool ret = 0;
    struct io_int_config io_int_config1;

    io_int_config1.ext_int_num = GPIO_TO_INT(irq);
    io_int_config1.irqflags = interrupt_type;
    io_int_config1.pisr = func;
    io_int_config1.dev = dev;

    lidbg("ext_int_num:%d,%d\n", irq,GPIO_TO_INT(irq));

    ret =  soc_io_irq(&io_int_config1);
#ifdef SOC_mt35x
    return ret;
#endif
#if 1
#ifdef SUSPEND_ONLINE
	//if(g_var.suspend_airplane_mode == 0)
	{
		if(irq == MCU_IIC_REQ_GPIO)
		{
	      		 lidbg("Enable IO(%d), irq[%d] as wakeup source\n", irq, io_int_config1.ext_int_num);
	       	enable_irq_wake(GPIO_TO_INT(MCU_IIC_REQ_GPIO));
		}
	}
#endif
#endif

    return ret;
}

bool iSOC_IO_ISR_Enable(u32 irq)
{
    lidbg("SOC_IO_ISR_Enable:%d,%d\n",irq,GPIO_TO_INT(irq));
    soc_irq_enable(GPIO_TO_INT(irq));
    return 1;
}

bool iSOC_IO_ISR_Disable(u32 irq)
{
    lidbg("SOC_IO_ISR_Disable:%d,%d\n",irq,GPIO_TO_INT(irq));
    soc_irq_disable(GPIO_TO_INT(irq));
    return 1;
}
bool iSOC_IO_ISR_Del (u32 irq)
{
    free_irq(GPIO_TO_INT(irq), NULL);
    return 1;
}

bool iSOC_IO_Config(u32 index, int func, u32 direction,  u32 pull, u32 drive_strength)
{
    return soc_io_config( index, func, direction, pull, drive_strength, 1);
}

bool iSOC_IO_Suspend_Config(u32 index, u32 direction, u32 pull, u32 drive_strength)
{
    return soc_io_suspend_config( index,  direction, pull, drive_strength);
}

void iSOC_IO_Output_Ext(u32 group, u32 index, bool status, u32 pull, u32 drive_strength)
{
    if(status == 1)
        soc_io_config( index, GPIOMUX_FUNC_GPIO, GPIOMUX_OUT_HIGH, pull, drive_strength, 1);
    else if (status == 0)
        soc_io_config( index,  GPIOMUX_FUNC_GPIO, GPIOMUX_OUT_LOW, pull, drive_strength, 1);

    soc_io_output(group, index,  status);
}

void iSOC_IO_Output(u32 group, u32 index, bool status)
{
#ifdef PLATFORM_ID_7
    soc_io_config( index, GPIOMUX_FUNC_GPIO, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA, 0);
#else
    if(status == 1)
        soc_io_config( index,  GPIOMUX_FUNC_GPIO, GPIOMUX_OUT_HIGH, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA, 0);
    else if (status == 0)
        soc_io_config( index,  GPIOMUX_FUNC_GPIO, GPIOMUX_OUT_LOW, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA, 0);
#endif
    soc_io_output(group, index,  status);
}

bool iSOC_IO_Input(u32 group, u32 index, u32 pull)
{
    soc_io_config( index,  GPIOMUX_FUNC_GPIO, GPIOMUX_IN, pull/*GPIO_CFG_NO_PULL*/, GPIOMUX_DRV_2MA, 0);
    return soc_io_input(index);
}

//return mv
bool iSOC_ADC_Get (u32 channel , u32 *value)
{
    if((g_var.system_status <= FLY_KERNEL_DOWN) && (g_var.system_status >= FLY_DEVICE_DOWN))
    {
        return 0;
    }

    *value = 0xffffffff;
#ifdef SOC_mt3360
    *value = soc_ad_read(channel);
#else
    *value = soc_ad_read(channel) / 1000;
#endif
#if 0
    lidbg("iSOC_ADC_Get=%d\n", *value);
#endif
    if(*value == 0xffffffff)
        return 0;
    return 1;
}
void iSOC_Key_Report(u32 key_value, u32 type)
{
    lidbg_key_report(key_value, type);
}

// 7bit i2c sub_addr
int iSOC_I2C_Send(int bus_id, char chip_addr, char *buf, unsigned int size)
{
    int ret;
    if(bus_id == g_hw.i2c_bus_lpc) WAKEUP_MCU_BEGIN;
    ret = i2c_api_do_send( bus_id,  chip_addr,  0,  buf,  size);
    if(bus_id == g_hw.i2c_bus_lpc) WAKEUP_MCU_END;
    return  ret;
}
int iSOC_I2C_Rec(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv( bus_id,  chip_addr,  sub_addr, buf,  size);
}

int iSOC_I2C_Rec_2B_SubAddr(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_sub_addr_2bytes( bus_id,  chip_addr,  sub_addr, buf,  size);
}

int iSOC_I2C_Rec_3B_SubAddr(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_sub_addr_3bytes( bus_id,  chip_addr,  sub_addr, buf,  size);
}

int iSOC_I2C_Rec_Simple(int bus_id, char chip_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_no_sub_addr( bus_id,  chip_addr,  0, buf,  size);
}

int i2c_api_do_recv_SAF7741(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_send_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_recv_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);

int iSOC_I2C_Rec_SAF7741(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_SAF7741( bus_id,  chip_addr, sub_addr, buf,  size);
}

int iSOC_I2C_Send_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_send_TEF7000( bus_id,  chip_addr, sub_addr, buf,  size);
}

int iSOC_I2C_Rec_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_TEF7000( bus_id,  chip_addr, sub_addr, buf,  size);
}
/////////spi
/*
int iSOC_SPI_Mode_Set(int bus_id, u8 mode,u8 bits_per_word,u32 max_speed_hz)
{
   return spi_api_do_set( bus_id,  mode,bits_per_word, max_speed_hz);
}
int iSOC_SPI_Rec(int bus_id,char *buf, unsigned int size)
{
    return   spi_api_do_read( bus_id, buf,  size);
}

int iSOC_SPI_Send(int bus_id,char *buf, unsigned int size)
{
    return   spi_api_do_write( bus_id, buf,  size);
}

int iSOC_SPI_Do_Write_Then_Read(int bus_id, const u8 *txbuf, unsigned n_tx, u8 *rxbuf, unsigned n_rx)
{
   return spi_api_do_write_then_read(bus_id, txbuf,  n_tx, rxbuf, n_rx);
}
*/
void iSOC_PWM_Set(int pwm_id, int duty_ns, int period_ns)
{
    soc_pwm_set(pwm_id, duty_ns, period_ns);
}

//0~255
int iSOC_BL_Set( u32 bl_level)
{
    lidbg("SOC_BL_Set:%d\n", bl_level);
    if(bl_level == 1)
    {
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_ON));
    }

    else if(bl_level == 0)
    {
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE, NOTIFIER_MINOR_BL_HAL_OFF));

    }
    else
        soc_bl_set(bl_level);
    return 1;

}

int iSOC_Display_Get_Res(u32 *screen_x, u32 *screen_y)
{
    return soc_get_screen_res(screen_x, screen_y);
}

struct fly_smem *iSOC_Get_Share_Mem(void)
{
    return p_fly_smem;
}

void iSOC_System_Status(FLY_SYSTEM_STATUS status)
{
    lidbg("SOC_System_Status=%d\n", status);
    g_var.system_status = status;
    lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, g_var.system_status));
}

int iSOC_Get_CpuFreq(void)
{
    int cpu_freq, i;
    /*
    	char buf[16];
        lidbg_readwrite_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq", buf, NULL, 16);
        cpu_freq = simple_strtoul(buf, 0, 0);
        g_var.cpu_freq = cpu_freq;
    */
    for(i = 0; i < 4; i++)
    {
        cpu_freq = cpufreq_get(i);
        if(cpu_freq > 0)
            break;
    }

    if(0)
    for(i = 0; i < 4; i++)
    {
        cpu_freq = cpufreq_get(i);
        lidbg("cpu%dfreq_get=%d\n", i,cpu_freq);

    }
    return cpu_freq;
}


int iSOC_Temp_Get(void)
{
    return soc_temp_get(g_hw.mem_sensor_num);
}



int iSOC_Uart_Send(char *arg)
{
//TBD
/*
    char cmd[128] = {0};
#ifdef UART_SEND_DATA_TO_LPC
    sprintf(cmd, "/system/lib/modules/out/lidbg_uart_send_data "LPC_UART_PORT" "UART_BAUD_REATE" %d %d %d %d %d", arg[0], arg[1], arg[3], arg[4], arg[5]);
#endif

    lidbg_shell_cmd(cmd);
 */
    return 0;
    
}

static void set_func_tbl(void)
{
    //io
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Output = iSOC_IO_Output;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Input = iSOC_IO_Input;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Output_Ext = iSOC_IO_Output_Ext;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Config = iSOC_IO_Config;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Suspend_Config = iSOC_IO_Suspend_Config;

    //i2c
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send = iSOC_I2C_Send;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec = iSOC_I2C_Rec;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_Simple = iSOC_I2C_Rec_Simple;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_2B_SubAddr = iSOC_I2C_Rec_2B_SubAddr;

    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_SAF7741 = iSOC_I2C_Rec_SAF7741;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send_TEF7000 = iSOC_I2C_Send_TEF7000;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_TEF7000 = iSOC_I2C_Rec_TEF7000;

    //spi
    /*
        plidbg_dev->soc_func_tbl.pfnSOC_SPI_Mode_Set=iSOC_SPI_Mode_Set;
        plidbg_dev->soc_func_tbl.pfnSOC_SPI_Send =iSOC_SPI_Send;
        plidbg_dev->soc_func_tbl.pfnSOC_SPI_Rec =iSOC_SPI_Rec;
        plidbg_dev->soc_func_tbl.pfnSOC_SPI_Send_Rec = iSOC_SPI_Do_Write_Then_Read;
    */
    //io-irq
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Add = iSOC_IO_ISR_Add;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Enable = iSOC_IO_ISR_Enable;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Disable = iSOC_IO_ISR_Disable;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Del = iSOC_IO_ISR_Del;

    //ad
    plidbg_dev->soc_func_tbl.pfnSOC_ADC_Get = iSOC_ADC_Get;

    //key
    plidbg_dev->soc_func_tbl.pfnSOC_Key_Report = iSOC_Key_Report;

    //bl
    plidbg_dev->soc_func_tbl.pfnSOC_BL_Set = iSOC_BL_Set;

    //display/touch
    plidbg_dev->soc_func_tbl.pfnSOC_Display_Get_Res = iSOC_Display_Get_Res;

    //   plidbg_dev->soc_func_tbl.pfnSOC_Get_Share_Mem = iSOC_Get_Share_Mem;
    plidbg_dev->soc_func_tbl.pfnSOC_System_Status = iSOC_System_Status;
    plidbg_dev->soc_func_tbl.pfnSOC_WakeLock_Stat  = lidbg_wakelock_register;
    plidbg_dev->soc_func_tbl.pfnSOC_Get_CpuFreq  = iSOC_Get_CpuFreq;

    plidbg_dev->soc_func_tbl.pfnSOC_Uart_Send = iSOC_Uart_Send;
    plidbg_dev->soc_func_tbl.pfnHal_Ts_Callback = NULL;

    plidbg_dev->soc_func_tbl.pfnSOC_Temp_Get = iSOC_Temp_Get;

}

int interface_open(struct inode *inode, struct file *filp)
{
    return 0;
}
int interface_release(struct inode *inode, struct file *filp)
{
    return 0;
}
ssize_t interface_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int ret = 0;
    size_t read_value = 0;
	unsigned int count = sizeof(size_t);
    read_value = (size_t)plidbg_dev;

    lidbg("interface_read:read_value=%zx,read_count=%d\n", (size_t)read_value, count);
    if (copy_to_user(buf, &read_value, count))
    {
        ret =  - EFAULT;
    }
    else
    {
        ret = count;
    }

    return count;
}
static ssize_t interface_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        PM_ERR("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';
    PM_WARN("interface_write:[%s]\n", cmd_buf);


    //flyaudio logic
    if(!strcmp(cmd_buf, "BOOT_COMPLETED"))
    {
        g_var.android_boot_completed = 1;
        PM_WARN("g_var.android_boot_completed = 1\n");
    }
    return size;
}

#define DEVICE_NAME "lidbg_interface"
static struct file_operations dev_fops =
{
    .owner	=	THIS_MODULE,
    .open   =   interface_open,
    .read   =   interface_read,
    .write   =   interface_write,
    .release =  interface_release,
};

int fly_interface_init(void)
{
    ssize_t p ;
    DUMP_BUILD_TIME;
    lidbg_new_cdev(&dev_fops, DEVICE_NAME);
    plidbg_dev = kmalloc(sizeof(struct lidbg_interface), GFP_KERNEL);

    if (plidbg_dev == NULL)
    {

        LIDBG_ERR("kmalloc.\n");
        return 0;
    }

    p = (ssize_t) & (plidbg_dev->soc_func_tbl);
    {
        int i;
        for(i = 0; i < sizeof(plidbg_dev->soc_func_tbl) / sizeof(void *); i++)
        {
            // (((int *) & (plidbg_dev->soc_func_tbl))[i]) = interface_func_tbl_default;
            *((ssize_t *)(p + i * sizeof(void *))) = (ssize_t)interface_func_tbl_default;
        }
    }
    memset(&(plidbg_dev->soc_pvar_tbl), (ssize_t)NULL, sizeof(struct lidbg_pvar_t));

    set_func_tbl();


    {
        int ret = fs_fill_list(FLYHAL_CONFIG_PATH, FS_CMD_FILE_CONFIGMODE, &flyhal_config_list);
        LIDBG_WARN("FLYHAL_CONFIG_PATH:%s ret=%d", FLYHAL_CONFIG_PATH, ret);
        g_var.pflyhal_config_list = &flyhal_config_list;
        fs_show_list(g_var.pflyhal_config_list);
    }


    g_var.temp = 0;
    g_var.system_status = FLY_SCREEN_ON;
    g_var.machine_id = get_machine_id();


    g_var.is_fly = 0;
    g_var.fake_suspend = 0;
#ifdef SUSPEND_ONLINE	
    g_var.acc_flag =  FLY_ACC_ON;//SOC_IO_Input(MCU_ACC_STATE_IO, MCU_ACC_STATE_IO, GPIO_CFG_PULL_UP);
#endif
    g_var.ws_lh = NULL;
    g_var.fb_on = true;
    g_var.ts_active_key = TS_NO_KEY;

    g_var.flyaudio_reboot=0;
    g_var.led_app_status = 1;
    g_var.led_hal_status = 1;
    g_var.is_udisk_needreset = 1;
    g_var.usb_status = 0;
    g_var.usb_request = 0;
    g_var.usb_cam_request = 0;
    g_var.acc_goto_sleep_time = 0;
    g_var.backcar_type = BACKCAR_TYPE_CVBS;
#ifdef SOC_mt35x	
    g_var.rear_cam_ready = 1;
#endif

#ifdef SUSPEND_ONLINE	
    g_var.suspend_timeout_protect = 0;
#else
    g_var.suspend_timeout_protect = 1;
#endif

    g_var.android_boot_completed = 0;
    g_var.udisk_stable_test = 0;
#ifdef DEFAULT_SUSPEND_NO_AIRPLANE_MODE
    g_var.suspend_airplane_mode = 0;
#else
    g_var.suspend_airplane_mode = 1;
#endif
    g_var.enable_gsensor_data_for_android = 0;
    g_var.keep_lcd_on = 0;
	
    if(gboot_mode == MD_RECOVERY)
    {
        g_var.recovery_mode = 1;
        g_var.android_boot_completed = 1;
	 g_var.acc_flag =  FLY_ACC_ON;
        lidbg("system mode is recovery_mode,and set g_var.android_boot_completed = 1\n");
    }
    else
    {
        g_var.recovery_mode = 0;
        lidbg("system mode is normal_mode\n");
    }

    if(fs_is_file_exist(HAL_SO))
    {
        lidbg("is product\n");
        g_var.is_fly = true;
    }
    else
    {
        g_var.is_fly = false;
        lidbg("is origin\n");
    }

    memset(&g_var.hw_info, 0, sizeof(struct hw_info));

    FS_REGISTER_INT(g_var.hw_info.hw_version, "hw_version", 0, NULL);
    FS_REGISTER_INT(g_var.hw_info.ts_type, "ts_type", 0, NULL);
    FS_REGISTER_INT(g_var.hw_info.ts_config, "ts_config", 0, NULL);
    FS_REGISTER_INT(g_var.hw_info.lcd_type, "lcd_type", 0, NULL);
    FS_REGISTER_INT(g_var.hw_info.virtual_key, "virtual_key", 0, NULL);


    if(g_var.hw_info.hw_version == 0)
    {
#ifdef BOARD_V1
        g_var.hw_info.hw_version = 1;
#endif
#ifdef BOARD_V2
        g_var.hw_info.hw_version = 2;
#endif
#ifdef BOARD_V3
        g_var.hw_info.hw_version = 3;
#endif
#ifdef BOARD_V4
        g_var.hw_info.hw_version = 4;
#endif
#ifdef BOARD_V5
        g_var.hw_info.hw_version = 5;
#endif
#ifdef BOARD_V6
        g_var.hw_info.hw_version = 6;
#endif
#ifdef BOARD_V7
        g_var.hw_info.hw_version = 7;
#endif
#ifdef BOARD_V8
        g_var.hw_info.hw_version = 8;
#endif
#ifdef BOARD_V9
        g_var.hw_info.hw_version = 9;
#endif

    }
    lidbg("hw_version=%d\n", g_var.hw_info.hw_version);

    lidbg("android_version=%d\n", ANDROID_VERSION);


    //if((g_var.recovery_mode == 0) && ( fs_is_file_exist(LIDBG_PATH)))
    {
        g_var.is_first_update = is_out_updated;
        lidbg("g_var.is_first_update=%d\n", g_var.is_first_update);
    }

    g_var.is_debug_mode = fs_is_file_exist("/data/lidbg/set_debug_mode");

    register_lidbg_notifier(&bl_ctl_bn);
    LIDBG_MODULE_LOG;
    return 0;
}

void fly_interface_deinit(void) {}

module_init(fly_interface_init);
module_exit(fly_interface_deinit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");


