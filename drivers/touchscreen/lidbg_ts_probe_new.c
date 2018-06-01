
#include "lidbg.h"

LIDBG_DEFINE;

static int flyts_hal_init(void);
static struct semaphore sem;
static wait_queue_head_t wait_queue;

u8 flyts_hal_data;

#define FIFO_SIZE (256)
u8 *flyts_hal_fifo_buffer;
static struct kfifo flyts_hal_data_fifo;


#define USE_TS_NUM (0)

static LIST_HEAD(lidbg_ts_config_list);

struct ts_devices g_ts_devices;


#define GTP_SWAP SWAP
#define GTP_REVERT(x, y)     do{\
         x = max_x-x;\
         y = max_y-y;\
       }while (0)
#define GTP_REVERT_Y(y)     do{\
         y = max_y-y;\
       }while (0)
#define GTP_REVERT_X(x)     do{\
         x = max_x-x;\
       }while (0)

#define GTP_TOP_SCOPE_Y(y, scope)  do{\
	if(y < scope)\
		y = screen_touch_scope_val;\
       }while (0)

#define GTP_TOP_SCOPE_X(x, scope)  do{\
	if(x < scope)\
		x = screen_touch_scope_val;\
       }while (0)

static int ts_scan_delayms;

int ts_should_revert = -1;
static int screen_touch_scope_y = 0;
static int screen_touch_scope_x = 0;
static int screen_touch_scope_val = 0;
bool is_ts_load = false;
u32 max_x, max_y;

struct probe_device
{
    char chip_addr;
    unsigned int sub_addr;
    char *name;
    void (*reset)(void);
    void (*find_cb)(void);
};

void reset_high_active(void);
void reset_low_active(void);
void gt9xx_reset_high_active(void);
void ts_gpio_free(void);
void ts_gpio_free_gt911(void);

struct probe_device ts_probe_dev[] =
{
    {0x14, 0x00, "gt911.ko", gt9xx_reset_high_active, ts_gpio_free_gt911},
    {0x5d, 0x00, "gt811.ko", reset_high_active, ts_gpio_free},
    {0x55, 0x00, "gt801.ko", reset_low_active, ts_gpio_free},
    {0x38, 0x00, "ft5x06_ts.ko", reset_high_active, ts_gpio_free}
    //{0x5d, 0x00, LOG_CAP_TS_GT910, "gt910new.ko",reset_high_active},
};


//zone below [method]
void gt9xx_reset_high_active(void)
{
    SOC_IO_Output(0, GTP_RST_PORT, !GTP_RST_PORT_ACTIVE);
    msleep(200);
#ifndef SOC_msm8x25
    SOC_IO_Output(0, GTP_INT_PORT, 1);
    udelay(200);
#endif
    SOC_IO_Output(0, GTP_RST_PORT, GTP_RST_PORT_ACTIVE);
    msleep(300);
}
void reset_high_active(void)
{
    SOC_IO_Output(0, GTP_RST_PORT, !GTP_RST_PORT_ACTIVE);
    msleep(200);
    SOC_IO_Output(0, GTP_RST_PORT, GTP_RST_PORT_ACTIVE);
    msleep(300);
}

void reset_low_active(void)
{
    SOC_IO_Output(0, GTP_RST_PORT, GTP_RST_PORT_ACTIVE);
    msleep(200);
    SOC_IO_Output(0, GTP_RST_PORT, !GTP_RST_PORT_ACTIVE);
    msleep(300);
}
void ts_gpio_free(void)
{
#ifndef SOC_msm8x25
    //gpio_free(GTP_INT_PORT);
    //gpio_free(GTP_RST_PORT);
#endif
}


void ts_gpio_free_gt911(void)
{
#ifndef SOC_msm8x25
    //gpio_free(GTP_INT_PORT);
    //gpio_free(GTP_RST_PORT);
#endif
}

