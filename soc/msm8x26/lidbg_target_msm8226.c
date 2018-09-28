//dts

struct hw_version_specific g_hw_version_specific[] =
{

    {
        //msm8226 v1 ,run in 25Q baseboard
        .gpio_lcd_reset = 25,
        .gpio_t123_reset = 28,
        .gpio_dsi83_en = 62,

        .gpio_usb_id = -1,
        .gpio_usb_power = 109,
        .gpio_usb_switch = -1,

        .gpio_int_gps = 50,

        .gpio_int_button_left1 = 31,
        .gpio_int_button_left2 = 32,
        .gpio_int_button_right1 = 33,
        .gpio_int_button_right2 = 34,

        .gpio_led1 = 60,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = 108,
        .gpio_mcu_wp = 35,
        .gpio_mcu_app = 36,

        .gpio_ts_int = 69,
        .gpio_ts_rst = 24,

	.gpio_back_det = 56,

        .i2c_bus_dsi83 = 2,
        .i2c_bus_bx5b3a = 2,
        .i2c_bus_ts = 5,
        .i2c_bus_gps = 5,
        .i2c_bus_saf7741 = 5,
        .i2c_bus_tef6638 = -1,
        .i2c_bus_lpc = 0,
        .i2c_bus_pca9634 = 5,
        .ad_val_mcu = 0,
        .ad_key =
        {
            {
                .ch = 35,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_HOME},
                }
            },

