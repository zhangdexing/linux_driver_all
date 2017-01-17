
#ifndef _FLY_INTERFACE__
#define _FLY_INTERFACE__

#ifdef BUILD_DRIVERS
#include <flymeg.h>

#include <lidbg_pm.h>
#include <lidbg_drivers_loader.h>
#include <lidbg_target.h>
#include <misc_devices.h>
#include <lidbg_bpmsg.h>
#include <lidbg_ts_probe.h>
#include <lidbg_flyparameter.h>
#include <lidbg_monkey.h>
#include <lidbg_lpc.h>
#else
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
//#include <asm/system.h>
#include <linux/fb.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/semaphore.h>
#include <linux/kfifo.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/vmalloc.h>
#include <linux/cpufreq.h>

#endif

enum sound_enum
{
    SND_START = 1,
    SND_STOP,
    SND_NAVI_START,
    SND_NAVI_STOP,
    SND_PA_MUTE,
    SND_PA_UNMUTE,
};

enum key_enum
{
    TS_NO_KEY = 0,
    TS_KEY_POWER,
    TS_KEY_BACK,
    TS_KEY_HOME,
    TS_KEY_VOLUMEDOWN,
    TS_KEY_VOLUMEUP,
    TS_KEY_NAVI,
    TS_KEY_SEEKUP,
    TS_KEY_SEEKDOWN,
    TS_KEY_MUTE,

};
struct ts_devices_key
{
    bool is_depend_key;
    enum key_enum key_value;
    bool key_pressed;
    s32 key_x;
    s32 key_y;
    s32 offset_x;
    s32 offset_y;
};
struct ts_devices
{
    char ts_description[64];
    s32 lcd_origin_x;
    s32 lcd_origin_y;
    s32 key_nums;
    struct ts_devices_key key[15];
};
//extern enum key_enum ts_active_key;
static inline int write_node(char *filename, char *wbuff)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int file_len = 1;

    filep = filp_open(filename,  O_RDWR, 0);
    if(IS_ERR(filep))
        return -1;
    old_fs = get_fs();
    set_fs(get_ds());

    if(wbuff)
        filep->f_op->write(filep, wbuff, strlen(wbuff), &filep->f_pos);
    set_fs(old_fs);
    filp_close(filep, 0);
    return file_len;
}
#define NOTIFIER_MAJOR_ACC_EVENT (111)
#define NOTIFIER_MINOR_SUSPEND_PREPARE  (2)
#define NOTIFIER_MINOR_SUSPEND_UNPREPARE (3)
#define NOTIFIER_MINOR_POWER_OFF  (4)
#define NOTIFIER_MINOR_SCREEN_ON  (5)
#define NOTIFIER_MINOR_SCREEN_OFF (6)
#define NOTIFIER_MINOR_BL_LCD_ON  (7)
#define NOTIFIER_MINOR_BL_LCD_OFF (8)


typedef enum
{
    LTL_TRANSFER_RTC = 1,
    LTL_TRANSFER_NULL
} linux_to_lidbg_transfer_t;
typedef enum
{
    PM_AUTOSLEEP_STORE1 = 1,
    PM_AUTOSLEEP_SET_STATE2,
    PM_QUEUE_UP_SUSPEND_WORK3,
    PM_TRY_TO_SUSPEND4,
    PM_TRY_TO_SUSPEND4P1,
    PM_TRY_TO_SUSPEND4P2,
    PM_TRY_TO_SUSPEND4P3,
    PM_TRY_TO_SUSPEND4P4,
    PM_TRY_TO_SUSPEND4P5,
    PM_SUSPEND5,
    PM_ENTER_STATE6,
    PM_ENTER_STATE6P1,
    PM_ENTER_STATE6P2,
    PM_SUSPEND_DEVICES_AND_ENTER7,
    PM_SUSPEND_ENTER8,
    PM_SUSPEMD_OPS_ENTER9,
    PM_SUSPEMD_OPS_ENTER9P1,
    PM_NULL
} fly_pm_stat_step;

