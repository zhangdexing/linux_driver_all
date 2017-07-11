
#include "lidbg.h"
#define RECOVERY_PATH_FLY_HW_INFO_CONFIG RECOVERY_USB_MOUNT_POINT"/machine_info.conf"

#define FLAG_HW_INFO_VALID (0x12345678)
#define PATA_TAG "flypara_tag:"
#define EXTRA_FLYPARA_MIRROR "/dev/flyparameter"

LIDBG_DEFINE;

static fly_hw_data *g_fly_hw_data = NULL;
recovery_meg_t *g_recovery_meg = NULL;
recovery_meg_t default_recovery_meg ;
char *p_kmem = NULL;
int update_hw_info = 0;
static char *p_flyparameter_node = NULL;
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
    lidbg(PATA_TAG"flyparameter:%s:g_fly_hw_data:flag=%x,%x,hw=%d,ts=%d,%d,lcd=%d\n", when,
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
    {
        char buff[128] = {0};
        fs_fill_list(get_udisk_file_path(buff, "conf/machine_info.conf"), FS_CMD_FILE_CONFIGMODE, &hw_config_list);
    }

    fs_get_intvalue(&hw_config_list, "hw_version", &(p_info->hw_info.hw_version), NULL);
    fs_get_intvalue(&hw_config_list, "ts_type", &(p_info->hw_info.ts_type), NULL);
    fs_get_intvalue(&hw_config_list, "ts_config", &(p_info->hw_info.ts_config), NULL);
    fs_get_intvalue(&hw_config_list, "lcd_type", &(p_info->hw_info.lcd_type), NULL);
    fly_hw_info_show("fs_fill_list", p_info);
}

