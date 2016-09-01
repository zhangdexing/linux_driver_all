//dts

struct hw_version_specific g_hw_version_specific[] =
{
    {
        //msm8909 v1
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 8,

        .gpio_usb_power = 31,
        .gpio_usb_id = 32,
        .gpio_usb_switch = -1,

        .gpio_int_gps = 96,

        .gpio_int_button_left1 = 25,
        .gpio_int_button_left2 = 26,
        .gpio_int_button_right1 = 23,
        .gpio_int_button_right2 = 24,

        .gpio_led1 = 0,
        .gpio_led2 = 3,

        .gpio_int_mcu_i2c_request = 94,
         .gpio_mcu_i2c_wakeup = 95,
        .gpio_mcu_wp = 33,
        .gpio_mcu_app = 34,
	.gpio_request_fastboot = 35,

        .gpio_ts_int = 13,
        .gpio_ts_rst = 12,

        .gpio_dvd_tx = 20,
        .gpio_dvd_rx = 21,

        .gpio_bt_tx = 4,
        .gpio_bt_rx = 5,
		.gpio_accel_int1 = 95,

		.gpio_back_det = 35,

		.i2c_bus_accel = 1,
        .i2c_bus_dsi83 = 1,
        .i2c_bus_ts = 4,
        .i2c_bus_gps = 5,
        .i2c_bus_tef6638 = 5,
        .i2c_bus_lpc = 3,
        .i2c_bus_pca9634 = 4,
        .ad_val_mcu = 1,
        .fly_parameter_node = "/dev/block/platform/7824900.sdhci/by-name/flyparameter",

        // msm8909.dtsi  qcom,sensor-information
        .cpu_sensor_num = 3,
        .mem_sensor_num = 0,
 
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

