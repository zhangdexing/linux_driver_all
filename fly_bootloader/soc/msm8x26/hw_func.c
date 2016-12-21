#include "../soc.h"
#include "fly_target.h"

extern void dsi83_init();

void flyaudio_hw_init(void)
{
	dprintf(INFO, "Flyaudio hardware init. \n");
#ifdef BOOTLOADER_MSM8996
	gpio_set_direction(g_bootloader_hw.gpio_level_conversion_en, GPIO_OUTPUT);
	gpio_set_val(g_bootloader_hw.gpio_level_conversion_en, 1);
#endif
	gpio_set_direction(g_bootloader_hw.gpio_mcu_wp, GPIO_OUTPUT);
	gpio_set_val(g_bootloader_hw.gpio_mcu_wp, 0);//set to alive

	gpio_set_direction(g_bootloader_hw.gpio_hal_ready, GPIO_OUTPUT);
	gpio_set_val(g_bootloader_hw.gpio_hal_ready, 1);//set to not ready

	gpio_set_direction(g_bootloader_hw.gpio_ready, GPIO_OUTPUT);
	gpio_set_val(g_bootloader_hw.gpio_ready, 1);//set to ready

#ifdef NEW_SUSPEND
	dprintf(INFO, "LK wakeup LPC, io(%d)\n", g_bootloader_hw.lk_wakeup_lpc_io);
	gpio_set_direction(g_bootloader_hw.lk_wakeup_lpc_io, GPIO_OUTPUT);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 0);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 1);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 0);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 1);
	mdelay(10);
	gpio_set_val(g_bootloader_hw.lk_wakeup_lpc_io, 0);
	mdelay(100);
#endif
	dsi83_init();
}
