#include <../drivers/flyaudio/lidbg_interface.h>
#include "lidbg_def.h"
#include "cmn_func.h"

#define DEVICE_NAME "dbg_msg"


static DECLARE_COMPLETION(msg_ready);


DEFINE_SEMAPHORE(lidbg_msg_sem);

static int thread_msg(void *data);

#define TOTAL_LOGS  (50)
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

    plidbg_msg = ( lidbg_msg *)vmalloc(sizeof( lidbg_msg));
    if (plidbg_msg == NULL)
    {
        LIDBG_ERR("vmalloc.\n");
    }
    memset(plidbg_msg->log, '\0', /*sizeof( lidbg_msg)*/TOTAL_LOGS * LOG_BYTES);
    plidbg_msg->w_pos = plidbg_msg->r_pos = 0;

    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1)
        {
            wait_for_completion(&msg_ready);

            lidbg("[lidbg_kernel_msg] %s\n", plidbg_msg->log[plidbg_msg->r_pos]);
            plidbg_msg->r_pos = (plidbg_msg->r_pos + 1)  % TOTAL_LOGS;

        }
        else
        {
            schedule_timeout(HZ);
        }
    }
    return 0;
}


ssize_t  msg_write(struct file *filp, const char __user *buffer, size_t size, loff_t *offset)
{

    down(&lidbg_msg_sem);
    if(copy_from_user(&(plidbg_msg->log[plidbg_msg->w_pos]), buffer, size > LOG_BYTES ? LOG_BYTES : size))
    {
        lidbg("copy_from_user ERR\n");
    }

    //for safe
    plidbg_msg->log[plidbg_msg->w_pos][(size > LOG_BYTES ? LOG_BYTES : size) - 1] = '\0';

    plidbg_msg->w_pos = (plidbg_msg->w_pos + 1)  % TOTAL_LOGS;

    up(&lidbg_msg_sem);

    complete(&msg_ready);

    return size;
}


int msg_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int msg_release(struct inode *inode, struct file *filp)
{
    return 0;
}


static struct file_operations dev_fops =
{
    .owner	=	THIS_MODULE,
    .open   =   msg_open,
    .write  =   msg_write,
    .release =  msg_release,
};

static struct miscdevice misc =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,

};
static int __init msg_init(void)
{
    int ret;
    struct task_struct *task;

    DUMP_BUILD_TIME;
    ret = misc_register(&misc);

    task = kthread_create(thread_msg, NULL, "thread_dbg_msg");
    if(IS_ERR(task))
    {
        lidbg("Unable to start thread.\n");
    }
    else wake_up_process(task);


    return ret;
}

static void __exit msg_exit(void)
{
    misc_deregister(&misc);
    lidbg (DEVICE_NAME"msg  dev_exit\n");
}

module_init(msg_init);
module_exit(msg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");


