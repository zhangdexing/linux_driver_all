//dts

struct hw_version_specific g_hw_version_specific[] =
{

    {
        //msm8974 v1
        .gpio_lcd_reset = 25,
        .gpio_t123_reset = 28,
        .gpio_dsi83_en = 58,

        .gpio_usb_id = -1,
        .gpio_usb_power = 109,
        .gpio_usb_switch = -1,

        .gpio_int_gps = 50,

        .gpio_int_button_left1 = 54,
        .gpio_int_button_left2 = 53,
        .gpio_int_button_right1 = 56,
        .gpio_int_button_right2 = 55,

        .gpio_led1 = 60,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = 108,
        .gpio_mcu_wp = 35,
        .gpio_mcu_app = 36,

        .gpio_ts_int = 14,
        .gpio_ts_rst = 13,


        .i2c_bus_dsi83 = 3,
        .i2c_bus_bx5b3a = 3,
        .i2c_bus_ts = 2,
        .i2c_bus_gps = 2,
        .i2c_bus_saf7741 = 2,
        .i2c_bus_tef6638 = 2,
        .i2c_bus_lpc = 0,
        .i2c_bus_pca9634 = 2,
        .ad_val_mcu = 0,
        .ad_key =
        {
            {
                .ch = 39,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_HOME},
                }
            },

            {
                .ch = 38,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2000, KEY_MENU},
                    {2500, KEY_BACK},
                }
            },
        },

        .thermal_ctrl_en = 0,
        .cpu_freq_thermal =
        {
            {1,  70,  2265600, "2265600"},
            {70, 80,  1728000, "1728000"},
            {81, 90,  1267200, "1267200"},
            {91, 100, 1036800, "1036800"},
            {101, 150, 600000, "600000"},
            {151, 500, 300000, "300000"},
            {0, 0, 0, "0"}, //end flag
        },

        .cpu_freq_list = "300000,422400,652800,729600,883200,960000,1036800,1190400,1267200,1497600,1574400,1728000,1958400,2265600",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone5/temp",
        .cpu_freq_recovery_limit = "600000",
        .cpu_sensor_num = 5,
        .mem_sensor_num = 3,
        .fly_parameter_node = "/dev/block/platform/msm_sdcc.1/by-name/flyparameter",
    },
    {
        //msm8974 v2
        .gpio_lcd_reset = 80,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 58,

        .gpio_usb_id = 73,
        .gpio_usb_power = 78,
        .gpio_usb_switch = -1,

        .gpio_int_gps = 50,

        .gpio_int_button_left1 = 54,
        .gpio_int_button_left2 = 53,
        .gpio_int_button_right1 = 56,
        .gpio_int_button_right2 = 55,

        .gpio_led1 = 75,
        .gpio_led2 = 81,

        .gpio_int_mcu_i2c_request = 76,
        .gpio_mcu_wp = 79,
        .gpio_mcu_app = 68,
	.gpio_request_fastboot = 69,
        .gpio_ts_int = 14,
        .gpio_ts_rst = 12,

        .gpio_dvd_tx = 8,
        .gpio_dvd_rx = 9,

        .gpio_bt_tx = 0,
        .gpio_bt_rx = 1,
		.gpio_accel_int1 = 66,

		.i2c_bus_accel = 3,
        .i2c_bus_dsi83 = 3,
        .i2c_bus_bx5b3a = 3,
        .i2c_bus_ts = 2,
        .i2c_bus_gps = 2,
        .i2c_bus_saf7741 = 2,
        .i2c_bus_tef6638 = 2,
        .i2c_bus_lpc = 0,
        .i2c_bus_pca9634 = 2,
        .ad_val_mcu = 0,

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
                    {543, KEY_BACK} ,
                }
            },

            {
                .ch = 38,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2000, KEY_MENU},
                    {2500, KEY_BACK},
                }
            },
        },


        .thermal_ctrl_en = 0,
        .cpu_freq_thermal = //for recovery
        {
            // {1,  70,  2265600,"2265600"},
            // {70, 80,  1728000,"1728000"},
            // {81, 90,  1497600,"1497600"},
            { -500, 50, 1190400, "1190400"},
            {51, 60, 729600, "729600"},
            {61, 70, 652800, "652800"},
            {71, 80, 422400, "422400"},
            {81, 150, 300000, "300000"},
            {0, 0, 0, "0"}, //end flag
        },

        .cpu_freq_list = "300000,422400,652800,729600,883200,960000,1036800,1190400,1267200,1497600,1574400,1728000,1958400,2265600",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone5/temp",
        .cpu_freq_recovery_limit = "600000",
        .cpu_sensor_num = 5,
        .mem_sensor_num = 3,
        .fan_onoff_temp = 75,

        .fly_parameter_node = "/dev/block/platform/msm_sdcc.1/by-name/flyparameter",
    },

};

