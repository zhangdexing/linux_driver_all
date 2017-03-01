
#include "lidbg.h"

#define DEVICE_NAME "lidbg_trace_msg0"
#define LIDBG_TRACE_MSG_FIFO_SIZE		(32  * 1024)

static LIST_HEAD(lidbg_trace_msg_string_list);
static LIST_HEAD(lidbg_trace_msg_cb_list_head);

struct kfifo fifo_kmsg_collect, *p_kmsg_collect = NULL;
spinlock_t spinlock_kmsg_collect;
struct mutex mutex_kmsg_collect;
void kmsg_fifo_collect(char *buff, int buff_len)
{
    if(p_kmsg_collect == NULL)
    {
        if(kfifo_alloc(&fifo_kmsg_collect, 1 * 1024 * 1024, GFP_KERNEL))
        {
            lidbgerr("[%s]:kfifo_alloc \n", __func__);
            return;
        }
        mutex_init(&mutex_kmsg_collect);
        spin_lock_init(&spinlock_kmsg_collect);
        p_kmsg_collect = &fifo_kmsg_collect;
        kfifo_reset(p_kmsg_collect);
    }

    if(kfifo_is_full(p_kmsg_collect) || kfifo_avail(p_kmsg_collect) < buff_len)
    {
#if 1
        int ret = 0, tmp;
        char tmp_buff[512];
        while(ret < 256 * 2)
        {
            ret++;
            tmp = kfifo_out_spinlocked(p_kmsg_collect, tmp_buff, sizeof(tmp_buff) , &spinlock_kmsg_collect);
        }
        lidbg("[%s]:kfifo_is_full\n", __func__);
#else
        kfifo_reset(p_kmsg_collect);
#endif
    }
    mutex_lock(&mutex_kmsg_collect);
    kfifo_in_spinlocked(p_kmsg_collect, buff, buff_len, &spinlock_kmsg_collect);
    mutex_unlock(&mutex_kmsg_collect);

}

void kmsg_fifo_save(void)
{
    int ret;
    if( pdev->disable_flag ==  1)
		   return ;
    if(p_kmsg_collect && !kfifo_is_empty(p_kmsg_collect))
    {
        char *file_ptr = NULL;
        int fifo_len = 0;
        fifo_len = kfifo_len(p_kmsg_collect);
        lidbg("[%s]:save start %d\n", __func__, fifo_len);

        file_ptr = vmalloc(fifo_len + 1);
        if(!file_ptr || fifo_len <= 0)
        {
            lidbgerr( "vmalloc|%d\n", fifo_len);
            return ;
        }

        ret = kfifo_out(p_kmsg_collect, file_ptr, fifo_len);
        file_ptr[ret] = '\0';
        fs_file_separator(LIDBG_LOG_DIR"lidbg_kmsg_5M.txt");
        bfs_file_amend(LIDBG_LOG_DIR"lidbg_kmsg_5M.txt", file_ptr, 6);
        vfree(file_ptr);
        lidbg("[%s]:save complete %d\n", __func__, ret);
    }
    else
        lidbg("[%s]:skip: %d  %d\n", __func__, p_kmsg_collect == NULL, p_kmsg_collect != NULL ? kfifo_is_empty(p_kmsg_collect) : -1);
}



struct lidbg_trace_message_device
{
    char *name;
    int disable_flag;
    struct kfifo fifo;
    struct semaphore sem;
};

struct lidbg_trace_msg_cb_list
{
    struct list_head tmp_list;
    char *key_word;
    void *data;
    void (*cb_func)(char *key_word, void *data);
};

static struct lidbg_trace_message_device *pdev;

void lidbg_trace_msg_disable(int flag)
{
    pdev->disable_flag = flag;
    lidbg("lidbg_trace_msg_disable\n");
}
EXPORT_SYMBOL(lidbg_trace_msg_disable);

bool lidbg_trace_msg_cb_register(char *key_word, void *data, void (*cb_func)(char *key_word, void *data))
{
    struct lidbg_trace_msg_cb_list *cb_list = NULL;
    if( pdev->disable_flag ==  1)
		   return true;
    cb_list = kzalloc(sizeof(struct lidbg_trace_msg_cb_list), GFP_KERNEL);
    if(!cb_list)
        lidbg("[%s]: Kzalloc mem for lidbg_trace_msg_cb_list faild !\n", __func__);

    if(key_word && cb_func)
    {
        cb_list->key_word = key_word;
        cb_list->data = data;
        cb_list->cb_func = cb_func;
        list_add(&(cb_list->tmp_list), &lidbg_trace_msg_cb_list_head);		//attention: some protection on lidbg_trace_msg_cb_list_head will be better
        return true;
    }
    else
        return false;
}
EXPORT_SYMBOL(lidbg_trace_msg_cb_register);

