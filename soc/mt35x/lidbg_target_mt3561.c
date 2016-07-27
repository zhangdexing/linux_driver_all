//dts

struct hw_version_specific g_hw_version_specific[] =
{
	{
        //msm8909 v1
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = -1,

        .gpio_usb_power = -1,
        .gpio_usb_id = -1,
        .gpio_usb_switch = -1,

        .gpio_int_gps = -1,

        .gpio_int_button_left1 = -1,
        .gpio_int_button_left2 = -1,
        .gpio_int_button_right1 = -1,
        .gpio_int_button_right2 = -1,

        .gpio_led1 = -1,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = -1,
         .gpio_mcu_i2c_wakeup = -1,
        .gpio_mcu_wp = -1,
        .gpio_mcu_app = -1,
	.gpio_request_fastboot = -1,

        .gpio_ts_int = -1,
        .gpio_ts_rst = -1,

        .gpio_dvd_tx = -1,
        .gpio_dvd_rx = -1,

        .gpio_bt_tx = -1,
        .gpio_bt_rx = -1,
		.gpio_accel_int1 = -1,

		.gpio_back_det = -1,

		.i2c_bus_accel = 0,
        .i2c_bus_dsi83 = 0,
        .i2c_bus_ts = 0,
        .i2c_bus_gps = 0,
        .i2c_bus_tef6638 = 0,
        .i2c_bus_lpc = 0,
        .i2c_bus_pca9634 = 0,
        .ad_val_mcu = -1,
        .fly_parameter_node = "/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/flyparameter",

	},

};