void parse_ts_info(struct probe_device *ts_info)
{
    char path[100];

    if(gboot_mode == MD_FLYSYSTEM)
    {
        sprintf(path, "/flysystem/lib/out/%s", ts_info->name);
        lidbg_insmod( path );
    }
    else  if(gboot_mode == MD_DEBUG)
    {
        sprintf(path, "/data/out/%s", ts_info->name);
        lidbg_insmod( path );
    }
    else
    {
        sprintf(path, "/system/lib/modules/out/%s", ts_info->name);
        lidbg_insmod( path );
    }		
    lidbg_fs_log(TS_LOG_PATH, "loadts=%s,USE_TS_NUM:%d,g_var.hw_info.ts_type:%d,ts_should_revert:%d\n", ts_info->name, USE_TS_NUM, g_var.hw_info.ts_type, ts_should_revert);
    fs_mem_log("loadts=%s,USE_TS_NUM:%d,g_var.hw_info.ts_type:%d,ts_should_revert:%d\n", ts_info->name, USE_TS_NUM, g_var.hw_info.ts_type, ts_should_revert);

}

struct probe_device *ts_scan(struct probe_device *tsdev, int size)
{
    static unsigned int loop = 0;
    int32_t rc1, rc2;
    int i;
    u8 tmp;

    for(i = 0; i < size; i++)
    {
        tsdev->reset();

        rc1 = SOC_I2C_Rec_Simple(TS_I2C_BUS, tsdev->chip_addr, &tmp, 1 );
        rc2 = SOC_I2C_Rec(TS_I2C_BUS, tsdev->chip_addr, tsdev->sub_addr, &tmp, 1 );
        SOC_I2C_Rec(TS_I2C_BUS, 0x12, 0x00, &tmp, 1 ); //let i2c bus release

        if ((rc1 < 0) && (rc2 < 0))
        {
            loop++;
            lidbg("fail.%d:[0x%x] rc1=%x,rc2=%x,delayms=%d\n", loop, tsdev->chip_addr, rc1, rc2, ts_scan_delayms);
        }
        else
        {
            lidbg("found:[0x%x,%s]\n", tsdev->chip_addr, tsdev->name);
            tsdev->find_cb();
            return tsdev;
        }
        tsdev++;
    }
#ifdef SOC_msm8x25
    if(loop == 30)
    {
        if(g_var.is_fly)
            lidbg_insmod("/flysystem/lib/out/gt801.ko");
        else
            lidbg_insmod("/system/lib/modules/out/gt801.ko");
    }
#endif

    return NULL;
}

#define TS_TAG "ts_config:"
void ts_devices_show(char *whocalls)
{
    int loop;
    if(!whocalls)
        return;
    LIDBG_WARN(TS_TAG"====%s====\n", whocalls);
    LIDBG_WARN(TS_TAG"x,y,nums,desc:%d,%d,%d,%s\n", g_ts_devices.lcd_origin_x, g_ts_devices.lcd_origin_y, g_ts_devices.key_nums, g_ts_devices.ts_description);
    for(loop = 0; loop < g_ts_devices.key_nums; loop++)
    {
        LIDBG_WARN(TS_TAG"key%d:%d,%d,%d,%d,%d,%d,%d\n", loop,
                   g_ts_devices.key[loop].is_depend_key,
                   g_ts_devices.key[loop].key_value,
                   g_ts_devices.key[loop].key_pressed,
                   g_ts_devices.key[loop].key_x,
                   g_ts_devices.key[loop].key_y,
                   g_ts_devices.key[loop].offset_x,
                   g_ts_devices.key[loop].offset_y);
    }
}