static void lidbg_trace_key_word(char *string)
{
    char *ret = NULL;
    struct string_dev *pmsg;
    struct lidbg_trace_msg_cb_list *cb_list;

    //down(&pdev->sem);
    list_for_each_entry(pmsg, &lidbg_trace_msg_string_list, tmp_list)
    {
        if(strlen(pmsg->yourkey))
            ret = strstr(string, pmsg->yourkey);
        if(ret)
        {
            bfs_file_amend(LIDBG_TRACE_MSG_PATH, string, 0);
            ret = NULL;
        }
    }

    list_for_each_entry(cb_list, &lidbg_trace_msg_cb_list_head, tmp_list)	//attention: some protection on lidbg_trace_msg_cb_list_head will be better
    {
        if(strlen(cb_list->key_word))
            ret = strstr(string, cb_list->key_word);
        if(ret)
        {
            cb_list->cb_func(cb_list->key_word, cb_list->data);
            ret = NULL;
        }

    }
    //up(&pdev->sem);
}

static void lidbg_trace_msg_is_enough(int len)
{
    if(kfifo_is_full(&pdev->fifo) || (kfifo_avail(&pdev->fifo) < len))
    {
#if 0
        int ret;
        char msg_clean_buff[len];

        down(&pdev->sem);
        ret = kfifo_out(&pdev->fifo, msg_clean_buff, len);
        up(&pdev->sem);
#else
        down(&pdev->sem);
        kfifo_reset(&pdev->fifo);
        up(&pdev->sem);

#endif
    }

}

#ifdef  TRACE_MSG_FROM_KMSG
static int thread_trace_msg_in(void *data)
{
    int len;
    char *buff;
    struct file *filep;
    mm_segment_t old_fs;
    buff = (char *)vmalloc( MEM_SIZE_4_KB);
    if(buff == NULL)
    {
        lidbg("thread_trace_msg_in.ERR.kmalloc.exit\n");
        return 0;
    }

    filep = filp_open("/proc/kmsg", O_RDONLY, 0644);
    if(filep && !IS_ERR(filep) && filep->f_op && filep->f_op->read)
    {
        lidbg("thread_trace_msg_in.while\n");
    }
    else
    {
        lidbg("thread_trace_msg_in.ERR.filp_open.exit\n");
        return 0;
    }


    memset(buff, '\0', MEM_SIZE_4_KB);

    old_fs = get_fs();
    set_fs(get_ds());

    while(1)
    {
        if(!pdev->disable_flag)
        {
            len = filep->f_op->read(filep, buff, MEM_SIZE_4_KB, &filep->f_pos);

            if(len >= 0)
            {
                kmsg_fifo_collect(buff, len);
                lidbg_trace_msg_is_enough(len);

                down(&pdev->sem);
                kfifo_in(&pdev->fifo, buff, len);
                up(&pdev->sem);
            }
        }

        msleep(500);
    }

    set_fs(old_fs);

    filp_close(filep, 0);
    return 0;
}
#endif

static int thread_trace_msg_out(void *data)
{
    int i = 0;
    int len;
    char buff[512];

    memset(buff, '\0', sizeof(buff));
    while(1)
    {

        if(!pdev->disable_flag)
        {
            if(kfifo_is_empty(&pdev->fifo))
            {
                msleep(100);
                continue;
            }

            down(&pdev->sem);
            len = kfifo_out(&pdev->fifo, &buff[i], 1);
            up(&pdev->sem);

            if(buff[i] == '\n' || (i >= 510))
            {
                buff[i + 1] = '\0';
                lidbg_trace_key_word(buff);
                i = 0;
                continue;
            }

            i++;
        }
        else
            msleep(1000);
    }

    return 0;

}

static int lidbg_trace_msg_open (struct inode *inode, struct file *filp)
{
    DUMP_FUN;
    filp->private_data = pdev;
    return 0;
}

static ssize_t lidbg_trace_msg_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    DUMP_FUN;
    return 0;
}

static ssize_t lidbg_trace_msg_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;
    char pbuff[256];
    memset(pbuff, '\0', sizeof(pbuff));

    if (copy_from_user( pbuff, buf, count))
        lidbg("Lidbg msg copy_from_user ERR\n");

    lidbg_trace_msg_is_enough(count);

    down(&pdev->sem);
    kfifo_in(&pdev->fifo, pbuff, count);
    up(&pdev->sem);

    return ret;
}

int lidbg_trace_msg_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static  struct file_operations lidbg_trace_msg_fops =
{
    .owner = THIS_MODULE,
    .read = lidbg_trace_msg_read,
    .write = lidbg_trace_msg_write,
    .open = lidbg_trace_msg_open,
    .release = lidbg_trace_msg_release,
};
void callback_disable_trace_msg(char *focus, void *data)
{
    lidbg_trace_msg_disable(1);
}

