//dts

struct hw_version_specific g_hw_version_specific[] =
{

    {
        //mt3360 v1
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = -1,

        .gpio_usb_id = -1,
        .gpio_usb_power = -1,
        .gpio_usb_switch = -1,

        .gpio_int_gps = -1,

        .gpio_int_button_left1 = -1,
        .gpio_int_button_left2 = -1,
        .gpio_int_button_right1 = -1,
        .gpio_int_button_right2 = -1,

        .gpio_led1 = -1,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = -1,
        .gpio_mcu_wp = 124,
        .gpio_mcu_app = 53,

        .gpio_ts_int = 37,
        .gpio_ts_rst = 55,


        .i2c_bus_dsi83 = -1,
        .i2c_bus_bx5b3a = -1,
        .i2c_bus_ts = 0,
        .i2c_bus_gps = -1,
        .i2c_bus_saf7741 = -1,
        .i2c_bus_tef6638 = -1,
        .i2c_bus_lpc = -1,
        .ad_val_mcu = 0,
        .thermal_ctrl_en = 0,
        .cpu_freq_thermal =
        {
            {0, 0, 0, "0"}, //end flag
        },

        .fly_parameter_node = "/dev/block/flyparameter",
    },
    {
        //mt3360 v2
        .gpio_lcd_reset = -1,
        .gpio_t123_reset = -1,
        .gpio_dsi83_en = -1,

        .gpio_usb_id = -1,
        .gpio_usb_power = -1,
        .gpio_usb_switch = -1,

        .gpio_int_gps = -1,

        .gpio_int_button_left1 = -1,
        .gpio_int_button_left2 = -1,
        .gpio_int_button_right1 = -1,
        .gpio_int_button_right2 = -1,

        .gpio_led1 = -1,
        .gpio_led2 = -1,

        .gpio_int_mcu_i2c_request = -1,
        .gpio_mcu_wp = -1,
        .gpio_mcu_app = -1,

        .gpio_ts_int = -1,
        .gpio_ts_rst = -1,


        .i2c_bus_dsi83 = -1,
        .i2c_bus_bx5b3a = -1,
        .i2c_bus_ts = -1,
        .i2c_bus_gps = -1,
        .i2c_bus_saf7741 = -1,
        .i2c_bus_lpc = -1,
        .ad_val_mcu = 0,

        .thermal_ctrl_en = 0,
        .cpu_freq_thermal =
        {
            {0, 0, 0, "0"}, //end flag
        },
    },

};



