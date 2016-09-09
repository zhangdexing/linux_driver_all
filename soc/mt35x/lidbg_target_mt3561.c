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
        .gpio_usb_front_en = 17,
        .gpio_usb_backcam_en = 20,

        .gpio_int_gps = 87,

        .gpio_int_button_left1 = 87,
        .gpio_int_button_left2 = 87,
        .gpio_int_button_right1 = 87,
        .gpio_int_button_right2 = 87,

        .gpio_led1 = 87,
        .gpio_led2 = 87,

        .gpio_int_mcu_i2c_request = 6,
         .gpio_mcu_i2c_wakeup = 87,
        .gpio_mcu_wp = 87,
        .gpio_mcu_app = 87,
	.gpio_request_fastboot = 87,

        .gpio_ts_int = 10,
        .gpio_ts_rst = 146,

        .gpio_dvd_tx = 87,
        .gpio_dvd_rx = 87,

        .gpio_bt_tx = 87,
        .gpio_bt_rx = 87,
		.gpio_accel_int1 = 87,

		.gpio_back_det = 87,

		.i2c_bus_accel = 0,
        .i2c_bus_dsi83 = 0,
        .i2c_bus_ts = 1,
        .i2c_bus_gps = 0,
        .i2c_bus_tef6638 = 0,
        .i2c_bus_lpc = 3,
        .i2c_bus_pca9634 = 0,
        .ad_val_mcu = 87,
        .fly_parameter_node = "/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/flyparameter",

	},

};

