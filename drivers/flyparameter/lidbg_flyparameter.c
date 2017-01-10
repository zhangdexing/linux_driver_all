
#include "lidbg.h"
#define PATH_FLY_HW_INFO_CONFIG USB_MOUNT_POINT"/conf/machine_info.conf"
#define RECOVERY_PATH_FLY_HW_INFO_CONFIG RECOVERY_USB_MOUNT_POINT"/machine_info.conf"

#define FLAG_HW_INFO_VALID (0x12345678)

LIDBG_DEFINE;

static fly_hw_data *g_fly_hw_data = NULL;
recovery_meg_t *g_recovery_meg = NULL;
recovery_meg_t default_recovery_meg ;
char *p_kmem = NULL;
int update_hw_info = 0;
enum update_info_enum
{
    NO_FLIE = 0,
    UPDATE_SUC = 1,
    UPDATE_FAIL = 2,
    NOT_NEED_UPDATE = 3,
};
enum update_info_enum update_info  = NOT_NEED_UPDATE;
void fly_hw_info_show(char *when, fly_hw_data *p_info)
{
    lidbg("flyparameter:%s:g_fly_hw_data:flag=%x,%x,hw=%d,ts=%d,%d,lcd=%d\n", when,
          p_info->flag_hw_info_valid,
          p_info->flag_need_update,
          p_info->hw_info.hw_version,
          p_info->hw_info.ts_type,
          p_info->hw_info.ts_config,
          p_info->hw_info.lcd_type);
}


void g_hw_info_store(void)
{
    lidbg_fs_mem("g_hw_info:hw=%d,ts=%d,%d,lcd=%d\n",
                 g_var.hw_info.hw_version,
                 g_var.hw_info.ts_type,
                 g_var.hw_info.ts_config,
                 g_var.hw_info.lcd_type);
}


void read_fly_hw_config_file(fly_hw_data *p_info)
{
    LIST_HEAD(hw_config_list);
    if(g_var.recovery_mode)
        fs_fill_list(RECOVERY_PATH_FLY_HW_INFO_CONFIG, FS_CMD_FILE_CONFIGMODE, &hw_config_list);
    else
        fs_fill_list(PATH_FLY_HW_INFO_CONFIG, FS_CMD_FILE_CONFIGMODE, &hw_config_list);

    fs_get_intvalue(&hw_config_list, "hw_version", &(p_info->hw_info.hw_version), NULL);
    fs_get_intvalue(&hw_config_list, "ts_type", &(p_info->hw_info.ts_type), NULL);
    fs_get_intvalue(&hw_config_list, "ts_config", &(p_info->hw_info.ts_config), NULL);
    fs_get_intvalue(&hw_config_list, "lcd_type", &(p_info->hw_info.lcd_type), NULL);
    fly_hw_info_show("fs_fill_list", p_info);
}

