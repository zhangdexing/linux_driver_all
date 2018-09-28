#include "fly_target.h"

struct bootloader_hw_config g_hw_info[] =
{
    {
        .adc_info[0] = {
            .ad_vol = 3200,
        },
        .ctp_info = {
            .ctp_int = 0x010f,//GPIO1_B7
            .ctp_rst = 0x000e,//GPIO0_B6
            .ctp_sda = 0x011c,//GPIO1_D4
            .ctp_scl = 0x011d,//GPIO1_D5
        },

        .lpc_info = {
            .lpc_slave_add = 0x50,
            .lpc_sda = 0x0118,//GPIO1_D0
            .lpc_scl = 0x0119,//GPIO1_D1
        },
    }
};