#define NOTIFIER_MAJOR_BL_LCD_STATUS_CHANGE	(120)
#define NOTIFIER_MINOR_BL_HAL_ON	(0)
#define NOTIFIER_MINOR_BL_HAL_OFF	(1)
#define NOTIFIER_MINOR_BL_APP_ON	(2)
#define NOTIFIER_MINOR_BL_APP_OFF	(3)



#if (defined(BUILD_SOC) || defined(BUILD_CORE) || defined(BUILD_DRIVERS))
#define NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE (110)

#define NOTIFIER_MINOR_ACC_ON (20)
#define NOTIFIER_MINOR_ACC_OFF (21)


#define PM_WARN(fmt, args...) do { printk(KERN_CRIT"[lidbg]   [pm] .warn.%s: " fmt,__func__,##args);}while(0)
#define PM_ERR(fmt, args...) do { printk(KERN_CRIT"[lidbg]   [pm] .lidbgerr.%s: " fmt,__func__,##args);}while(0)
#define PM_SUC(fmt, args...) do { printk(KERN_CRIT"[lidbg]   [pm] .succ.%s: " fmt,__func__,##args);}while(0)
#define PM_SLEEP_DBG(fmt, args...) do{printk(KERN_CRIT"[lidbg]   [pm]sleep_step: ++++++++++++++" fmt,##args);}while(0)
#define PM_WAKE_DBG(fmt, args...) do{printk(KERN_CRIT"[lidbg]   [pm]wake_step: ===" fmt,##args);}while(0)
#else
#define PM_WARN(fmt, args...) do { printk(KERN_CRIT"[lidbg]   [pm] .warn.%s: " fmt,__func__,##args);}while(0)
#define PM_ERR(fmt, args...) do { printk(KERN_CRIT"[lidbg]   [pm] .lidbgerr.%s: " fmt,__func__,##args);}while(0)
#define PM_SUC(fmt, args...) do { printk(KERN_CRIT"[lidbg]   [pm] .succ.%s: " fmt,__func__,##args);}while(0)
#define PM_SLEEP_DBG(fmt, args...) do{printk(KERN_CRIT"[lidbg]   [pm]sleep_step: ++++++++++++++" fmt,##args);}while(0)
#define PM_WAKE_DBG(fmt, args...) do{printk(KERN_CRIT"[lidbg]   [pm]wake_step: ===" fmt,##args);}while(0)

#define NOTIFIER_VALUE(major,minor)  (((major)&0xffff)<<16 | ((minor)&0xffff))

#define BEGIN_KMEM do{old_fs = get_fs();set_fs(get_ds());}while(0)
#define END_KMEM   do{set_fs(old_fs);}while(0)

#define KEY_RELEASED    (0)
#define KEY_PRESSED      (1)
#define KEY_PRESSED_RELEASED   ( 2)
struct tspara
{
    int x;
    int y;
    int press;
} ;
typedef irqreturn_t (*pinterrupt_isr)(int irq, void *dev_id);

#define ADC_MAX_CH (8)

#endif

#ifdef SUSPEND_ONLINE
typedef enum
{
    FLY_SCREEN_OFF,
	FLY_GOTO_SLEEP,
    FLY_DEVICE_DOWN,
    FLY_FASTBOOT_REQUEST,
    FLY_ANDROID_DOWN,
	FLY_SLEEP_TIMEOUT,
    FLY_KERNEL_DOWN,
    FLY_KERNEL_UP,

    FLY_ANDROID_UP,
    FLY_DEVICE_UP,
    FLY_SCREEN_ON,
} FLY_SYSTEM_STATUS;
#else
typedef enum
{
    FLY_SCREEN_OFF,
    FLY_DEVICE_DOWN,
    FLY_FASTBOOT_REQUEST,
    FLY_ANDROID_DOWN,
	FLY_GOTO_SLEEP,
    FLY_KERNEL_DOWN,
    FLY_KERNEL_UP,

    FLY_ANDROID_UP,
    FLY_DEVICE_UP,
    FLY_SCREEN_ON,
    
    FLY_SLEEP_TIMEOUT,
} FLY_SYSTEM_STATUS;
#endif