static int tmp = 0;
void cb_int_kmsg_fifo_save(char *key, char *value )
{
    kmsg_fifo_save();
}
static int  lidbg_trace_msg_probe(struct platform_device *ppdev)
{
    int ret;

    DUMP_FUN;
    pdev = (struct lidbg_trace_message_device *)kmalloc( sizeof(struct lidbg_trace_message_device), GFP_KERNEL);
    if (pdev == NULL)
    {
        ret = -ENOMEM;
        lidbg("Kmalloc space for lidbg msg failed.\n");
        return ret;
    }
	
    pdev->disable_flag = 1;
    return 0;
	
    ret = kfifo_alloc(&pdev->fifo, LIDBG_TRACE_MSG_FIFO_SIZE, GFP_KERNEL);
    if(ret)
    {
        lidbg("Alloc kfifo for lidbg dev failed.\n");
        return ret;
    }
    pdev->disable_flag = 1;

    sema_init(&pdev->sem, 1);

    if(fs_fill_list("/flysystem/lib/out/lidbg_trace_msg_string_list.conf", FS_CMD_FILE_LISTMODE, &lidbg_trace_msg_string_list) < 0)
        fs_fill_list("/system/lib/modules/out/lidbg_trace_msg_string_list.conf", FS_CMD_FILE_LISTMODE, &lidbg_trace_msg_string_list);

    fs_file_separator(LIDBG_TRACE_MSG_PATH);
    fs_register_filename_list(LIDBG_TRACE_MSG_PATH, true);
	
#if (defined(PLATFORM_ID_14) || defined(SOC_mt35x))
    pdev->disable_flag = 1;
#else
    FS_REGISTER_INT(pdev->disable_flag, "trace_msg_disable", 1, NULL);
#endif
    FS_REGISTER_INT(tmp, "kmsg_fifo_save", 0, cb_int_kmsg_fifo_save);

#ifdef  TRACE_MSG_FROM_KMSG
    if((gboot_mode == MD_FLYSYSTEM) || (gboot_mode == MD_DEBUG))
        CREATE_KTHREAD(thread_trace_msg_in, NULL);
#endif
    CREATE_KTHREAD(thread_trace_msg_out, NULL);

    lidbg_new_cdev(&lidbg_trace_msg_fops, DEVICE_NAME);
//#ifndef USE_CALL_USERHELPER
 //   lidbg_uevent_focus("USB_STATE=CONFIGURED", callback_disable_trace_msg ); //USB_STATE=DISCONNECTED
//#endif
    lidbg_trace_msg_cb_register("USB_STATE=CONFIGURED", NULL, callback_disable_trace_msg);

    {
        char buff[32] = {0};
        fs_file_read("/sys/class/android_usb/android0/state", buff, 0, sizeof(buff));
        lidbg("state:%s\n", buff);
        if(!strncmp(buff, "CONFIGURED", strlen("CONFIGURED")))
            lidbg_trace_msg_disable(1);
    }

    LIDBG_MODULE_LOG;

    return 0;
}


static int  lidbg_trace_msg_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver lidbg_trace_msg_driver =
{
    .probe		= lidbg_trace_msg_probe,
    .remove     = lidbg_trace_msg_remove,
    .driver         = {
        .name = "lidbg_trace_msg",
        .owner = THIS_MODULE,
    },
};

static struct platform_device lidbg_trace_msg_device =
{
    .name               = "lidbg_trace_msg",
    .id                 = -1,
};

static int __init lidbg_trace_msg_init(void)
{
    DUMP_BUILD_TIME;
    platform_device_register(&lidbg_trace_msg_device);

    return platform_driver_register(&lidbg_trace_msg_driver);
}

static void __exit lidbg_trace_msg_exit(void)
{
    DUMP_FUN;

    platform_device_unregister(&lidbg_trace_msg_device);
    platform_driver_unregister(&lidbg_trace_msg_driver);
}

void trace_msg_main(int argc, char **argv)
{
    lidbg("trace_msg_main:%s\n", argv[0]);
    if(!strncmp(argv[0], "disable", strlen("disable")))
        lidbg_trace_msg_disable(1);
    else if(!strncmp(argv[0], "save", strlen("save")))
        kmsg_fifo_save();
    else if(!strncmp(argv[0], "len", strlen("len")))
        lidbg("[%s]:kfifo_len: %d byte/%d KB\n", __func__, kfifo_len(p_kmsg_collect), kfifo_len(p_kmsg_collect) / 1024);

}

module_init(lidbg_trace_msg_init);
module_exit(lidbg_trace_msg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");
EXPORT_SYMBOL(trace_msg_main);
EXPORT_SYMBOL(kmsg_fifo_save);


