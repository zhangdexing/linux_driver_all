//like dts

struct hw_version_specific g_hw_version_specific[] =
{
    {
        //px3 v1
        .gpio_led1 = RK30_PIN0_PB4,
        .gpio_ts_int = RK30_PIN0_PA7,
        .gpio_ts_rst = RK30_PIN0_PA6,
        .i2c_bus_ts = 2,
        .i2c_bus_tef6638 = -1,
        .fly_parameter_node = "/dev/block/platform/emmc/by-name/flyparameter",
        .lpc_disable = 1,
        .ad_val_mcu = 1,
    },

    {
        //px3 v2
        .gpio_led1 = RK30_PIN2_PD7,
        .gpio_ts_int = RK30_PIN1_PB7,
        .gpio_ts_rst = RK30_PIN0_PB6,
        .gpio_wifi_pwr = RK30_PIN3_PA0,
        .gpio_gps_rst = RK30_PIN1_PA3,
        .i2c_bus_ts = 2,
        .i2c_bus_tef6638 = 3,
        .i2c_bus_pca9634 = 2,
        .fly_parameter_node = "/dev/block/platform/emmc/by-name/flyparameter",
        .ad_val_mcu = 1,
        .gpio_int_mcu_i2c_request = RK30_PIN0_PB7 ,
        .i2c_bus_lpc = 0,
        .gpio_mcu_wp = RK30_PIN0_PA2,
        .gpio_mcu_app = RK30_PIN0_PB1,
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
        .gpio_int_button_left1 = RK30_PIN0_PA6,
        .gpio_int_button_left2 = RK30_PIN0_PA5,
        .gpio_int_button_right1 = RK30_PIN0_PA1,
        .gpio_int_button_right2 = RK30_PIN0_PA0,
        .is_single_edge = 1,
    },
};

