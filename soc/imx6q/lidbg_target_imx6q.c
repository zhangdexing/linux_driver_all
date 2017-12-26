//like dts

struct hw_version_specific g_hw_version_specific[] =
{

    {
        //imx6q v1
        //.gpio_lcd_reset = 0,
        //.gpio_t123_reset = 0,
        //.gpio_dsi83_en = 0,

        //.gpio_usb_id = 0,
        //.gpio_usb_power = 0,
        //.gpio_usb_switch = 0,

        .gpio_int_gps = 1,

        .gpio_int_button_left1 = 175,
        .gpio_int_button_left2 = 176,
        .gpio_int_button_right1 = 171,
        .gpio_int_button_right2 = 174,

        .gpio_led1 = 29,
        //gpio_led2 = 0,

        .gpio_int_mcu_i2c_request = 204, //	i2c_c
	.gpio_mcu_read_busy = 79,	// ap_state6
	.gpio_mcu_i2c_wakeup = 105,	//	AUD5_RXD
        .gpio_mcu_wp = 9,	// ap_state2
        .gpio_mcu_app = 8,	// ap_state1
	.gpio_request_fastboot= 106,	// ap_state3 ok

        .gpio_ts_int = 110,
        .gpio_ts_rst = 111,


        //.i2c_bus_dsi83 = 0,
        .i2c_bus_bx5b3a = 0,
        .i2c_bus_ts = 0,
        .i2c_bus_gps = 0,
        //.i2c_bus_saf7741 = 0,
        .i2c_bus_tef6638 = 0,
        .i2c_bus_lpc = 2,

        .i2c_bus_fm1388=0,
	    .spi_bus_fm1388=1,
	    .spi_bus_ymu836=0,
        .ad_val_mcu = 1,
        //.thermal_ctrl_en = 0,
        .cpu_freq_thermal =
        {
            {0, 0, 0, "0"}, //end flag
        },

        .fly_parameter_node = "/dev/block/mmcblk3p13",
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

