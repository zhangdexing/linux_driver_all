#include "fly_target.h"

struct bootloader_hw_config g_hw_info[] =
{
    {
        .display_info = {
            .dsi83_slave_add = 0x2d,
            .dsi83_en_pin = 58,
            .dsi83_sda = 29,
            .dsi83_scl = 30,
        },

	.adc_info[0] = {
	   .ad_ch = -1,
	   .ad_ctrl_ch = -1,
	   .ad_vol = 3200,
         },

	.adc_info[ADC_KEY_CHNL - 1] = {
	  .ad_ch = -1,
	  .ad_ctrl_ch = -1,
	  .ad_vol = 0,
	},

        .ctp_info = {
            .ctp_int = 10,
            .ctp_rst = 146,
            .ctp_sda = 49,
            .ctp_scl = 50,
        },

        .lpc_info = {
            .lpc_slave_add = 0x50,
            .lpc_sda = 53,
            .lpc_scl = 54,
        },
	.lk_wakeup_lpc_io = 95,
	.gpio_mcu_wp = 9,
	.gpio_ready = 36,
	.gpio_hal_ready = 34,
        .dbg_uart_port = "console=ttyHSL0,115200,n8 androidboot.console=ttyHSL0 ",
    }
};