typedef enum
{
	FLY_ACC_OFF,
	FLY_ACC_ON,
}FLY_ACC_STATUS;


typedef enum
{
	BACKCAR_TYPE_CVBS,
	BACKCAR_TYPE_USB,
}FLY_BACKCAR_TYPE;

typedef enum
{
	ID0_MSM7627A,
	ID1_MSM8625,
	ID2_MSM8226_442,
	ID3_MSM8926_444,
	ID4_MSM8974_444,
	ID5_MT3360_42,
	ID6_MSM8226_50,
	ID7_MSM8974_51,
	ID8_RK3188_RADXA_442,
	ID9_RK3188_PX3_444,
	ID10_MSM8226_511,
	ID11_MSM8909_511,
	ID12_MSM8226_444,
	ID13_A80_44,
	ID14_MSM8974_600,
}PLATFORMID;

struct lidbg_fn_t
{

    void (*pfnSOC_IO_Output) (unsigned int group, unsigned int index, bool status);
    bool (*pfnSOC_IO_Input) (unsigned int group, unsigned int index, unsigned int pull);
    void (*pfnSOC_IO_Output_Ext)(unsigned int group, unsigned int index, bool status, unsigned int pull, unsigned int drive_strength);
    bool (*pfnSOC_IO_Config)(unsigned int index, int func, u32 direction, unsigned int pull, unsigned int drive_strength);

    bool (*pfnSOC_IO_ISR_Add)(unsigned int irq, unsigned int interrupt_type, pinterrupt_isr func, void *dev);
    bool (*pfnSOC_IO_ISR_Enable)(unsigned int irq);
    bool (*pfnSOC_IO_ISR_Disable)(unsigned int irq);
    bool (*pfnSOC_IO_ISR_Del )(unsigned int irq);

    int (*pfnSOC_I2C_Send) (int bus_id, char chip_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Rec)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Rec_Simple)(int bus_id, char chip_addr, char *buf, unsigned int size);

    int (*pfnSOC_I2C_Rec_SAF7741)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Send_TEF7000)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Rec_TEF7000)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Rec_2B_SubAddr)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);

    bool (*pfnSOC_ADC_Get)(unsigned int channel , unsigned int *value);
    void (*pfnSOC_Key_Report)(unsigned int key_value, unsigned int type);
    int  (*pfnSOC_BL_Set)( unsigned int level);
    int  (*pfnSOC_Display_Get_Res)(unsigned int *screen_x, unsigned int *screen_y);
    int  (*pfnSOC_LPC_Send)(unsigned char *p, unsigned int len);
    void (*pfnSOC_System_Status)(FLY_SYSTEM_STATUS status);
    void (*pfnSOC_WakeLock_Stat)(bool lock, const char *name);

    void (*pfnSOC_PM_STEP)(fly_pm_stat_step step, void *data);
    int (*pfnLINUX_TO_LIDBG_TRANSFER)(linux_to_lidbg_transfer_t _enum, void *data);

    //screan_off :0 screan_on :1 suspendon:2 suspendoff:3
    void (*pfnHal_Acc_Callback)(int para);

    int (*pfnSOC_SPI_Mode_Set) (int bus_id, u8 mode, u8 bits_per_word, u32 max_speed_hz);
    int (*pfnSOC_SPI_Send) (int bus_id, char *buf, unsigned int size);
    int (*pfnSOC_SPI_Rec)(int bus_id, char *buf, unsigned int size);
    int (*pfnSOC_SPI_Send_Rec)(int bus_id, const u8 *txbuf, unsigned n_tx, u8 *rxbuf, unsigned n_rx);

    int (*pfnSOC_Get_System_Sound_Status)(void *para, int length);
    int (*pfnGPS_sound_status)(void);

    void (*pfnSOC_Set_Touch_Pos)(struct tspara *touch);
    int (*pfnSOC_Get_CpuFreq)(void);

    bool (*pfnSOC_IO_Suspend_Config)(unsigned int index, u32 direction, unsigned int pull, unsigned int drive_strength);
    int (*pfnSOC_Uart_Send)(char *arg);
    void (*pfnHal_Ts_Callback)(int para);


    int (*pfnSOC_Temp_Get)(void);
    int (*pfnSOC_I2C_Rec_TEF6638)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    bool (*pfnLPC_ADC_Get)(unsigned int channel , unsigned int *value);
};


