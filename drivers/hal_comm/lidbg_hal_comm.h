//做成动态的读写根据type来知道数据是发给谁的；那就不存在上述两个定义；
#include <linux/kernel.h>        /* We're doing kernel work */
#include <linux/module.h>        /* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>        /* for get_user and put_user */
#include <linux/kfifo.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/cdev.h>  
#include <linux/device.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>

#ifndef _IOCOM_NONE
# define _IOCOM_NONE      0U
#endif
 
#ifndef _IOCOM_WRITE
# define _IOCOM_WRITE     1U
#endif
 
#ifndef _IOCOM_READ
# define _IOCOM_READ      2U
#endif



#define _IOCOM_DIRBITS 2
#define _IOCOM_TYPEBITS 8
#define _IOCOM_SIZEBITS	22

#define _IOCOM_TYPEMASK ((1 << _IOCOM_TYPEBITS)-1)
#define _IOCOM_DIRMASK ((1 << _IOCOM_DIRBITS)-1)
#define _IOCOM_SIZEMASK ((1 << _IOCOM_SIZEBITS)-1)


#define _IOCOM_TYPESHIFT 0
#define _IOCOM_SIZESHIFT (_IOCOM_TYPESHIFT + _IOCOM_TYPEBITS)
#define _IOCOM_DIRSHIFT (_IOCOM_SIZESHIFT + _IOCOM_SIZEBITS)

#define _IOCOM(dir,type,size) \
	(((dir) << _IOCOM_DIRSHIFT) | \
	 ((type) << _IOCOM_TYPESHIFT) | \
	 ((size) << _IOCOM_SIZESHIFT))

/* used to create numbers */
#define _IOOM(type,size)     _IOCOM(_IOCOM_NONE,(type),(size))
#define _IOROM(type,size)      _IOCOM(_IOCOM_READ,(type),(size))
#define _IOWOM(type,size)      _IOCOM(_IOCOM_WRITE,(type),(size))

/* used to decode ioctl numbers.. */
#define _IOCOM_DIR(arg)            (((arg) >> _IOCOM_DIRSHIFT) & _IOCOM_DIRMASK)
#define _IOCOM_TYPE(arg)           (((arg) >> _IOCOM_TYPESHIFT) & _IOCOM_TYPEMASK)
#define _IOCOM_SIZE(arg)           (((arg) >> _IOCOM_SIZESHIFT) & _IOCOM_SIZEMASK)

typedef struct fifo_list
{
	int mark_type;
	struct kfifo data_fifo;
	struct list_head list;
	struct semaphore fifo_sem;
	struct mutex fifo_mutex;
} threads_list;

#define HAL_FIFO_SIZE 100
#define HAL_DATA_SIZE_MAX 1016