void ts_devices_init(void)
{
    char ts_config_file[64] = {0}, tmp[32] = {0};
    snprintf(tmp, sizeof(tmp), "ts_config/ts_config_%d.conf", g_var.hw_info.virtual_key );

    get_lidbg_file_path(ts_config_file, tmp);

    fs_get_intvalue(g_var.pflyhal_config_list, "screen_touch_scope_Y", &screen_touch_scope_y, NULL);
    fs_get_intvalue(g_var.pflyhal_config_list, "screen_touch_scope_X", &screen_touch_scope_x, NULL);
    fs_get_intvalue(g_var.pflyhal_config_list, "screen_touch_scope_val", &screen_touch_scope_val, NULL);

    if(g_var.hw_info.virtual_key)
    {
        char *ts_devices_key_map = NULL, *ts_description = NULL;
        LIDBG_WARN(TS_TAG"<use:%s>\n", ts_config_file);
        fs_fill_list(ts_config_file, FS_CMD_FILE_CONFIGMODE, &lidbg_ts_config_list);

        if((fs_get_intvalue(&lidbg_ts_config_list, "lcd_origin_x", &g_ts_devices.lcd_origin_x, NULL) < 0) || (fs_get_intvalue(&lidbg_ts_config_list, "lcd_origin_y", &g_ts_devices.lcd_origin_y, NULL) < 0)
                || fs_get_intvalue(&lidbg_ts_config_list, "key_nums", &g_ts_devices.key_nums, NULL) < 0)
            LIDBG_WARN(TS_TAG"<err:lcd_origin_x>\n");
        else
            LIDBG_WARN(TS_TAG"suc:%d,%d,%d\n", g_ts_devices.lcd_origin_x, g_ts_devices.lcd_origin_y, g_ts_devices.key_nums);

        if(fs_get_value(&lidbg_ts_config_list, "ts_devices_key_map", &ts_devices_key_map) < 0 || fs_get_value(&lidbg_ts_config_list, "ts_description", &ts_description) < 0)
            LIDBG_WARN(TS_TAG"<err,ts_devices_key_map>\n");
        else
        {
            char *key_item[32] = {NULL};
            int key_count = 0, loop = 0;
            if(ts_description)
                strncpy(g_ts_devices.ts_description, ts_description, sizeof(g_ts_devices.ts_description));

            LIDBG_WARN(TS_TAG"<ts_description:%s>\n", g_ts_devices.ts_description);
            LIDBG_WARN(TS_TAG"<map:%s>\n", ts_devices_key_map);

            key_count = lidbg_token_string(ts_devices_key_map, "@", key_item);
            LIDBG_WARN(TS_TAG"<key_count:%d,%d>\n", key_count, g_ts_devices.key_nums);

            for(loop = 0; loop < key_count; loop++)//toke keymap
            {
                int item_token_len = 0;
                char *item_intvalue[32] = {NULL};
                if(!key_item[loop])
                    continue;
                LIDBG_WARN(TS_TAG"<%d.toke:%s>\n", loop, key_item[loop]);
                item_token_len = lidbg_token_string(key_item[loop], ",", item_intvalue);
                if(item_token_len >= 7)
                {
                    g_ts_devices.key[loop].is_depend_key = simple_strtoul(item_intvalue[0], 0, 0);
                    g_ts_devices.key[loop].key_value = simple_strtoul(item_intvalue[1], 0, 0);
                    g_ts_devices.key[loop].key_pressed = simple_strtoul(item_intvalue[2], 0, 0);
                    g_ts_devices.key[loop].key_x = simple_strtoul(item_intvalue[3], 0, 0);
                    g_ts_devices.key[loop].key_y = simple_strtoul(item_intvalue[4], 0, 0);
                    g_ts_devices.key[loop].offset_x = simple_strtoul(item_intvalue[5], 0, 0);
                    g_ts_devices.key[loop].offset_y = simple_strtoul(item_intvalue[6], 0, 0);
                }
                else
                    LIDBG_WARN(TS_TAG"err:item_token_len<7.%dn", item_token_len);
            }
        }
        ts_devices_show("ts_devices_init");
    }
    else
        LIDBG_WARN("<file miss:%s>\n", ts_config_file);
}
int get_input_key(enum key_enum key_value)
{
    switch(key_value)
    {
    case TS_KEY_HOME :
        return KEY_HOME;
    case TS_KEY_POWER:
        return KEY_HOME;
    case TS_KEY_BACK:
        return KEY_BACK;
    case    TS_KEY_VOLUMEDOWN:
        return KEY_VOLUMEDOWN;
    case   TS_KEY_VOLUMEUP:
        return KEY_VOLUMEUP;
    case   TS_KEY_NAVI:
        return KEY_HOME;
    default:
        return KEY_BACK;

    }
}