            {
                .ch = 37,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2000, KEY_MENU},
                    {2500, KEY_BACK},
                }
            },
        },


        .thermal_ctrl_en = 1,
        .cpu_freq_thermal =
        {
            {1,  80,  1401600, "1401600", "450000000"},
            {81, 85,  1344000, "1344000", "450000000"},
            {86, 90,  1190400, "1190400", "450000000"},
            {91, 95,  998400, "998400", "450000000"},
            {96, 100, 787200, "787200", "450000000"},
            {101, 105, 600000, "600000", "320000000"},
            {106, 500, 300000, "300000", "200000000"},
            {0, 0, 0, "0"}, //end flag
        },

        .cpu_freq_list = "300000,384000,600000,787200,998400,1094400,1190400,1305600,1344000,1401600",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone5/temp",
        .gpu_max_freq_node = "/sys/class/kgsl/kgsl-3d0/max_gpuclk",
        .cpu_freq_recovery_limit = "600000",
        .cpu_sensor_num = 5,
        .mem_sensor_num = 3,

        .fly_parameter_node = "/dev/block/platform/msm_sdcc.1/by-name/flyparameter",
        .system_switch_en = 1,

    },
    {
        //msm8226 v2
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 62,

        .gpio_usb_id = 23,
        .gpio_usb_power = 28,
        .gpio_usb_switch = 109,

        .gpio_int_gps = 50,

        .gpio_int_button_left1 = 31,
        .gpio_int_button_left2 = 32,
        .gpio_int_button_right1 = 33,
        .gpio_int_button_right2 = 34,

        .gpio_led1 = 60,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = 108,
        .gpio_mcu_wp = 35,
        .gpio_mcu_app = 36,

        .gpio_ts_int = 69,
        .gpio_ts_rst = 24,

		.gpio_back_det = 56,

        .i2c_bus_dsi83 = 2,
        .i2c_bus_bx5b3a = 2,
        .i2c_bus_ts = 5,
        .i2c_bus_gps = 5,
        .i2c_bus_saf7741 = 5,
        .i2c_bus_lpc = 0,
        .i2c_bus_pca9634 = 5,
        .ad_val_mcu = 0,

        .ad_key =
        {
            {
                .ch = 35,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_HOME},
                }
            },

            {
                .ch = 37,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2000, KEY_MENU},
                    {2500, KEY_BACK},
                }
            },
        },

        .thermal_ctrl_en = 1,
        .cpu_freq_thermal =
        {
            {1,  80,  1401600, "1401600", "450000000"},
            {81, 85,  1344000, "1344000", "450000000"},
            {86, 90,  1190400, "1190400", "450000000"},
            {91, 95,  998400, "998400", "450000000"},
            {96, 100, 787200, "787200", "450000000"},
            {101, 105, 600000, "600000", "320000000"},
            {106, 500, 300000, "300000", "200000000"},
            {0, 0, 0, "0"}, //end flag
        },


        .cpu_freq_list = "300000,384000,600000,787200,998400,1094400,1190400,1305600,1344000,1401600",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone5/temp",
        .gpu_max_freq_node = "/sys/class/kgsl/kgsl-3d0/max_gpuclk",
        .cpu_freq_recovery_limit = "600000",
        .cpu_sensor_num = 5,
        .mem_sensor_num = 3,
        .fly_parameter_node = "/dev/block/platform/msm_sdcc.1/by-name/flyparameter",

        .system_switch_en = 1,
    },
    {
        //msm8226 v3 1.6G
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 62,

        .gpio_usb_id = 23,
        .gpio_usb_power = 28,
        .gpio_usb_switch = 109,

        .gpio_int_gps = 50,

        .gpio_int_button_left1 = 31,
        .gpio_int_button_left2 = 32,
        .gpio_int_button_right1 = 33,
        .gpio_int_button_right2 = 34,

        .gpio_led1 = 60,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = 108,
        .gpio_mcu_wp = 35,
        .gpio_mcu_app = 36,
        .gpio_request_fastboot = 56,

        .gpio_ts_int = 69,
        .gpio_ts_rst = 24,

        .gpio_dvd_tx = 12,
        .gpio_dvd_rx = 13,

        .gpio_bt_tx = 20,
        .gpio_bt_rx = 21,
		.gpio_accel_int1 = 64,

		.gpio_back_det = 56,

		.i2c_bus_accel = 2,
        .i2c_bus_dsi83 = 2,
        .i2c_bus_bx5b3a = 2,
        .i2c_bus_ts = 5,
        .i2c_bus_gps = 5,
        .i2c_bus_saf7741 = 5,
        .i2c_bus_lpc = 0,
        .i2c_bus_pca9634 = 5,

        .ad_val_mcu = 0,

        .ad_key =
        {
            {
                .ch = 35,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_HOME},
                }
            },

            {
                .ch = 37,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2000, KEY_MENU},
                    {2500, KEY_BACK},
                }
            },
        },

        .ad_key_map = {
            [TS_NO_KEY] = { 0, 3300 },
            [TS_KEY_POWER] =	{37, 2800},
            [TS_KEY_BACK] = { 35, 2500 },
            [TS_KEY_HOME] = { 37, 2500 },
            [TS_KEY_VOLUMEDOWN] = {37, 200},
            [TS_KEY_VOLUMEUP] = {35, 2800},
            [TS_KEY_NAVI] = {37, 1000},
            [TS_KEY_SEEKUP] = {37, 1500},
            [TS_KEY_SEEKDOWN] = {37, 2000},
        },

        .thermal_ctrl_en = 1,
#if 0
        .cpu_freq_thermal =
        {

            {1,  75,  1593600, "1593600", "450000000"},
            {76, 80,  1401600, "1401600", "450000000"},
            {81, 85,  1305600, "1305600", "450000000"},
            {86, 87,  998400, "998400", "450000000"},
            {88, 90, 787200, "787200", "450000000"},
            {91, 100, 600000, "600000", "320000000"},
            {101, 500, 300000, "300000", "200000000"},
            {0, 0, 0, "0"}, //end flag
        },
#else
        .cpu_freq_thermal =
        {
            { -500,  80,  1593600, "1593600", "450000000"},
            {81, 83,  1401600, "1401600", "450000000"},
            {84, 86,  1305600, "1305600", "450000000"},
            {87, 89,  998400, "998400", "450000000"},
            {90, 92, 787200, "787200", "450000000"},
            {93, 123, 600000, "600000", "320000000"},
            {124, 125, 384000, "384000", "200000000"},
            {126, 500, 300000, "300000", "200000000"},
            {0, 0, 0, "0"}, //end flag
        },// env - 75 ; mem -  101; cpu - 105 ;; env - 70; mem -  96; cpu - 101

