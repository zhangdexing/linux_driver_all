//like dts

struct hw_version_specific g_hw_version_specific[] =
{

    {
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


        .i2c_bus_dsi83 = 2,
        .i2c_bus_bx5b3a = 2,
        .i2c_bus_ts = 5,
        .i2c_bus_gps = 5,
        .i2c_bus_saf7741 = 5,
        .i2c_bus_tef6638 = -1,
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
};