void ts_key_report(s32 input_x, s32 input_y, struct ts_devices_key *tskey, int size)
{
    int i;
    u8 fifo_out, bytes;
    for(i = 0; i < size; i++)
    {
        if( (abs( input_x - tskey->key_x) <= tskey->offset_x) && (abs( input_y - tskey->key_y) <= tskey->offset_y))
        {
            if(!g_var.is_fly)
                SOC_Key_Report(get_input_key(tskey->key_value), KEY_PRESSED_RELEASED);
            else
            {
                g_var.ts_active_key = tskey->key_value;
                if(SOC_Hal_Ts_Callback)
                {
                    pr_debug("SOC_Hal_Ts_Callbacking:%d\n", g_var.ts_active_key);
                    SOC_Hal_Ts_Callback( g_var.ts_active_key);
                }
                else
                {
                    flyts_hal_data = g_var.ts_active_key;

                    down(&sem);
                    if(kfifo_is_full(&flyts_hal_data_fifo))
                    {
                        bytes = kfifo_out(&flyts_hal_data_fifo, &fifo_out, 1);
                        lidbg("[ts_hal]kfifo_full!!!!!\n");
                    }
                    kfifo_in(&flyts_hal_data_fifo, &flyts_hal_data, 1);
                    up(&sem);

                    wake_up_interruptible(&wait_queue);
                    lidbg("flyts_hal_data = %x\n", flyts_hal_data);
                }
            }
            lidbg("tskey->key_value% d", tskey->key_value);
            return;
        }
        tskey++;
    }
#if 0
    if(SOC_Hal_Ts_Callback)
    {
        //lidbg("SOC_Hal_Ts_Callbacking:%d",TS_NO_KEY);
        SOC_Hal_Ts_Callback(TS_NO_KEY);
    }
#endif
}

void ts_probe_prepare(void)
{
    char buff[50] = {0};

    FS_REGISTER_INT(ts_scan_delayms, "ts_scan_delayms", 500, NULL);

    ts_should_revert = fs_find_string(g_var.pflyhal_config_list, "TSMODE_XYREVERT");
   if(g_recovery_meg->hwInfo.info[7]=='1')
    {
        LIDBG_WARN("<hwInfo.info[7]=1,ts_should_revert=1,[%d]>\n",ts_should_revert);
        ts_should_revert=1;
    }
    if(g_recovery_meg->hwInfo.info[7]=='2')
     {
         LIDBG_WARN("<hwInfo.info[7]=2,ts_should_revert=2,[%d]>\n",ts_should_revert);
         ts_should_revert=2;
     }
    if(ts_should_revert > 0)
        LIDBG_WARN("<TS.XY will revert>\n");
    else
        LIDBG_WARN("<TS.XY will normal>\n");

    if(fs_is_file_exist(LIDBG_LOG_DIR"no_revert.txt"))
    {
        LIDBG_WARN("<TS.XY will no_revert./data/lidbg/no_revert.txt>\n");
        ts_should_revert = 0;
    }

    lidbg_insmod(get_lidbg_file_path(buff, "lidbg_ts_to_recov.ko"));
    ts_devices_init();
}
//zone end
void ts_data_report(touch_type t, int id, int x, int y, int w)
{
    u8 fifo_out, bytes;
    GTP_SWAP(x, y);
    if (1 == ts_should_revert)
        GTP_REVERT(x, y);
    if (2 == ts_should_revert)
        GTP_REVERT_Y(y);
    if (3 == ts_should_revert)
        GTP_REVERT_X(x);
    if (screen_touch_scope_y)
	GTP_TOP_SCOPE_Y(y, screen_touch_scope_y);
    if (screen_touch_scope_x)
	GTP_TOP_SCOPE_X(x, screen_touch_scope_x);
    pr_debug("%s:%d,%d[%d,%d,%d]\n", __FUNCTION__, t, id, x, y, w);
    if(g_var.hw_info.virtual_key > 0)
    {
        if( ((g_ts_devices.lcd_origin_x < x) && (x < 1024 + g_ts_devices.lcd_origin_x) && (g_ts_devices.lcd_origin_y < y) && (y < g_ts_devices.lcd_origin_y + 600)) || (t == TOUCH_SYNC) || (t == TOUCH_UP))
        {
            lidbg_touch_handle(t, id, x - g_ts_devices.lcd_origin_x, y - g_ts_devices.lcd_origin_y, w);
            if( (t == TOUCH_UP) && (id == 0))
            {
                if(SOC_Hal_Ts_Callback)
                {
                    pr_debug("SOC_Hal_Ts_Callbacking:%d\n", TS_NO_KEY);
                    SOC_Hal_Ts_Callback(TS_NO_KEY);
                }
                else
                {
                    flyts_hal_data = TS_NO_KEY;
                    down(&sem);
                    if(kfifo_is_full(&flyts_hal_data_fifo))
                    {
                        bytes = kfifo_out(&flyts_hal_data_fifo, &fifo_out, 1);
                        lidbg("[ts_hal]kfifo_full!!!!!\n");
                    }
                    kfifo_in(&flyts_hal_data_fifo, &flyts_hal_data, 1);
                    up(&sem);

                    wake_up_interruptible(&wait_queue);
                    lidbg("flyts_hal_data = %x\n", flyts_hal_data);
                }
            }
        }
        else
            ts_key_report(x, y, g_ts_devices.key, SIZE_OF_ARRAY(g_ts_devices.key));
    }
    else
        lidbg_touch_handle(t, id, x, y, w);

    if(t == TOUCH_DOWN)
    {
        g_var.flag_for_15s_off++;
        if(g_var.flag_for_15s_off >= 1000)
        {
            g_var.flag_for_15s_off = 1000;
        }
    }

    if((id == 1) && (t == TOUCH_DOWN) && (g_var.recovery_mode == 1)) // 2 fingers in recovery send power key
        SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
	
    if((id == 4) && (t == TOUCH_DOWN) && (g_var.is_fly == 1)) // 2 fingers in flysystem
                    lidbg("\n\n5 fingers trigger \n\n");

    if((id == 4) && (t == TOUCH_DOWN) && (!g_var.is_fly) ) // 5 fingers in origin system send back key
    {
        if(g_var.system_status >= FLY_KERNEL_UP)
            SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
    }

#if 0
    if((id == 4) && (t == TOUCH_DOWN) ) // 5 fingers in origin system send back key
    {
        if(g_var.system_status >= FLY_KERNEL_UP)
            SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
    }
#endif

    if((id == 0) && (1 == g_var.recovery_mode))
    {
        static struct tspara touch = {0, 0, 0} ;
        if(t == TOUCH_DOWN)
            touch.press = 0;
        else
            touch.press = 1;
        touch.x = x;
        touch.y = y;
        //SOC_Set_Touch_Pos(&touch);  //not ready
    }
}

