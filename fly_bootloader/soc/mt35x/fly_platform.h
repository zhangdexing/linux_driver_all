#ifndef __FLYPLATFORM_H__
#define __FLYPLATFORM_H__

#include <app.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <arch/arm.h>
#include <arch/ops.h>
#include <arch/arm/mmu.h>
#include <bits.h>
#include <debug.h>
#include <dev/keys.h>
#include <dev/udc.h>
#include <dev/lcdc.h>
#include <dev/gpio.h>
#include <dev/ssbi.h>
#include <dev/flash.h>
#include <dev/gpio_keypad.h>
#include <lib/console.h>
#include <lib/fs.h>
#include <lib/ptable.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <reg.h>
#include <target.h>
#include <video_fb.h>
#include <platform/mt_disp_drv.h>
#include <platform/partition.h>
#include <platform/msdc_utils.h>
#include <platform/boot_mode.h>

#define BOARD_VERSION 0
#define ADC_KEY_CHNL 2
#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

/* logo format */
#define RGB565 1
#define RGB888 2
#define ARGB8888 3
#define LOGO_FORMAT ARGB8888
#define DISPLAY_TYPE_MIPI
#define INVALID_PTN (-1)
#define BOOTLOADER_MT3561
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define flush_memery mt_disp_update(0, 0, FBCON_WIDTH, FBCON_HEIGHT);
extern int mboot_android_load_recoveryimg_hdr(char *part_name, unsigned long addr);
extern int mboot_android_load_recoveryimg(char *part_name, unsigned long addr);
extern void *mt_get_fb_addr(void);
extern BOOTMODE g_boot_mode;
#endif

