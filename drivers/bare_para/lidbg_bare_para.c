
#include "lidbg.h"
LIDBG_DEFINE;

static fly_bare_data *g_bare_data = NULL;
#define BARE_DATA_NODE "/dev/block/bootdevice/by-name/misc"
#define TAG "bare_tag:"
void bare_data_show(char *who, fly_bare_data *p_info)
{
    lidbg(TAG"%s:flag_valid=%x,flag_dirty=%x,uart_dbg_en=%d\n", who,
          p_info->flag_valid, p_info->flag_dirty, p_info->bare_info.uart_dbg_en);
}
bool bare_data_write(char *who, fly_bare_data *p_info)
{
    bool status = false;
    p_info->flag_valid = FLAG_VALID;
    lidbg_shell_cmd("chmod 777 $(ls -l /dev/block/bootdevice/by-name/misc | cut -d \">\" -f2)");
    msleep(200);
    if( p_info && fs_file_write(BARE_DATA_NODE, false, (void *) p_info, MEM_SIZE_512_KB , sizeof(fly_bare_data)) >= 0)
    {
        lidbg(TAG"%s,save success\n", who);
        status = true;
    }
    else
    {
        lidbg(TAG"%s,save fail\n", who);
    }
    return status;
}
fly_bare_data *bare_data_get(char *who)
{
    return g_bare_data;
}

int lidbg_bare_info_init(void)
{
    g_bare_data = kzalloc(sizeof(fly_bare_data), GFP_KERNEL);
    if(!g_bare_data)
        lidbgerr(TAG"kzalloc.g_bare_data\n");
    lidbg(TAG"%d,%d\n", (int)sizeof(fly_bare_data), fs_is_file_exist(BARE_DATA_NODE));

    if(g_bare_data && fs_file_read(BARE_DATA_NODE, (char *)g_bare_data, MEM_SIZE_512_KB , sizeof(fly_bare_data)) >= 0)
    {
        bare_data_show("read", g_bare_data);
        return true;
    }
    return 0;
}

int bare_info_open(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t  bare_info_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    return size;
}
static  struct file_operations bare_info_fops =
{
    .owner = THIS_MODULE,
    .open = bare_info_open,
    .read = bare_info_read,
};

int lidbg_bare_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;

    lidbg_bare_info_init();//block other ko before hw_info set
    lidbg_new_cdev(&bare_info_fops, "lidbg_bare_info0");
    LIDBG_MODULE_LOG;
    return 0;

}
void lidbg_bare_deinit(void)
{
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.futengfei.2017.4.19");

module_init(lidbg_bare_init);
module_exit(lidbg_bare_deinit);


EXPORT_SYMBOL(bare_data_get);
EXPORT_SYMBOL(bare_data_write);
EXPORT_SYMBOL(bare_data_show);


