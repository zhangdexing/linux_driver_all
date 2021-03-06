//dts

struct hw_version_specific g_hw_version_specific[] =
{
	{
        //mt3561 v1
        .gpio_lcd_reset = 87,
        .gpio_t123_reset = 87,
        .gpio_dsi83_en = 87,

        .gpio_usb_power = 19,
        .gpio_usb_id = 0,
        .gpio_usb_switch = -1,
        .gpio_usb_udisk_en = 45,
        .gpio_usb_front_en = 21,
        .gpio_usb_backcam_en = 20,

        .gpio_gps_lna_en = 46,
	.gpio_gps_ant_power = 86,

        .gpio_int_button_left1 = 3,
        .gpio_int_button_left2 = 4,
        .gpio_int_button_right1 = 1,
        .gpio_int_button_right2 = 2,

        .gpio_led1 = 87,
        .gpio_led2 = 87,

        .gpio_int_mcu_i2c_request = 6,
        .gpio_mcu_i2c_wakeup = 7,
        .gpio_mcu_wp = 55,  //ap_state1
        .gpio_mcu_app = 9, //ap_state2
	.gpio_request_fastboot = 82, //ap_state3

        .gpio_ts_int = 10,
        .gpio_ts_rst = 146,

        .gpio_dvd_tx = 87,
        .gpio_dvd_rx = 87,

        .gpio_bt_tx = 87,
        .gpio_bt_rx = 87,
	.gpio_accel_int1 = 83,

	.gpio_back_det = 87,

	.i2c_bus_accel = 0,
        .i2c_bus_dsi83 = 0,
        .i2c_bus_ts = 1,
	.i2c_bus_ub9xx = 1,
        .i2c_bus_gps = 0,
        .i2c_bus_tef6638 = 0,
        .i2c_bus_lpc = 3,
        .i2c_bus_pca9634 = 0,
        .ad_val_mcu = 87,
        .fly_parameter_node = "/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/flyparameter",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone1/temp",
	.ad_val_mcu = 1,
	.ad_key =
        {
            {
                .ch = 39,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_HOME},
                    {1535, KEY_VOLUMEUP} ,
                    {2015, KEY_VOLUMEDOWN} ,

                }
            },

            {
                .ch = 38,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_BACK},
                }
            },
            {
                .ch = 37,
                .offset = 100,
                .max = 3300,
                .key_item = {
                },
            },

            {
                .ch = 36,
                .offset = 100,
                .max = 3300,
                .key_item = {
                },
            },
      },

	},
	{
        //mt3561 v2
        .gpio_lcd_reset = 87,
        .gpio_t123_reset = 87,
        .gpio_dsi83_en = 87,

        .gpio_usb_power = 19,
        .gpio_usb_id = 0,
        .gpio_usb_switch = -1,
        .gpio_usb_udisk_en = 45,
        .gpio_usb_front_en = 21,
        .gpio_usb_backcam_en = 20,

        .gpio_gps_lna_en = 46,
	.gpio_gps_ant_power = 86,

        .gpio_int_button_left1 = 3,
        .gpio_int_button_left2 = 4,
        .gpio_int_button_right1 = 1,
        .gpio_int_button_right2 = 2,

        .gpio_led1 = 42,
        .gpio_led2 = 87,

        .gpio_int_mcu_i2c_request = 6,
        .gpio_mcu_i2c_wakeup = 7,
        .gpio_mcu_wp = 55,  //ap_state1
        .gpio_mcu_app = 9, //ap_state2
	.gpio_request_fastboot = 82, //ap_state3
	.gpio_ready = 85,//LPC_MSM4       P0_15      qcom-->lpc:gpio ready=1,else 0

        .gpio_ts_int = 10,
        .gpio_ts_rst = 146,

        .gpio_dvd_tx = 87,
        .gpio_dvd_rx = 87,

        .gpio_bt_tx = 87,
        .gpio_bt_rx = 87,
	.gpio_accel_int1 = 83,

	.gpio_back_det = 87,

	.i2c_bus_accel = 0,
        .i2c_bus_dsi83 = 0,
        .i2c_bus_ts = 1,
	.i2c_bus_ub9xx = 1,
        .i2c_bus_gps = 0,
        .i2c_bus_tef6638 = 0,
        .i2c_bus_lpc = 3,
        .i2c_bus_pca9634 = 0,
        .i2c_bus_fm1388 = 0,
	    .spi_bus_fm1388 = 0,
        .ad_val_mcu = 87,
        .thermal_ctrl_en = 0,
	//Mt_thermal.h (z:\home\wqrftf99\futengfei\work1_qucom\mt3561\kernel-3.18\drivers\misc\mediatek\include\mt-plat\mt3561\include\mach)	15834	8/15/2016
        .cpu_sensor_num = 0,
        .mem_sensor_num = 1,
        .fly_parameter_node = "/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/flyparameter",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone1/temp",
        .ad_val_mcu = 1,
        .ad_key =
        {
            {
                .ch = 39,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_HOME},
                    {1535, KEY_VOLUMEUP} ,
                    {2015, KEY_VOLUMEDOWN} ,

                }
            },

            {
                .ch = 38,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_BACK},
                }
            },
            {
                .ch = 37,
                .offset = 100,
                .max = 3300,
                .key_item = {
                },
            },

            {
                .ch = 36,
                .offset = 100,
                .max = 3300,
                .key_item = {
                },
            },
      },

	},
};