int ts_probe_open(struct inode *inode, struct file *filp)
{
    return 0;
}
int ts_probe_close(struct inode *inode, struct file *filp)
{
    return 0;
}


static void parse_cmd(char *pt)
{
    int argc = 0;
    char *argv[32] = {NULL};

    lidbg("parse_cmd:%s\n", pt);
    argc = lidbg_token_string(pt, " ", argv);

    if (!strcmp(argv[0], "revert"))
    {
        ts_should_revert = 1;
        lidbg_shell_cmd("rm -rf "LIDBG_LOG_DIR"no_revert.txt");
    }
    else if (!strcmp(argv[0], "no_revert"))
    {
        ts_should_revert = 0;
        lidbg_shell_cmd("echo 123 > "LIDBG_LOG_DIR"no_revert.txt");
    }
}


static ssize_t ts_probe_write(struct file *filp, const char __user *buf,
                              size_t size, loff_t *ppos)
{
    char *p = NULL;
    int len = size;
    char tmp[size + 1];//C99 variable length array
    char *mem = tmp;

    memset(mem, '\0', size + 1);

    if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }

    if((p = memchr(mem, '\n', size)))
    {
        len = p - mem;
        *p = '\0';
    }
    else
        mem[len] =  '\0';

    parse_cmd(mem);

    return size;//warn:don't forget it;
}


static struct file_operations dev_fops =
{
    .owner = THIS_MODULE,
    .open = ts_probe_open,
    .write = ts_probe_write,
    .release = ts_probe_close,
};


//zone below [logic]
int ts_probe_thread(void *data)
{
    struct probe_device *ts = NULL;
#if 0//def SUSPEND_ONLINE
    g_var.acc_flag = SOC_IO_Input(MCU_ACC_STATE_IO, MCU_ACC_STATE_IO, GPIO_CFG_PULL_UP);
    LIDBG_WARN("<g_var.acc_flag.1:%d>\n",g_var.acc_flag);
    while(g_var.recovery_mode == 0&&g_var.acc_flag==FLY_ACC_OFF)
    {
        msleep(200);
    }
    LIDBG_WARN("<g_var.acc_flag.2:%d>\n",g_var.acc_flag);
#endif
    SOC_Display_Get_Res(&max_x, &max_y);

    ts_probe_prepare();

    if (USE_TS_NUM == 0 && g_var.hw_info.ts_type == 0 )
    {
        int cnt = 10;
        LIDBG_WARN("<mode:scan enable[%d,%d,%d]>\n", USE_TS_NUM, g_var.hw_info.ts_type, g_var.hw_info.ts_config);
        while(cnt--)
        {
            if((ts = ts_scan(ts_probe_dev, SIZE_OF_ARRAY(ts_probe_dev))))
            {
                parse_ts_info(ts);
                break;
                ts = NULL;
            }
            msleep(ts_scan_delayms);
        }
    }
    else
    {
        LIDBG_WARN("<mode:scan disable[%d,%d,%d]>\n", USE_TS_NUM, g_var.hw_info.ts_type, g_var.hw_info.ts_config);
        parse_ts_info(&ts_probe_dev[g_var.hw_info.ts_type > 0 ? g_var.hw_info.ts_type - 1 : USE_TS_NUM - 1 ]);
    }

    lidbg_new_cdev(&dev_fops, "ts_probe0");

    LIDBG_WARN("<ts_probe_thread exited>\n");
    return 0;
}
//zone end