struct hw_info
{
    int hw_version;
    int ts_type;
    int ts_config;
    int lcd_type;
    int virtual_key;
    int reserve[12];
};

struct lidbg_pvar_t // var must add  at the end of the struct
{
    int temp;
    FLY_SYSTEM_STATUS system_status;
    int machine_id;
    int cpu_freq;
    bool is_fly;
    unsigned int flag_for_15s_off;
    bool is_usb11;
    bool fake_suspend;
    bool acc_flag;
    struct list_head *ws_lh;
    bool recovery_mode;
    struct hw_info hw_info;
    bool fb_on;
    bool is_first_update;
    enum key_enum ts_active_key ;
    bool led_app_status;
    bool led_hal_status;
    bool is_debug_mode;
    bool is_udisk_needreset;
    bool usb_status;
    bool usb_request;
    bool suspend_timeout_protect;
    bool android_boot_completed;
    int alarmtimer_interval;
    bool usb_cam_request;
    int sleep_counter;
    int acc_counter;
    int acc_goto_sleep_time;	
    int flyaudio_reboot;
    int is_phone_in_call_state;
    int udisk_stable_test;
    char* car_type;
    int suspend_airplane_mode;
    bool enable_gsensor_data_for_android;
    int platformid;
    int keep_lcd_on;
    struct list_head *pflyhal_config_list;
    bool dvr_cam_ready;
    bool rear_cam_ready;
    FLY_BACKCAR_TYPE  backcar_type;	

};

typedef struct
{
    int flag_hw_info_valid;
    int flag_need_update;
    struct hw_info hw_info;
} fly_hw_data;

struct lidbg_interface
{
    union
    {
        struct lidbg_fn_t soc_func_tbl;
        unsigned char reserve[256];
    };
    union
    {
        struct lidbg_pvar_t soc_pvar_tbl;
        unsigned char reserve1[256];
    };
};

#define LIDBG_DEV_CHECK_READY  (plidbg_dev != NULL)
#define LIDBG_DEFINE  struct lidbg_interface *plidbg_dev = NULL
#define g_var  plidbg_dev->soc_pvar_tbl


#define LIDBG_GET  \
 	do{\
	 mm_segment_t old_fs;\
	 struct file *fd = NULL;\
	 printk("[lidbg]  [call LIDBG_GET] by %s\n",__FUNCTION__);\
	 while(1){\
	 	printk("lidbg: %s:%s try open lidbg_interface!\n",__FILE__,__FUNCTION__);\
	 	fd = filp_open("/dev/lidbg_interface", O_RDWR, 0);\
	 	printk("lidbg:get fd=%p\n",(void *)fd);\
	    if((fd == NULL)||((ssize_t)fd == (~(ssize_t)0 - 1))){printk("lidbg:get fd fail!\n");msleep(500);}\
	    else break;\
	 }\
	 BEGIN_KMEM;\
	 fd->f_op->read(fd, (void*)&plidbg_dev, sizeof(void *) ,&fd->f_pos);\
	 END_KMEM;\
	filp_close(fd,0);\
	if(plidbg_dev == NULL)\
	{\
		printk("LIDBG_GET fail!\n");\
	}\
}while(0)


