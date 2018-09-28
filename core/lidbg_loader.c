
#define NOT_USE_MEM_LOG

#include "lidbg.h"

#define TAG "dlidbg_loader:"

#define  RECOVERY_MODE_DIR "/sbin/recovery"
#define  FLY_MODE_FILE "/flysystem/lib/out/lidbg_loader.ko"
#define  DEBUG_MODE_FILE "/data/out/lidbg_loader.ko"

int load_modules_count = 0;
enum boot_mode gboot_mode;

char *insmod_soc_list[] =
{
    SOC_KO,
    NULL,
};

char *insmod_list[] =
{
    "lidbg_mem_log.ko",
    "lidbg_common.ko",
    "lidbg_fileserver.ko",
    //"lidbg_trace_msg.ko",
    "lidbg_wakelock_stat.ko",
    "lidbg_msg.ko",
    "lidbg_servicer.ko",
    "lidbg_touch.ko",
    "lidbg_spi.ko",
    "lidbg_key.ko",
    "lidbg_i2c.ko",
    "lidbg_io.ko",
    //"lidbg_uart.ko",
    "lidbg_main.ko",
    "lidbg_interface.ko",
    "lidbg_flyparameter.ko",
    "lidbg_bare_para.ko",
    "lidbg_drivers_loader.ko",
    NULL,
};

bool is_file_exist(char *file)
{
    struct file *filep;
    filep = filp_open(file, O_RDONLY , 0);
    if(IS_ERR(filep))
        return false;
    else
    {
        filp_close(filep, 0);
        return true;
    }
}

void lidbg_insmod( char argv1[])
{
    char shell_cmd[256];
    sprintf(shell_cmd, "insmod %s", argv1 == NULL ? " " : argv1);
    lidbg_uevent_shell(shell_cmd);
}

int thread_check_restart(void *data)
{
    DUMP_FUN_ENTER;
    ssleep(10);
    LIDBG_WARN(TAG"load_modules_count=%d\n", load_modules_count);
    if(load_modules_count == 0)
    {
        LIDBG_ERR(TAG"load_modules_count err,call kernel_restart!\n");

#ifdef SOC_msm8x25
        kernel_restart(NULL);
#endif
    }
    DUMP_FUN_LEAVE;
    return 0;
}

int thread_loader(void *data)
{
    int  j;
    char path[128] = {0}, *kopath = NULL;
    int tmp;
    DUMP_FUN_ENTER;

    while(!is_lidbg_uevent_ready)
    {
        lidbg(TAG"wait is_lidbg_uevent_ready\n");
        msleep(200);
    }

    CREATE_KTHREAD(thread_check_restart, NULL);

    if(is_file_exist(RECOVERY_MODE_DIR))
        gboot_mode = MD_RECOVERY;
    else if(is_file_exist(FLY_MODE_FILE))
        gboot_mode = MD_FLYSYSTEM;
    else
        gboot_mode = MD_ORIGIN;

    if(gboot_mode == MD_FLYSYSTEM)
        kopath = "/flysystem/lib/out/";
    else
        kopath = "/system/lib/modules/out/";

    if(is_file_exist(DEBUG_MODE_FILE))
    {
        gboot_mode = MD_DEBUG;
        kopath = "/data/out/";
    }

    lidbg(TAG"thread_loader start,%d\n", gboot_mode);
    for(j = 0; insmod_soc_list[j] != NULL; j++)
    {
        sprintf(path, "%s%s", kopath, insmod_soc_list[j]);
        lidbg_insmod(path);
    }

    for(j = 0; insmod_list[j] != NULL; j++)
    {
        tmp = load_modules_count;
        sprintf(path, "%s%s", kopath, insmod_list[j]);
        lidbg_insmod(path);
        while(tmp == load_modules_count) msleep(10);
    }

    DUMP_FUN_LEAVE;
    return 0;

}

int __init loader_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_WARN(TAG"in\n");
    CREATE_KTHREAD(thread_loader, NULL);
    return 0;
}

void __exit loader_exit(void)
{}

module_init(loader_init);
module_exit(loader_exit);

EXPORT_SYMBOL(gboot_mode);
EXPORT_SYMBOL(load_modules_count);
EXPORT_SYMBOL(lidbg_insmod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");
