
#include "lidbg.h"

#define GTP_MAX_TOUCH	5

struct lidbg_ts_data *ts_data = NULL;

int lidbg_init_input(struct lidbg_ts_data *pinput)
{
    int ret;
    char phys[32];
    ts_data->input_dev = input_allocate_device();
    if (ts_data->input_dev == NULL)
    {
        lidbg("Failed to allocate input device.\n");
        return -ENOMEM;
    }

    ts_data->input_dev->evbit[0] =
        BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
    set_bit(BTN_TOOL_FINGER, (ts_data->input_dev)->keybit);
    __set_bit(INPUT_PROP_DIRECT, (ts_data->input_dev)->propbit);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
    input_mt_init_slots(ts_data->input_dev, GTP_MAX_TOUCH);
#else
    input_mt_init_slots(ts_data->input_dev, GTP_MAX_TOUCH, 0);
#endif
    input_set_abs_params(ts_data->input_dev, ABS_MT_POSITION_X,
                         0, pinput->abs_x_max, 0, 0);
    input_set_abs_params(ts_data->input_dev, ABS_MT_POSITION_Y,
                         0, pinput->abs_y_max, 0, 0);
    input_set_abs_params(ts_data->input_dev, ABS_MT_WIDTH_MAJOR,
                         0, 255, 0, 0);
    input_set_abs_params(ts_data->input_dev, ABS_MT_TOUCH_MAJOR,
                         0, 255, 0, 0);
    input_set_abs_params(ts_data->input_dev, ABS_MT_TRACKING_ID,
                         0, 255, 0, 0);

    snprintf(phys, 32, "input/lidbg_ts");
    ts_data->input_dev->name = "lidbg_ts";
    ts_data->input_dev->phys = phys;
    ts_data->input_dev->id.bustype = BUS_I2C;
    ts_data->input_dev->id.vendor = 0xDEA1;
    ts_data->input_dev->id.product = 0xBEE1;
    ts_data->input_dev->id.version = 10428;

    ret = input_register_device(ts_data->input_dev);
    if (ret)
    {
        lidbg("input device failed.\n");
        input_free_device(ts_data->input_dev);
        ts_data->input_dev = NULL;
        return ret;
    }

    return 0;

}


void lidbg_touch_handle(touch_type t, int id, int x, int y, int w)
{
    if(t == TOUCH_DOWN)
    {
        input_mt_slot(ts_data->input_dev, id);
        input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, true);
        input_report_abs(ts_data->input_dev, ABS_MT_POSITION_X, x);
        input_report_abs(ts_data->input_dev, ABS_MT_POSITION_Y, y);
        input_report_abs(ts_data->input_dev, ABS_MT_TOUCH_MAJOR, w);
        input_report_abs(ts_data->input_dev, ABS_MT_WIDTH_MAJOR, w);
        ts_data->touch_cnt++;
        pr_debug("%s:%d[%d,%d,%d]\n", __FUNCTION__, id, x, y, w);
        if (ts_data->touch_cnt == 100)
        {
            ts_data->touch_cnt = 0;
            lidbg("%d[%d,%d,%d];\n", id, x, y, w);
        }
        if(id == 0)
        {
            g_curr_tspara.x = x;
            g_curr_tspara.y = y;
            g_curr_tspara.press = true;
        }
    }
    else if (t == TOUCH_UP)
    {

        input_mt_slot(ts_data->input_dev, id);
        input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, false);
        if(id == 0)
            g_curr_tspara.press = false;
    }
    else if (t == TOUCH_SYNC)
    {
        input_sync(ts_data->input_dev);
    }
}
void lidbg_touch_report(struct lidbg_ts_data *pdata)
{
#if 0
    int i;

    static int touch_cnt = 0;
    for (i = 0; i < GTP_MAX_TOUCH; i++)
    {
        input_mt_slot(ts_data->input_dev, pdata->id[i]);

        if ((pdata->touch_index) & (0x01 << i))
        {
            input_mt_slot(ts_data->input_dev, pdata->id[i]);
            input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, true);
            input_report_abs(ts_data->input_dev, ABS_MT_POSITION_X, pdata->x[i]);
            input_report_abs(ts_data->input_dev, ABS_MT_POSITION_Y, pdata->y[i]);
            input_report_abs(ts_data->input_dev, ABS_MT_TOUCH_MAJOR, pdata->w[i]);
            input_report_abs(ts_data->input_dev, ABS_MT_WIDTH_MAJOR, pdata->w[i]);
            touch_cnt++;
            if (touch_cnt == 100)
            {
                touch_cnt = 0;
                lidbg("%d[%d,%d,%d];\n", pdata->id[i], pdata->x[i], pdata->y[i], pdata->w[i]);
            }
            pdata->touch_index |= (0x01 << pdata->id[i + 1]);

        }
        else
        {
            input_mt_slot(ts_data->input_dev, i);
            input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, false);
        }

    }
    input_sync(ts_data->input_dev);
#endif
}

void  touch_event_init(void);
int lidbg_touch_init(void)
{
    ts_data = kzalloc(sizeof(struct lidbg_ts_data), GFP_KERNEL);

    soc_get_screen_res(&(ts_data->abs_x_max), &(ts_data->abs_y_max));
    lidbg_init_input(ts_data);
    touch_event_init();
    LIDBG_MODULE_LOG;
    return 0;
}


void lidbg_touch_deinit(void)
{
    input_unregister_device(ts_data->input_dev);
}


void lidbg_touch_main(int argc, char **argv)
{
    touch_type type;
    u32 x, y;
    if(argc < 2)
    {
        lidbg("Usage:\n");
        lidbg("x y type\n");
        return;
    }

    x = simple_strtoul(argv[0], 0, 0);
    y = simple_strtoul(argv[1], 0, 0);
    type = simple_strtoul(argv[2], 0, 0);
    lidbg_touch_handle(type, 0, x, y, 30);
    lidbg_touch_handle(TOUCH_SYNC, 0, 0, 0, 0);

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");


EXPORT_SYMBOL(lidbg_touch_main);
EXPORT_SYMBOL(lidbg_touch_report);
EXPORT_SYMBOL(lidbg_touch_handle);

module_init(lidbg_touch_init);
module_exit(lidbg_touch_deinit);



