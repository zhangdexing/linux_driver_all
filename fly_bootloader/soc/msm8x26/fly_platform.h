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
#include <baseband.h>
#include <boot_stats.h>
#include <blsp_qup.h>
#include <crypto_hash.h>
#include <debug.h>
#include <dev/keys.h>
#include <dev/udc.h>
#include <dev/lcdc.h>
#include <dev/gpio.h>
#include <dev/ssbi.h>
#include <dev/fbcon.h>
#include <dev/flash.h>
#include <dev/gpio_keypad.h>
#include <gsbi.h>
#include <lib/console.h>
#include <lib/fs.h>
#include <lib/ptable.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/timer.h>
#include <mmc.h>
#include <mddi.h>
#include <mmu.h>
#include <platform.h>
#include <pm8x41_adc.h>
#include <platform/irqs.h>
#include <platform/clock.h>
#include <platform/debug.h>
#include <platform/iomap.h>
#include <platform/irqs.h>
#include <platform/timer.h>
#include <partition_parser.h>
#include <qgic.h>
#include <reg.h>
#include <smem.h>
#include <target.h>
#include <uart_dm.h>

#if DEVICE_TREE
#include <libfdt.h>
#include <dev_tree.h>
#endif

#include "bootimg.h"
#include "devinfo.h"
#include "image_verify.h"
#include "recovery.h"
#include "scm.h"
#include "sparse_format.h"

#ifdef BOOTLOADER_MSM8909
#include <arch/ops.h>
#endif

#ifdef BOOTLOADER_MSM8909
#include <stdint.h>
#endif

#ifdef BOOTLOADER_MSM8226
#define BOARD_VERSION 0
#endif
#ifdef BOOTLOADER_MSM8974
#define BOARD_VERSION 1
#endif
#ifdef BOOTLOADER_MSM8909
#define BOARD_VERSION 2
#define NEW_SUSPEND 1
#endif
#ifdef BOOTLOADER_MSM8996
#define BOARD_VERSION 3
#define NEW_SUSPEND 1
#endif

#define ADC_KEY_CHNL 2

#define u8 	unsigned char
#define u16 unsigned short
#define u32	unsigned int

#define FFBM_MODE_BUF_SIZE   8

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

#ifdef MEMBASE
#define EMMC_BOOT_IMG_HEADER_ADDR (0xFF000+(MEMBASE))
#else
#define EMMC_BOOT_IMG_HEADER_ADDR 0xFF000
#endif

#define LOGO_ADD_ALLOCED 1

#if LOGO_ADD_ALLOCED == 1
#define LOGO_MALLOCED_ADDR (SCRATCH_ADDR)
#define RGB565_DATA_LEN 0x100000
#define RGB888_DATA_LEN 0x100000
#endif

/* logo format */
#define RGB565 1
#define RGB888 2
#define LOGO_FORMAT RGB888

extern struct fbcon_config *config;
extern unsigned page_mask;
extern unsigned page_size;

extern char ffbm_mode_string[FFBM_MODE_BUF_SIZE];
extern bool boot_into_ffbm;
extern device_info device;

extern int bp_meg;
#endif