bool fly_hw_info_get(fly_hw_data *p_info)
{
    if(FLYPARAMETER_NODE == NULL)
    {
        lidbg("g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }
    if(p_info && fs_file_read(FLYPARAMETER_NODE, (char *)p_info, MEM_SIZE_512_KB , sizeof(fly_hw_data)) >= 0)
    {
        fly_hw_info_show("fly_hw_info_get", p_info);
        return true;
    }
    return false;
}

bool fly_hw_info_save(fly_hw_data *p_info)
{
    DUMP_FUN;
    if(FLYPARAMETER_NODE == NULL)
    {
        lidbg("g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }
    read_fly_hw_config_file(p_info);
    if( p_info && fs_file_write(FLYPARAMETER_NODE, false, (void *) p_info, MEM_SIZE_512_KB , sizeof(fly_hw_data)) >= 0)
    {
        lidbg("fly_hw_data:save success\n");
        update_info = UPDATE_SUC;
        return true;
    }
    lidbg("fly_hw_data:save err\n");
    update_info = UPDATE_FAIL;
    return false;
}

void cb_fly_hw_info_save(char *key, char *value )
{
    if((update_hw_info != 0) && (fs_is_file_exist(PATH_FLY_HW_INFO_CONFIG)))
    {
        g_fly_hw_data->flag_need_update = 0;
        g_fly_hw_data->flag_hw_info_valid = FLAG_HW_INFO_VALID;
        fly_hw_info_save(g_fly_hw_data);
    }
}

bool flyparameter_info_get(void)
{
    bool is_ublox_so_exist = false;

    if(FLYPARAMETER_NODE == NULL)
    {
        lidbg("g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }

    if(p_kmem && fs_file_read(FLYPARAMETER_NODE, p_kmem, 0, sizeof(recovery_meg_t)) >= 0)
    {
        g_recovery_meg = (recovery_meg_t *)p_kmem;
        lidbg("flyparameter1:%s,%s,%s\n", g_recovery_meg->recoveryLanguage.flags, g_recovery_meg->bootParam.bootParamsLen.flags, g_recovery_meg->bootParam.upName.flags);
        lidbg("flyparameter2:%d,%s,%x\n", g_recovery_meg->bootParam.upName.val, g_recovery_meg->bootParam.autoUp.flags, g_recovery_meg->bootParam.autoUp.val);
        lidbg("g_recovery_meg %x\n", g_recovery_meg->hwInfo.bValid);

        if(g_recovery_meg->hwInfo.bValid == 0x12345678)
        {
            int ret;
            char parameter[256];
            memset(parameter, '\0', sizeof(parameter));

            ret = sprintf(parameter, "setprop HWINFO.FLY.Parameter %s", g_recovery_meg->hwInfo.info);
            if(ret < 0)
            {
                lidbg("fail to cpy parameter\n");
            }
            lidbg_shell_cmd( parameter );
            fs_clear_file("/data/commenPersist/hwinfo.txt");
            fs_file_write2("/data/commenPersist/hwinfo.txt", g_recovery_meg->hwInfo.info);
            lidbg("flyparameter=[%s]\n", g_recovery_meg->hwInfo.info);
            fs_mem_log("flyparameter=[%s]\n", g_recovery_meg->hwInfo.info);

            lidbg("flyparameter3 :%d,%d,%d,%d,%d\n", g_recovery_meg->hwInfo.info[0] - '0', g_recovery_meg->hwInfo.info[1] - '0', g_recovery_meg->hwInfo.info[2] - '0', g_recovery_meg->hwInfo.info[3] - '0', g_recovery_meg->hwInfo.info[4] - '0');
            g_var.hw_info.ts_config = 10 * (g_recovery_meg->hwInfo.info[0] - '0') + g_recovery_meg->hwInfo.info[1] - '0';
            g_var.hw_info.virtual_key = 10 * (g_recovery_meg->hwInfo.info[2] - '0') + g_recovery_meg->hwInfo.info[3] - '0';
            g_var.hw_info.lcd_type = g_recovery_meg->hwInfo.info[4] - '0';
            lidbg("ts_config:%d,virtual_key:%d\n", g_var.hw_info.ts_config, g_var.hw_info.virtual_key);

            is_ublox_so_exist = fs_is_file_exist("/flysystem/lib/out/"FLY_GPS_SO);
            lidbg("ts_config5:gps:%c,%d\n", g_recovery_meg->hwInfo.info[5] , is_ublox_so_exist);
            if((g_recovery_meg->hwInfo.info[5] == '1') && (is_ublox_so_exist))// 0 - ublox ,1 -qualcomm gps
            {
                lidbg("rm ublox so\n");
                lidbg_shell_cmd("mount -o remount /flysystem");
                lidbg_shell_cmd("rm /flysystem/lib/out/"FLY_GPS_SO);
                lidbg_shell_cmd("mount -o remount,ro /flysystem");
            }
	     {
	            char set_car_type[256];
	            memset(set_car_type, '\0', sizeof(set_car_type));
		     g_var.car_type = g_recovery_meg->bootParam.upName.flags;
	            lidbg("car_type=%s\n",g_var.car_type);

	            ret = sprintf(set_car_type, "setprop HWINFO.FLY.car_type %s", g_var.car_type);
	            if(ret < 0)
	            {
	                lidbg("fail to cpy car_type\n");
	            }
	            lidbg_shell_cmd( set_car_type );
	            lidbg_shell_cmd( "mount -o remount /" );
	            lidbg_shell_cmd( "chmod 777 /flyconfig" );
	            lidbg_shell_cmd( "/flysystem/bin/decodeFlyconfig &" );
	            lidbg("bring up:/flysystem/bin/decodeFlyconfig &\n");
	            fs_mem_log("car_type=[%s]\n", g_var.car_type);
	     }
		 
#if 0
            if( !strncmp(g_var.car_type, "822", 3))//Israel
            	{
            		 lidbg("Israel car_type,suspend_airplane_mode\n");
		 	g_var.suspend_airplane_mode = true;
            	}
#endif
            return true;
        }
    }
    else
    {
        lidbgerr("\n===flyparameter.disable===\n");
        g_recovery_meg = &default_recovery_meg;
    }
    return false;
}
//simple_strtoul(argv[0], 0, 0);
bool flyparameter_info_save(recovery_meg_t *p_info)
{
    if(FLYPARAMETER_NODE == NULL)
    {
        lidbg("g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }
    if( p_info && fs_file_write(FLYPARAMETER_NODE, false, (void *) p_info, 0, sizeof(recovery_meg_t)) >= 0)
    {
        lidbg("flyparameter:save success\n");
        return true;
    }
    lidbg("flyparameter:save err\n");
    return false;
}

int thread_lidbg_fly_hw_info_update(void *data)
{
    while(!fs_is_file_exist(RECOVERY_PATH_FLY_HW_INFO_CONFIG))
    {
        update_info  = NO_FLIE;
        msleep(50);
    }
    fly_hw_info_save(g_fly_hw_data);
    return 0;
}

int thread_fix_fly_update_info(void *data)
{
    char info;
    msleep(20000);
    fs_file_read("/dev/fly_upate_info0", &info, 0, sizeof(info));
    lidbg("read info is %c\n", info);
    return 1;
}

static bool fly_get_cmdline(void)
{
    char cmdline[512];
    fs_file_read("/proc/cmdline", cmdline, 0, sizeof(cmdline));
    cmdline[512 - 1] = '\0';
    printk( KERN_CRIT "kernel cmdline = %s", cmdline);
    return ((strstr(cmdline, "update_hw_info") == NULL) ? false : true);
}

int lidbg_fly_hw_info_init(void)
{
    g_fly_hw_data = kzalloc(sizeof(fly_hw_data), GFP_KERNEL);
    if(!g_fly_hw_data)
        lidbgerr("kzalloc.g_fly_hw_data\n");

    /*
        if(fs_is_file_exist(PATH_MACHINE_INFO_FILE))
        {
            lidbgerr("return.%s,miss\n", PATH_MACHINE_INFO_FILE);
            return -1;
        }
    */
    if(!fly_hw_info_get(g_fly_hw_data))
        lidbgerr("fly_hw_info_get\n");

    if(g_var.recovery_mode)
    {
        if(fly_get_cmdline())
        {
            g_fly_hw_data->flag_need_update = FLAG_HW_INFO_VALID;
            fly_hw_info_save(g_fly_hw_data);
            update_info = NOT_NEED_UPDATE;
        }
        else if(g_fly_hw_data->flag_need_update == FLAG_HW_INFO_VALID)
        {
            g_fly_hw_data->flag_need_update = 0;
            fly_hw_info_save(g_fly_hw_data);//clear need_update  flag first

            g_fly_hw_data->flag_hw_info_valid = FLAG_HW_INFO_VALID;
            CREATE_KTHREAD(thread_lidbg_fly_hw_info_update, NULL);
        }
        else
            update_info = NOT_NEED_UPDATE;
    }

    if((g_fly_hw_data->flag_hw_info_valid == FLAG_HW_INFO_VALID))
        // g_var.hw_info = g_fly_hw_data->hw_info;
    {
        int i;
        for(i = 0; i < sizeof(struct hw_info) / 4; i++)
        {
            //lidbg("i=%d,val1=%d,val2=%d\n",i,((int*)(&g_fly_hw_data->hw_info))[i],((int*)(&g_var.hw_info))[i]);
            if(((int *)(&g_fly_hw_data->hw_info))[i] != 0)
            {
                ((int *)(&g_var.hw_info))[i] = ((int *)(&g_fly_hw_data->hw_info))[i];
            }
        }
    }

    g_hw_info_store();
    return 0;
}

int fly_upate_info_open(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t  fly_upate_info_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    char tmp[2];
    update_info  = NOT_NEED_UPDATE;
    sprintf(tmp, "%d", update_info);
    if (copy_to_user(buffer, tmp, sizeof(tmp)))
    {
        lidbg("copy_to_user ERR\n");
    }
    lidbg("update_info = %d,read size =%zd\n\n", update_info, size);
    return size;

}
static  struct file_operations fly_upate_info_fops =
{
    .owner = THIS_MODULE,
    .open = fly_upate_info_open,
    .read = fly_upate_info_read,
};
int flyparameter_init(void)
{
    p_kmem = kzalloc(sizeof(recovery_meg_t), GFP_KERNEL);
    if(!p_kmem)
        lidbgerr("kzalloc.p_kmem\n");

    if(!flyparameter_info_get())
        lidbg("flyparameter_info_get\n");

    return 0;
}

int lidbg_flyparameter_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;
    lidbg_chmod(FLYPARAMETER_NODE);
    FS_REGISTER_INT(update_hw_info, "update_hw_info", 0, cb_fly_hw_info_save);
    flyparameter_init();
    lidbg_fly_hw_info_init();//block other ko before hw_info set
    lidbg_new_cdev(&fly_upate_info_fops, "fly_upate_info0");
    CREATE_KTHREAD(thread_fix_fly_update_info, NULL);
    LIDBG_MODULE_LOG;
    return 0;

}
void lidbg_flyparameter_deinit(void)
{
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.futengfei.2014.7.10");

module_init(lidbg_flyparameter_init);
module_exit(lidbg_flyparameter_deinit);


EXPORT_SYMBOL(g_recovery_meg);
EXPORT_SYMBOL(flyparameter_info_save);


