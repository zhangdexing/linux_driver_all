

#include "lidbg.h"
int  soc_set_screen_active_area(u32 width, u32 height)
{
    int fbidx;
    LIDBG_WARN("num_registered_fb = %d \n", num_registered_fb);

    for(fbidx = 0; fbidx < num_registered_fb; fbidx++)
    {
        if(fbidx == 0)
        {
            struct fb_info *info = registered_fb[fbidx];
            if (!info)
            {
                LIDBG_WARN("info=null\n");
                continue;
            }
            info->var.height = height;
            info->var.width = width;
            LIDBG_WARN("height=%d/width=%d\n", info->var.height, info->var.width);
        }
    }
    return 1;
}
int immediate_file_read(const char *filename, char *rbuff, loff_t offset, int readlen)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int read_len = 1;

    filep = filp_open(filename,  O_RDONLY, 0);
    if(IS_ERR(filep))
        return -1;
    old_fs = get_fs();
    set_fs(get_ds());

    filep->f_op->llseek(filep, offset, SEEK_SET);
    read_len = filep->f_op->read(filep, rbuff, readlen, &filep->f_pos);

    set_fs(old_fs);
    filp_close(filep, 0);
    return read_len;
}

int set_screen_active_area(void)
{
    char buff[32] = {0};
    int lcd_type_pos = 5, lcd_type;
    int len = immediate_file_read("/persist/hwinfo.txt", buff, 0, sizeof(buff));
    if(len >= lcd_type_pos)
    {
        buff[lcd_type_pos] = '\0';
        lcd_type = simple_strtoul(&buff[lcd_type_pos - 1], 0, 0);
    }
    else
        lcd_type = -1;
    LIDBG_WARN("hwinfo:[%s/%d/%d]\n",  buff, len, lcd_type);
    switch(lcd_type)
    {
    case 1:
        soc_set_screen_active_area(196.608 , 147.456  );
        break;
    case 2:
        soc_set_screen_active_area(196.608 , 147.456  );
        break;
    case 3:
        soc_set_screen_active_area(196.608 , 147.456  );
        break;
    default :
        break;
    }
    return lcd_type;
}
int thread_set_screen_active_area(void *data)
{
    int loop = 0;
    LIDBG_WARN("in");
    while(set_screen_active_area() == -1 && loop++ < 100)
        msleep(100);
    return 0;
}

int __init immediate_init(void)
{
    LIDBG_WARN("in");
    kthread_run(thread_set_screen_active_area, NULL, "ftf_set_screen");
    return 0;
}

void __exit immediate_exit(void)
{}

module_init(immediate_init);
module_exit(immediate_exit);

EXPORT_SYMBOL(immediate_file_read);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.2016-08-26 11:07:25");