static int ts_probe_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;
    CREATE_KTHREAD(ts_probe_thread, NULL);
    flyts_hal_init();
    return 0;
}

static void ts_probe_exit(void)
{
}

//###############################-<ts for hal>-###############################

/**
 * flyts_hal_poll - poll function
 * @filp:module file struct
 * @wait:poll table
 *
 * poll function.
 *
 */
static unsigned int flyts_hal_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    //lidbg("[flyts_hal_poll]wait begin\n");
    poll_wait(filp, &wait_queue, wait);
    //lidbg("[flyts_hal_poll]wait done\n");
    down(&sem);
    if(!kfifo_is_empty(&flyts_hal_data_fifo))
    {
        mask |= POLLIN | POLLRDNORM;
    }
    up(&sem);
    return mask;
}

/**
 * ts_hal_probe - probe function
 * @filp:file struct
 * @buf:user buffer
 * @count:bytes count
 * @f_pos:file pos
 *
 * read function.
 *
 */
ssize_t flyts_hal_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int read_len, fifo_len, bytes;
    lidbg("ts_hal read start.\n");
    if(kfifo_is_empty(&flyts_hal_data_fifo))
    {
        if(wait_event_interruptible(wait_queue, !kfifo_is_empty(&flyts_hal_data_fifo)))
            return -ERESTARTSYS;
    }
    down(&sem);

    fifo_len = kfifo_len(&flyts_hal_data_fifo);

    if(count > fifo_len)
        read_len = fifo_len;
    else
        read_len = count;

    bytes = kfifo_out(&flyts_hal_data_fifo, flyts_hal_fifo_buffer, read_len);
    up(&sem);

    if(copy_to_user(buf, flyts_hal_fifo_buffer, read_len))
    {
        return -1;
    }

    if(kfifo_len(&flyts_hal_data_fifo) > 0)
        wake_up_interruptible(&wait_queue);

    return read_len;
}

/**
 * flyts_hal_open - open function
 * @inode:inode
 * @filp:file struct
 *
 * open function.
 *
 */
int flyts_hal_open (struct inode *inode, struct file *filp)
{
    lidbg("[ts_hal]flyts_hal_open\n");
    return 0;
}

static  struct file_operations flyts_hal_fops =
{
    .owner = THIS_MODULE,
    .read = flyts_hal_read,
    .poll = flyts_hal_poll,
    .open = flyts_hal_open,
};

/**
 * ts_hal_probe - probe function
 * @pdev:platform_device
 *
 * probe function.
 *
 */
static int  flyts_hal_init(void)
{
    flyts_hal_fifo_buffer = (u8 *)kmalloc(FIFO_SIZE + 1, GFP_KERNEL);
    if(flyts_hal_fifo_buffer == NULL)
    {
        lidbg("flyts_hal_init kmalloc err\n");
        return 0;
    }
    lidbg_new_cdev(&flyts_hal_fops, "lidbg_ts_probe0");//add cdev
    init_waitqueue_head(&wait_queue);
    sema_init(&sem, 1);
    kfifo_init(&flyts_hal_data_fifo, flyts_hal_fifo_buffer, FIFO_SIZE);
    lidbg_shell_cmd("chmod 777 /dev/lidbg_ts_probe0");
    return 0;
}


module_init(ts_probe_init);
module_exit(ts_probe_exit);

EXPORT_SYMBOL(is_ts_load);
EXPORT_SYMBOL(ts_should_revert);
EXPORT_SYMBOL(ts_data_report);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("lsw.");
