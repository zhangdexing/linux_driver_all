#include "../soc.h"
#include <platform/mt_gpio.h>
#include "fly_target.h"
void bootloader_exit_func(void)
{
	mt_set_gpio_mode_chip(g_bootloader_hw.ctp_info.ctp_sda, GPIO_MODE_DEFAULT);
	mt_set_gpio_mode_chip(g_bootloader_hw.ctp_info.ctp_scl, GPIO_MODE_DEFAULT);
	mt_set_gpio_mode_chip(g_bootloader_hw.lpc_info.lpc_sda, GPIO_MODE_DEFAULT);
	mt_set_gpio_mode_chip(g_bootloader_hw.lpc_info.lpc_scl, GPIO_MODE_DEFAULT);
}