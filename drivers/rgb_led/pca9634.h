#ifndef __PCA9634_H__
#define __PCA9634_H__

#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/syscalls.h>
//#include <asm/system.h>

#include <linux/workqueue.h>

#if 1
#define BYTE u8
#define UINT u32
#define UINT32 u32
#define BOOL bool
#define ULONG u32

#define FALSE 0
#define TRUE 1
#endif


#define I2C_API_XFER_MODE_SEND 1
#define I2C_API_XFER_MODE_RECV 2
#define I2C_API_XFER_MODE_RECV_SUBADDR_2BYTES 3

#define 	PCA9634_GPIO_RST          (LPC)  //no use

#define 	PCA9634_I2C_ADDR	      0x70	//All call address(individual 0x15 not work)
#define	PCA9634_RST_I2C_ADDR	0x03 //Software Reset address

#endif  //__PCA9634_H__

