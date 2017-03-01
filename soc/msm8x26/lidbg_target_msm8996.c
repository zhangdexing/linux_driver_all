//dts

struct hw_version_specific g_hw_version_specific[] =
{
    {
        //msm8996 v1
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 12,

        .gpio_usb_power = 13,
        .gpio_usb_id = 49,
        .gpio_usb_switch = -1,
        .gpio_usb_vbus_en = 14,
        .gpio_usb_udisk_en = 14,
        .gpio_usb_front_en = 15,
        .gpio_usb_backcam_en = 16,
        //.gpio_int_gps = 96,
	 .gpio_gps_en = -1,
        .gpio_int_button_left1 = 92,
        .gpio_int_button_left2 = 93,
        .gpio_int_button_right1 = 90,
        .gpio_int_button_right2 = 91,

        .gpio_led1 = 50,
        .gpio_int_mcu_i2c_request = 80,//I2C_C  	lpc-->qcom:request i2c communication,wakeup qcom trigger like this,111111111000111111111
        .gpio_mcu_i2c_wakeup = 79,//I2C_C2     		qcom-->lpc:wakeup lpc  ,trigger like this ,00000000011100000000

        .gpio_mcu_wp = 25,//LPC_MSM1     		qcom-->lpc:cpu alive=0,else 1
        .gpio_mcu_app = 26,//LPC_MSM2    		qcom-->lpc:hal alive=0,else 1
	.gpio_request_fastboot = 121,//LPC_MSM3  	lpc-->qcom:acc status , on=1,off=0
	.gpio_ready = 28,//LPC_MSM4                     qcom-->lpc:gpio ready=1,else 0

        .gpio_ts_int = 125,
        .gpio_ts_rst = 89,

        .gpio_dvd_tx = -1,
        .gpio_dvd_rx = -1,

        .gpio_bt_tx = 0,
        .gpio_bt_rx = 1,
	.gpio_accel_int1 = 117,
	.gpio_level_conversion_en = 10,
		//.gpio_back_det = 35,

	.i2c_bus_accel = 11,
        .i2c_bus_dsi83 = 8,
        .i2c_bus_ts = 12,//
        .i2c_bus_gps = 11,
        .i2c_bus_tef6638 = 11,
        .i2c_bus_lpc = 1,
        .i2c_bus_pca9634 = 12,//
        .ad_val_mcu = 1,
        .fly_parameter_node = "/dev/block/bootdevice/by-name/flyparameter",

        .thermal_ctrl_en = 0,
        // msm8909.dtsi  qcom,sensor-information
        .cpu_sensor_num = 3,
        .mem_sensor_num = 1,

        .cpu_freq_thermal =
        {
	    {-500,  85,  1267200, "1267200", "456000000",4},
	    {86,    89,  1094400, "1094400", "456000000",4},
	    {90,    92, 800000,  "800000",   "200000000",4},
	    {93,   500, 533333,  "533333",   "200000000",2},     
//	    {-500,   500, 533333,  "533333",   "200000000",2},    
	    {0,     0, 0, "0"} //end flag
        },
        //cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
         .cpu_freq_list = "533333,800000,998400,1094400,1190400,1248000,1267200",
         .cpu_freq_recovery_limit = "1267200",
         .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone3/temp",
         .gpu_max_freq_node = "/sys/class/kgsl/kgsl-3d0/max_gpuclk",//cat /sys/class/kgsl/kgsl-3d0/gpu_available_frequencies   456000000 307200000 200000000
        .fan_onoff_temp = 200,

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
        //msm8996 v2
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 12,

        .gpio_usb_power = 13,
        .gpio_usb_id = 49,
        .gpio_usb_switch = -1,
        .gpio_usb_vbus_en = 14,
        .gpio_usb_udisk_en = 14,
        .gpio_usb_front_en = 15,
        .gpio_usb_backcam_en = 16,
        //.gpio_int_gps = 96,
	 .gpio_gps_en = -1,
        .gpio_int_button_left1 = 107,
        .gpio_int_button_left2 = 106,
        .gpio_int_button_right1 = 90,
        .gpio_int_button_right2 = 94,

        .gpio_led1 = 50,
        .gpio_int_mcu_i2c_request = 80,//I2C_C  	lpc-->qcom:request i2c communication,wakeup qcom trigger like this,111111111000111111111
        .gpio_mcu_i2c_wakeup = 79,//I2C_C2     		qcom-->lpc:wakeup lpc  ,trigger like this ,00000000011100000000

        .gpio_mcu_wp = 25,//LPC_MSM1     		qcom-->lpc:cpu alive=0,else 1
        .gpio_mcu_app = 26,//LPC_MSM2    		qcom-->lpc:hal alive=0,else 1
	.gpio_request_fastboot = 121,//LPC_MSM3  	lpc-->qcom:acc status , on=1,off=0
	.gpio_ready = 28,//LPC_MSM4                     qcom-->lpc:gpio ready=1,else 0

        .gpio_ts_int = 125,
        .gpio_ts_rst = 89,

        .gpio_dvd_tx = -1,
        .gpio_dvd_rx = -1,

        .gpio_bt_tx = 0,
        .gpio_bt_rx = 1,
	.gpio_accel_int1 = 117,
	.gpio_level_conversion_en = 10,
		//.gpio_back_det = 35,

	.i2c_bus_accel = 7,
        .i2c_bus_dsi83 = 8,
        .i2c_bus_ts = 12,//
        .i2c_bus_gps = 11,
        .i2c_bus_tef6638 = 11,
        .i2c_bus_lpc = 1,
        .i2c_bus_pca9634 = 12,//
        .ad_val_mcu = 1,
        .fly_parameter_node = "/dev/block/bootdevice/by-name/flyparameter",

        .thermal_ctrl_en = 0,
        // msm8909.dtsi  qcom,sensor-information
        .cpu_sensor_num = 3,
        .mem_sensor_num = 1,

        .cpu_freq_thermal =
        {
	    {-500,  85,  1267200, "1267200", "456000000",4},
	    {86,    89,  1094400, "1094400", "456000000",4},
	    {90,    92, 800000,  "800000",   "200000000",4},
	    {93,   500, 533333,  "533333",   "200000000",2},     
//	    {-500,   500, 533333,  "533333",   "200000000",2},    
	    {0,     0, 0, "0"} //end flag
        },
        //cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
         .cpu_freq_list = "533333,800000,998400,1094400,1190400,1248000,1267200",
         .cpu_freq_recovery_limit = "1267200",
         .cpu_freq_temp_node = "/sys/class/thermal/thermal_zone3/temp",
         .gpu_max_freq_node = "/sys/class/kgsl/kgsl-3d0/max_gpuclk",//cat /sys/class/kgsl/kgsl-3d0/gpu_available_frequencies   456000000 307200000 200000000
        .fan_onoff_temp = 200,

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

