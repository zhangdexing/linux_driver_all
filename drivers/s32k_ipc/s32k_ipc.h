#ifndef _S32K_IPC_H_
#define _S32K_IPC_H_

#include <linux/ioctl.h>

/*Macros to help debuging*/  
#undef PDEBUG  
#ifdef IPC_DEBUG
    #ifdef __KERNEL__  
        #define PDEBUG(fmt, args...) printk(KERN_DEBUG "IPC:" fmt,## args)   
    #else  
        #define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)  
    #endif  
#else  
#define PDEBUG(fmt, args...)
#endif  
  
#define IPC_MAJOR 520 
#define IPC_MINOR 0  
#define COMMAND1 1  
#define COMMAND2 2  
  
struct ipc_dev {  
    struct cdev cdev;  
};  

/*mem设备描述结构体*/
struct mem_dev                                     
{                                                        
  char *data;                      
  unsigned long size;       
};
  
ssize_t ipc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);  
ssize_t ipc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);  
//loff_t ipc_llseek(struct file *filp, loff_t off, int whence);  
//long ipc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);  
  
#endif  
