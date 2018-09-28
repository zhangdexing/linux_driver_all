
#include "lidbg.h"
#define DEVICE_NAME "lidbg_msg"


static DECLARE_COMPLETION(msg_ready);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
DECLARE_MUTEX(lidbg_msg_sem);
#else
DEFINE_SEMAPHORE(lidbg_msg_sem);
#endif


static int thread_msg(void *data);

#define TOTAL_LOGS  (100)
#define LOG_BYTES   (256)

typedef struct
{
    int w_pos;
    int r_pos;
    char log[TOTAL_LOGS][LOG_BYTES];
} lidbg_msg;

lidbg_msg *plidbg_msg = NULL;


int thread_msg(void *data)
{


    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1)
        {
            wait_for_completion(&msg_ready);

            lidbg("[fly_msg] %s", plidbg_msg->log[plidbg_msg->r_pos]);
            plidbg_msg->r_pos = (plidbg_msg->r_pos + 1)  % TOTAL_LOGS;

        }
        else
        {
            schedule_timeout(HZ);
        }
    }
    return 0;
}


ssize_t  msg_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    return size;
}

ssize_t  msg_write(struct file *filp, const char __user *buffer, size_t size, loff_t *offset)
{


    down(&lidbg_msg_sem);
    if(copy_from_user(&(plidbg_msg->log[plidbg_msg->w_pos]), buffer, size > LOG_BYTES ? LOG_BYTES : size))
    {
        lidbg("copy_from_user ERR\n");
    }

    //for safe
    plidbg_msg->log[plidbg_msg->w_pos][(size > LOG_BYTES ? LOG_BYTES - 1 : size)] = '\0';

    plidbg_msg->w_pos = (plidbg_msg->w_pos + 1)  % TOTAL_LOGS;

    up(&lidbg_msg_sem);

    complete(&msg_ready);

    return size;
}


int msg_open(struct inode *inode, struct file *filp)
{
    //down(&lidbg_msg_sem);

    return 0;
}

int msg_release(struct inode *inode, struct file *filp)
{
    //up(&lidbg_msg_sem);
    return 0;
}


static struct file_operations dev_fops =
{
    .owner	=	THIS_MODULE,
    .open   =   msg_open,
    .read   =   msg_read,
    .write  =   msg_write,
    .release =  msg_release,
};

static int __init msg_init(void)
{
    plidbg_msg = ( lidbg_msg *)vmalloc(sizeof( lidbg_msg));
    if (plidbg_msg == NULL)
    {
        LIDBG_ERR("vmalloc.\n");
    }
    memset(plidbg_msg->log, '\0', /*sizeof( lidbg_msg)*/TOTAL_LOGS * LOG_BYTES);
    plidbg_msg->w_pos = plidbg_msg->r_pos = 0;


    lidbg_new_cdev(&dev_fops, DEVICE_NAME);

    CREATE_KTHREAD(thread_msg, NULL);

    lidbg_shell_cmd("chmod 777 /dev/lidbg_msg");

    LIDBG_MODULE_LOG;

    return 0;
}

static void __exit msg_exit(void)
{
    lidbg (DEVICE_NAME"msg  dev_exit\n");
}

module_init(msg_init);
module_exit(msg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");