#endif
        .cpu_freq_list = "300000,384000,600000,787200,998400,1094400,1190400,1305600,1344000,1401600,1497600,1593600",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone5/temp",
        .gpu_max_freq_node = "/sys/class/kgsl/kgsl-3d0/max_gpuclk",
        .cpu_freq_recovery_limit = "600000",
        .cpu_sensor_num = 5,
        .mem_sensor_num = 3,
        .fan_onoff_temp = 90,

        .fly_parameter_node = "/dev/block/platform/msm_sdcc.1/by-name/flyparameter",

        .system_switch_en = 0,
    },

    {
        //msm8926 v4 1.6G
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 62,

        .gpio_usb_id = 23,
        .gpio_usb_power = 28,
        .gpio_usb_switch = 109,

        .gpio_int_gps = 50,

        .gpio_int_button_left1 = 31,
        .gpio_int_button_left2 = 32,
        .gpio_int_button_right1 = 33,
        .gpio_int_button_right2 = 34,

        .gpio_led1 = 60,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = 108,
        .gpio_mcu_wp = 35,
        .gpio_mcu_app = 36,

        .gpio_ts_int = 69,
        .gpio_ts_rst = 24,

        .gpio_dvd_tx = 12,
        .gpio_dvd_rx = 13,

        .gpio_bt_tx = 20,
        .gpio_bt_rx = 21,

		.gpio_back_det = 56,

        .i2c_bus_dsi83 = 2,
        .i2c_bus_bx5b3a = 2,
        .i2c_bus_ts = 5,
        .i2c_bus_gps = 5,
        .i2c_bus_saf7741 = 5,
        .i2c_bus_lpc = 0,
        .i2c_bus_pca9634 = 5,

        .ad_key =
        {
            {
                .ch = 35,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2500, KEY_HOME},
                }
            },

            {
                .ch = 37,
                .offset = 100,
                .max = 3300,
                .key_item = {
                    {2000, KEY_MENU},
                    {2500, KEY_BACK},
                }
            },
        },

        .ad_key_map = {
            [TS_NO_KEY] = { 0, 3300 },
            [TS_KEY_POWER] =	{37, 2800},
            [TS_KEY_BACK] = { 35, 2500 },
            [TS_KEY_HOME] = { 37, 2500 },
            [TS_KEY_VOLUMEDOWN] = {37, 200},
            [TS_KEY_VOLUMEUP] = {35, 2800},
            [TS_KEY_NAVI] = {37, 1000},
            [TS_KEY_SEEKUP] = {37, 1500},
            [TS_KEY_SEEKDOWN] = {37, 2000},
        },

        .thermal_ctrl_en = 1,
        .cpu_freq_thermal =
        {

            { -500,  80,  1593600, "1593600", "450000000"},
            {81, 83,  1401600, "1401600", "450000000"},
            {84, 86,  1305600, "1305600", "450000000"},
            {87, 89,  998400, "998400", "450000000"},
            {90, 92, 787200, "787200", "450000000"},
            {93, 123, 600000, "600000", "320000000"},
            {124, 125, 384000, "384000", "200000000"},
            {126, 500, 300000, "300000", "200000000"},
            {0, 0, 0, "0"}, //end flag
        },// env - 75 ; mem -101; cpu - 105 ;; env - 70; mem -  96; cpu - 101


        .cpu_freq_list = "300000,384000,600000,787200,998400,1094400,1190400,1305600,1344000,1401600,1497600,1593600",
        .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone5/temp",
        .gpu_max_freq_node = "/sys/class/kgsl/kgsl-3d0/max_gpuclk",
        .cpu_freq_recovery_limit = "600000",
        .cpu_sensor_num = 5,
        .mem_sensor_num = 3,
        .fan_onoff_temp = 90,

        .fly_parameter_node = "/dev/block/platform/msm_sdcc.1/by-name/flyparameter",

        .system_switch_en = 0,
    },

};

