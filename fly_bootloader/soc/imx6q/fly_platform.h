#ifndef __FLYPLATFORM_H__
#define __FLYPLATFORM_H__

#include <common.h>
#include <stdarg.h>
#include <bedbug/bedbug.h>
//#include <armlinux/config.h>
//#include "rkloader.h"	//flash read/write

#define BOOTLOADER_IMX6Q
//#define NEW_SUSPEND

#define TRUE        1
#define FALSE       0
#define BOARD_VERSION 0
#define ADC_KEY_CHNL 1

DECLARE_GLOBAL_DATA_PTR;

/* GPIO TLMM: Direction */
#ifndef GPIO_INPUT
#define GPIO_INPUT      0
#endif
#ifndef GPIO_OUTPUT
#define GPIO_OUTPUT     1
#endif

/* debug levels */
#define CRITICAL 0
#define ALWAYS 0
#define INFO 1
#define SPEW 2

#define INVALID_PTN -1

#define dprintf(level, x...) printf(x)

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

/* logo format */
#define RGB565 1
#define RGB888 2
#define LOGO_FORMAT RGB565


extern struct fbcon_config *config;
extern unsigned page_mask;
#define flush_memery(x,y) flush_cache(0x8ef20f60,x*y*3);
#endif

