#ifndef _FLYBOOTLOADER_SOC_
#define _FLYBOOTLOADER_SOC_

#include "fly_platform.h"

/*gpio operatios*/
void gpio_set_direction(int pin, int dir);
void gpio_set_val(int pin, int val);
int gpio_get_val(int pin);

/*adc get value*/
int adc_get(void);

/*get fb base addr*/
void *fb_base_get(void);

/*ptn operations*/
int ptn_read(char *ptn_name, unsigned int offset, unsigned long len, unsigned char *buf);

/*hardware init*/
void flyaudio_hw_init(void);

/* flyrecovery */
int boot_flyrecovery_from_mmc(void);

/* dbg msg enable or disable*/
char *dbg_msg_en(const char *system_cmd, int dbg_msg_en);

/* call before exit bootloader */
void bootloader_exit_func(void);

#endif
