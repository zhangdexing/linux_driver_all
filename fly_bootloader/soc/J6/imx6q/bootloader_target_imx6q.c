#include "fly_target.h"

struct bootloader_hw_config g_hw_info[] =
{
  {
        .adc_info[0] = {
            .ad_vol = 3200,
        },
        .ctp_info = {
            .ctp_int = 60,
            .ctp_rst = 61,
            .ctp_sda = 154,
            .ctp_scl = 155,
        },

        .lpc_info = {
            .lpc_slave_add = 0x50,
            .lpc_sda = 6,
            .lpc_scl = 3,
        },
        .lk_wakeup_lpc_io = 95,
		.gpio_mcu_wp = 9,
        .gpio_ready = 36,
        .gpio_hal_ready = 34,
   }
};
