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
    {
        //msm8909 v2
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = 8,

        .gpio_usb_power = 11,
        .gpio_usb_id = 32,
        .gpio_usb_switch = -1,
        .gpio_usb_udisk_en = 3,
        .gpio_usb_front_en = 17,
        .gpio_usb_backcam_en = 28,
        //.gpio_int_gps = 96,
	 .gpio_gps_en = 2,
        .gpio_int_button_left1 = 25,
        .gpio_int_button_left2 = 26,
        .gpio_int_button_right1 = 23,
        .gpio_int_button_right2 = 24,

        .gpio_led1 = 0,

        .gpio_int_mcu_i2c_request = 94,//I2C_C  P1-15	lpc-->qcom:request i2c communication,wakeup qcom trigger like this,111111111000111111111
        .gpio_mcu_i2c_wakeup = 95,//I2C_C2     P2-3 		qcom-->lpc:wakeup lpc  ,trigger like this ,00000000011100000000

        .gpio_mcu_wp = 33,//LPC_MSM1     P0-17		qcom-->lpc:cpu alive=0,else 1
        .gpio_mcu_app = 34,//LPC_MSM2     P1-25		qcom-->lpc:hal alive=0,else 1
	.gpio_request_fastboot = 35,//LPC_MSM3   P1-23	lpc-->qcom:acc status , on=1,off=0
	.gpio_ready = 36,//LPC_MSM4       P0_15      qcom-->lpc:gpio ready=1,else 0

        .gpio_ts_int = 13,
        .gpio_ts_rst = 12,

        .gpio_dvd_tx = 20,
        .gpio_dvd_rx = 21,

        .gpio_bt_tx = 4,
        .gpio_bt_rx = 5,
	.gpio_accel_int1 = 65,

		//.gpio_back_det = 35,

	.i2c_bus_accel = 5,
        .i2c_bus_dsi83 = 1,
        .i2c_bus_ts = 4,
        .i2c_bus_gps = 5,
        .i2c_bus_tef6638 = 5,
        .i2c_bus_lpc = 3,
        .i2c_bus_pca9634 = 4,
        .ad_val_mcu = 1,
        .fly_parameter_node = "/dev/block/bootdevice/by-name/flyparameter",

        .thermal_ctrl_en = 1,
        // msm8909.dtsi  qcom,sensor-information
        .cpu_sensor_num = 3,
        .mem_sensor_num = 0,

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