bool fly_hw_info_get(fly_hw_data *p_info)
{
    if(p_flyparameter_node == NULL)
    {
        lidbg(PATA_TAG"g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }
    if(p_info && fs_file_read(p_flyparameter_node, (char *)p_info, MEM_SIZE_512_KB , sizeof(fly_hw_data)) >= 0)
    {
        fly_hw_info_show("fly_hw_info_get", p_info);
        return true;
    }
    return false;
}

bool fly_hw_info_save(fly_hw_data *p_info)
{
    DUMP_FUN;
    if(p_flyparameter_node == NULL)
    {
        lidbg(PATA_TAG"g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }
    read_fly_hw_config_file(p_info);
    if( p_info && fs_file_write(p_flyparameter_node, false, (void *) p_info, MEM_SIZE_512_KB , sizeof(fly_hw_data)) >= 0)
    {
        lidbg(PATA_TAG"fly_hw_data:save success\n");
        update_info = UPDATE_SUC;
        return true;
    }
    lidbg(PATA_TAG"fly_hw_data:save err\n");
    update_info = UPDATE_FAIL;
    return false;
}

/*
void fly_hw_info_save_from(char *where)
{
    char buff[128] = {0};
    if((update_hw_info != 0) && (fs_is_file_exist(get_udisk_file_path(buff, "conf/machine_info.conf"))))
    {
        g_fly_hw_data->flag_need_update = 0;
        g_fly_hw_data->flag_hw_info_valid = FLAG_HW_INFO_VALID;
        fly_hw_info_save(g_fly_hw_data);
    }
}
*/
bool flyparameter_info_get(void)
{
    bool is_ublox_so_exist = false;

    if(p_flyparameter_node == NULL)
    {
        lidbg(PATA_TAG"g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }

    if(p_kmem && fs_file_read(p_flyparameter_node, p_kmem, 0, sizeof(recovery_meg_t)) >= 0)
    {
        g_recovery_meg = (recovery_meg_t *)p_kmem;
        lidbg(PATA_TAG"flyparameter1:%s,%s,%s\n", g_recovery_meg->recoveryLanguage.flags, g_recovery_meg->bootParam.bootParamsLen.flags, g_recovery_meg->bootParam.upName.flags);
        lidbg(PATA_TAG"flyparameter2:%d,%s,%x\n", g_recovery_meg->bootParam.upName.val, g_recovery_meg->bootParam.autoUp.flags, g_recovery_meg->bootParam.autoUp.val);
        lidbg(PATA_TAG"g_recovery_meg %x\n", g_recovery_meg->hwInfo.bValid);

        if(g_recovery_meg->hwInfo.bValid == 0x12345678)
        {
            int ret, len;
            char parameter[256];
            memset(parameter, '\0', sizeof(parameter));
            len = strlen(g_recovery_meg->hwInfo.info);

            ret = sprintf(parameter, "setprop HWINFO.FLY.Parameter %s", g_recovery_meg->hwInfo.info);
            if(ret < 0)
            {
                lidbg(PATA_TAG"fail to cpy parameter\n");
            }
            lidbg_shell_cmd( parameter );
            fs_clear_file("/data/commenPersist/hwinfo.txt");
            fs_file_write2("/data/commenPersist/hwinfo.txt", g_recovery_meg->hwInfo.info);
            lidbg_shell_cmd("chmod 777 /data/commenPersist/hwinfo.txt");
            lidbg(PATA_TAG"flyparameter=[%s] len:%d\n", g_recovery_meg->hwInfo.info, len);
            fs_mem_log("flyparameter=[%s] len:%d\n", g_recovery_meg->hwInfo.info, len);

            lidbg(PATA_TAG"flyparameter3 :%d,%d,%d,%d,%d\n", g_recovery_meg->hwInfo.info[0] - '0', g_recovery_meg->hwInfo.info[1] - '0', g_recovery_meg->hwInfo.info[2] - '0', g_recovery_meg->hwInfo.info[3] - '0', g_recovery_meg->hwInfo.info[4] - '0');
            g_var.hw_info.ts_config = 10 * (g_recovery_meg->hwInfo.info[0] - '0') + g_recovery_meg->hwInfo.info[1] - '0';
            g_var.hw_info.virtual_key = 10 * (g_recovery_meg->hwInfo.info[2] - '0') + g_recovery_meg->hwInfo.info[3] - '0';
            g_var.hw_info.lcd_type = g_recovery_meg->hwInfo.info[4] - '0';
            g_var.hw_info.lcd_manufactor = 10 * (g_recovery_meg->hwInfo.info[8] - '0') + g_recovery_meg->hwInfo.info[9] - '0';
            if(len > 10)
                g_var.hw_info.hw_version2 =  g_recovery_meg->hwInfo.info[10] - '0';
#ifdef PLATFORM_msm8996
            g_var.hw_info.hw_version = g_var.hw_info.hw_version2 + 2;
#endif

            lidbg(PATA_TAG"ts_config:%d,virtual_key:%d,lcd_manufactor:%d,hw_version:%d\n", g_var.hw_info.ts_config, g_var.hw_info.virtual_key, g_var.hw_info.lcd_manufactor, g_var.hw_info.hw_version2);

            is_ublox_so_exist = fs_is_file_exist("/flysystem/lib/out/"FLY_GPS_SO);
            lidbg(PATA_TAG"ts_config5:gps:%c,%d\n", g_recovery_meg->hwInfo.info[5] , is_ublox_so_exist);
            if((g_recovery_meg->hwInfo.info[5] == '1') && (is_ublox_so_exist))// 0 - ublox ,1 -qualcomm gps
            {
                lidbg(PATA_TAG"rm ublox so\n");
                lidbg_shell_cmd("mount -o remount /flysystem");
                lidbg_shell_cmd("rm /flysystem/lib/out/"FLY_GPS_SO);
                lidbg_shell_cmd("mount -o remount,ro /flysystem");
            }
            {
                char set_car_type[256];
                memset(set_car_type, '\0', sizeof(set_car_type));
                g_var.car_type = g_recovery_meg->bootParam.upName.flags;
                lidbg(PATA_TAG"car_type=%s\n", g_var.car_type);

                ret = sprintf(set_car_type, "setprop HWINFO.FLY.car_type %s", g_var.car_type);
                if(ret < 0)
                {
                    lidbg(PATA_TAG"fail to cpy car_type\n");
                }
                lidbg_shell_cmd( set_car_type );
                lidbg_shell_cmd( "mount -o rw,remount rootfs /" );
                lidbg_shell_cmd( "chmod 777 /flyconfig" );
                lidbg_shell_cmd( "/flysystem/bin/decodeFlyconfig &" );
                lidbg(PATA_TAG"bring up:/flysystem/bin/decodeFlyconfig &\n");
                fs_mem_log("car_type=[%s]\n", g_var.car_type);
            }

#if 0
            if( !strncmp(g_var.car_type, "822", 3))//Israel
            {
                lidbg(PATA_TAG"Israel car_type,suspend_airplane_mode\n");
                g_var.suspend_airplane_mode = true;
            }
#endif
            return true;
        }
    }
    else
    {
        lidbgerr(PATA_TAG"\n===flyparameter.disable===\n");
        g_recovery_meg = &default_recovery_meg;
    }
    return false;
}
//simple_strtoul(argv[0], 0, 0);
bool flyparameter_info_save(recovery_meg_t *p_info)
{
    if(p_flyparameter_node == NULL)
    {
        lidbg(PATA_TAG"g_hw.fly_parameter_node == NULL,return\n");
        return 0;
    }
    if( p_info && fs_file_write(p_flyparameter_node, false, (void *) p_info, 0, sizeof(recovery_meg_t)) >= 0)
    {
        lidbg(PATA_TAG"flyparameter:save success\n");
        return true;
    }
    lidbg(PATA_TAG"flyparameter:save err\n");
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
    lidbg(PATA_TAG"read info is %c\n", info);
    msleep(40000);
    {
        char cmd[128] = {0};
        sprintf(cmd, "chmod 777 %s", p_flyparameter_node);
        lidbg_shell_cmd(cmd);
    }
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
        lidbgerr(PATA_TAG"kzalloc.g_fly_hw_data\n");

    /*
        if(fs_is_file_exist(PATH_MACHINE_INFO_FILE))
        {
            lidbgerr(PATA_TAG"return.%s,miss\n", PATH_MACHINE_INFO_FILE);
            return -1;
        }
    */
    if(!fly_hw_info_get(g_fly_hw_data))
        lidbgerr(PATA_TAG"fly_hw_info_get\n");

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
            //lidbg(PATA_TAG"i=%d,val1=%d,val2=%d\n",i,((int*)(&g_fly_hw_data->hw_info))[i],((int*)(&g_var.hw_info))[i]);
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
        lidbg(PATA_TAG"copy_to_user ERR\n");
    }
    lidbg(PATA_TAG"update_info = %d,read size =%zd\n\n", update_info, size);
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
        lidbgerr(PATA_TAG"kzalloc.p_kmem\n");

    if(!flyparameter_info_get())
        lidbg(PATA_TAG"flyparameter_info_get\n");

    return 0;
}

int lidbg_flyparameter_init(void)
{
    int cnt = 0;
    DUMP_BUILD_TIME;
    LIDBG_GET;
    p_flyparameter_node = FLYPARAMETER_NODE;
    lidbg(PATA_TAG"p_flyparameter_node:%s\n", p_flyparameter_node);
    {
        char cmd[128] = {0};
        sprintf(cmd, "chmod 777 %s", FLYPARAMETER_NODE);
        lidbg_shell_cmd(cmd);
    }
    while((fs_is_file_exist(p_flyparameter_node) == 0) && (cnt < 50))
    {
        lidbg(PATA_TAG"wait for FLYPARAMETER_NODE !!\n");
        {
            char cmd[128] = {0};
            sprintf(cmd, "chmod 777 %s", p_flyparameter_node);
            lidbg_shell_cmd(cmd);
        }
        msleep(200);
        cnt++;
    }
    if(fs_is_file_exist(EXTRA_FLYPARA_MIRROR))
        p_flyparameter_node = EXTRA_FLYPARA_MIRROR;
    else
        lidbg(PATA_TAG"p_flyparameter_node,use default\n");

    lidbg(PATA_TAG"p_flyparameter_node.in:EXTRA_FLYPARA_MIRROR:%d %s\n", fs_is_file_exist(EXTRA_FLYPARA_MIRROR), p_flyparameter_node);
    cnt = 0;
    while((fs_is_file_exist(p_flyparameter_node) == 0) && (cnt < 200))
    {
        lidbg(PATA_TAG"p_flyparameter_node:exist:%d %s\n", fs_is_file_exist(p_flyparameter_node), p_flyparameter_node);
        msleep(200);
        cnt++;
    }

    flyparameter_init();
    lidbg_fly_hw_info_init();//block other ko before hw_info set
    lidbg_new_cdev(&fly_upate_info_fops, "fly_upate_info0");
    CREATE_KTHREAD(thread_fix_fly_update_info, NULL);
    p_flyparameter_node = FLYPARAMETER_NODE;
    lidbg(PATA_TAG"p_flyparameter_node.restore:%s\n", p_flyparameter_node);
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