#define LIDBG_THREAD_DEFINE   \
    struct lidbg_interface *plidbg_dev = NULL;\
	static struct task_struct *getlidbg_task;\
	static int thread_getlidbg(void *data);\
	int thread_getlidbg(void *data)\
	{\
		LIDBG_GET;\
		return 0;\
	}

#define LIDBG_GET_THREAD  do{\
	getlidbg_task = kthread_create(thread_getlidbg, NULL, "getlidbg_task");\
	if(IS_ERR(getlidbg_task))\
	{\
		printk("Unable to start kernel thread.\n");\
	}else wake_up_process(getlidbg_task);\
}while(0)


extern struct lidbg_interface *plidbg_dev;

static inline int check_pt(void)
{
    while (plidbg_dev == NULL)
    {
        printk("lidbg:check if plidbg_dev==NULL\n");
        printk("%s,line %d\n", __FILE__, __LINE__);
        dump_stack();
        msleep(200);
    }
    return 0;
}


#define SOC_IO_Output (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Output))
#define SOC_IO_Input  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Input))
#define SOC_IO_Output_Ext (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Output_Ext))
#define SOC_IO_Config  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Config))
#define SOC_IO_Suspend_Config  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Suspend_Config))

#define SOC_I2C_Send  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send))
#define SOC_I2C_Rec   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec))
#define SOC_I2C_Rec_Simple   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_Simple))

#define SOC_I2C_Rec_SAF7741  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_SAF7741))
#define SOC_I2C_Send_TEF7000   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send_TEF7000))
#define SOC_I2C_Rec_TEF7000   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_TEF7000))
#define SOC_I2C_Rec_2B_SubAddr   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_2B_SubAddr))

//SPI
#define SOC_SPI_Mode_Set (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_SPI_Mode_Set))
#define SOC_SPI_Send  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_SPI_Send))
#define SOC_SPI_Rec   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_SPI_Rec))
#define SOC_SPI_Rec_Send   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_SPI_Send_Rec))

#define SOC_IO_ISR_Add  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Add))
#define SOC_IO_ISR_Enable   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Enable))
#define SOC_IO_ISR_Disable  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Disable))
#define SOC_IO_ISR_Del  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Del))

#define SOC_ADC_Get  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_ADC_Get))

#define SOC_Key_Report  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Key_Report))

#define SOC_BL_Set  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_BL_Set))

#define SOC_Display_Get_Res  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Display_Get_Res))

#define SOC_LPC_Send  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_LPC_Send))
#define SOC_System_Status (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_System_Status))
#define SOC_WakeLock_Stat (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_WakeLock_Stat))

#define FLAG_FOR_15S_OFF   (plidbg_dev->soc_pvar_tbl.flag_for_15s_off)

#define DVR_CAM_READY   (plidbg_dev->soc_pvar_tbl.dvr_cam_ready)
#define REAR_CAM_READY   (plidbg_dev->soc_pvar_tbl.rear_cam_ready)

#define LINUX_TO_LIDBG_TRANSFER (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnLINUX_TO_LIDBG_TRANSFER))
#define SOC_PM_STEP (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_PM_STEP))
#define SOC_Hal_Acc_Callback (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnHal_Acc_Callback))

#define GPS_sound_status (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnGPS_sound_status))

#define SOC_Set_Touch_Pos (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Set_Touch_Pos))
#define SOC_Get_System_Sound_Status (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Get_System_Sound_Status))
#define SOC_Get_CpuFreq (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Get_CpuFreq))
#define SOC_Uart_Send  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Uart_Send))
#define SOC_Hal_Ts_Callback (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnHal_Ts_Callback))
#define SOC_Temp_Get (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Temp_Get))

#define SOC_I2C_Rec_TEF6638  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_TEF6638))
#define LPC_ADC_Get  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnLPC_ADC_Get))
#endif
