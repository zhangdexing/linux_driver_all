/* drivers/input/touchscreen/gt9xx.c
 *
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Linux Foundation chooses to take subject only to the GPLv2 license
 * terms, and distributes only under these terms.
 *
 * 2010 - 2013 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Version: 1.8
 * Authors: andrew@goodix.com, meta@goodix.com
 * Release Date: 2013/04/25
 * Revision record:
 *      V1.0:
 *          first Release. By Andrew, 2012/08/31
 *      V1.2:
 *          modify gtp_reset_guitar,slot report,tracking_id & 0x0F.
 *                  By Andrew, 2012/10/15
 *      V1.4:
 *          modify gt9xx_update.c. By Andrew, 2012/12/12
 *      V1.6:
 *          1. new heartbeat/esd_protect mechanism(add external watchdog)
 *          2. doze mode, sliding wakeup
 *          3. 3 more cfg_group(GT9 Sensor_ID: 0~5)
 *          3. config length verification
 *          4. names & comments
 *                  By Meta, 2013/03/11
 *      V1.8:
 *          1. pen/stylus identification
 *          2. read double check & fixed config support
 *          2. new esd & slide wakeup optimization
 *                  By Meta, 2013/06/08
 */

#include "gt9xx.h"
#include "lidbg.h"
#include <linux/of_gpio.h>

#include <linux/input/mt.h>
#include "touch.h"
touch_t touch = {0, 0, 0};
LIDBG_DEFINE;

#define GOODIX_DEV_NAME	"Goodix-CTP"
#define CFG_MAX_TOUCH_POINTS	5
#define GOODIX_COORDS_ARR_SIZE	4
#define MAX_BUTTONS		4

/* HIGH: 0x28/0x29, LOW: 0xBA/0xBB */
#define GTP_I2C_ADDRESS_HIGH	0x14
#define GTP_I2C_ADDRESS_LOW	0x5D
#define CFG_GROUP_LEN(p_cfg_grp)  (sizeof(p_cfg_grp) / sizeof(p_cfg_grp[0]))

#define GOODIX_VTG_MIN_UV	2600000
#define GOODIX_VTG_MAX_UV	3300000
#define GOODIX_I2C_VTG_MIN_UV	1800000
#define GOODIX_I2C_VTG_MAX_UV	1800000
#define GOODIX_VDD_LOAD_MIN_UA	0
#define GOODIX_VDD_LOAD_MAX_UA	10000
#define GOODIX_VIO_LOAD_MIN_UA	0
#define GOODIX_VIO_LOAD_MAX_UA	10000

#define RESET_DELAY_T3_US	200	/* T3: > 100us */
#define RESET_DELAY_T4		6	/* T4: > 5ms */

#define	PHY_BUF_SIZE		32

#define GTP_MAX_TOUCH		5
#define GTP_ESD_CHECK_CIRCLE_MS	2000

#if GTP_HAVE_TOUCH_KEY
static const u16 touch_key_array[] = {KEY_MENU, KEY_HOMEPAGE, KEY_BACK};
#define GTP_MAX_KEY_NUM  (sizeof(touch_key_array)/sizeof(touch_key_array[0]))

#if GTP_DEBUG_ON
static const int  key_codes[] =
{
    KEY_HOME, KEY_BACK, KEY_MENU, KEY_SEARCH
};
static const char *const key_names[] =
{
    "Key_Home", "Key_Back", "Key_Menu", "Key_Search"
};
#endif
#endif

#ifdef SOC_mt3360
static int scan_int_config(int ext_in);
extern void BIM_SetEInt(unsigned int EIntNumber, unsigned int type, unsigned int debunceTime);
extern void BIM_EnableEInt(unsigned int EIntNumber);
extern void BIM_DisableEInt(unsigned int EIntNumber);
extern void ac83xx_mask_ack_bim_irq(unsigned int irq);

static struct mtk_ext_int ctp_int_config[] =
{
    { PIN_37_EINT2, 2, VECTOR_EXT3, EINT2_SEL},
    NULL,
};
#endif

static void gtp_reset_guitar(struct goodix_ts_data *ts, int ms);
static void gtp_int_sync(struct goodix_ts_data *ts, int ms);
static int gtp_i2c_test(struct i2c_client *client);

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
                                unsigned long event, void *data);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif

#if GTP_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct *gtp_esd_check_workqueue;
static void gtp_esd_check_func(struct work_struct *work);
static int gtp_init_ext_watchdog(struct i2c_client *client);
struct i2c_client  *i2c_connect_client;
#endif

#if GTP_SLIDE_WAKEUP
enum doze_status
{
    DOZE_DISABLED = 0,
    DOZE_ENABLED = 1,
    DOZE_WAKEUP = 2,
};
static enum doze_status = DOZE_DISABLED;
static s8 gtp_enter_doze(struct goodix_ts_data *ts);
#endif

static void goodix_ts_resume(struct goodix_ts_data *ts);
static void goodix_ts_suspend(struct goodix_ts_data *ts);

bool init_done;
static u8 chip_gt9xxs;  /* true if ic is gt9xxs, like gt915s */
u8 grp_cfg_version;
extern  bool is_ts_load;
extern int ts_should_revert;
static bool xy_revert_en = 1;
/*******************************************************
Function:
	Read data from the i2c slave device.
Input:
	client:     i2c device.
	buf[0~1]:   read start address.
	buf[2~len-1]:   read data buffer.
	len:    GTP_ADDR_LENGTH + read bytes count
Output:
	numbers of i2c_msgs to transfer:
		2: succeed, otherwise: failed
*********************************************************/
int gtp_i2c_read(struct i2c_client *client, u8 *buf, int len)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
    int ret = -EIO;
    int retries = 0;
    unsigned int sub_addr;

    GTP_DEBUG_FUNC();

    sub_addr = (unsigned int)(buf[0] << 8) + buf[1];

    while (retries < 5)
    {

        ret = SOC_I2C_Rec_2B_SubAddr(TS_I2C_BUS, client->addr, sub_addr , buf + 2, len - 2);
        if (ret == 2)
            break;
        retries++;
        msleep(300);
    }
    if (retries >= 5)
    {
#if GTP_SLIDE_WAKEUP
        /* reset chip would quit doze mode */
        if (DOZE_ENABLED == doze_status)
            return ret;
#endif
        GTP_DEBUG("I2C communication timeout, resetting chip...");
        if (init_done)
            gtp_reset_guitar(ts, 10);
        else
            dev_warn(&client->dev,
                     "<GTP> gtp_reset_guitar exit init_done=%d:\n",
                     init_done);
    }
    return ret;
}

/*******************************************************
Function:
	Write data to the i2c slave device.
Input:
	client:     i2c device.
	buf[0~1]:   write start address.
	buf[2~len-1]:   data buffer
	len:    GTP_ADDR_LENGTH + write bytes count
Output:
	numbers of i2c_msgs to transfer:
	1: succeed, otherwise: failed
*********************************************************/
int gtp_i2c_write(struct i2c_client *client, u8 *buf, int len)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
    int ret = -EIO;
    int retries = 0;

    GTP_DEBUG_FUNC();

    while (retries < 5)
    {
        ret = SOC_I2C_Send(TS_I2C_BUS, client->addr, buf, len);
        if (ret == 1)
            break;
        retries++;
        msleep(300);
    }
    if ((retries >= 5))
    {
#if GTP_SLIDE_WAKEUP
        if (DOZE_ENABLED == doze_status)
            return ret;
#endif
        GTP_DEBUG("I2C communication timeout, resetting chip...");
        if (init_done)
            gtp_reset_guitar(ts, 10);
        else
            dev_warn(&client->dev,
                     "<GTP> gtp_reset_guitar exit init_done=%d:\n",
                     init_done);
    }
    return ret;
}
/*******************************************************
Function:
	i2c read twice, compare the results
Input:
	client:  i2c device
	addr:    operate address
	rxbuf:   read data to store, if compare successful
	len:     bytes to read
Output:
	FAIL:    read failed
	SUCCESS: read successful
*********************************************************/
int gtp_i2c_read_dbl_check(struct i2c_client *client,
                           u16 addr, u8 *rxbuf, int len)
{
    u8 buf[16] = {0};
    u8 confirm_buf[16] = {0};
    u8 retry = 0;

    while (retry++ < 3)
    {
        memset(buf, 0xAA, 16);
        buf[0] = (u8)(addr >> 8);
        buf[1] = (u8)(addr & 0xFF);
        gtp_i2c_read(client, buf, len + 2);

        memset(confirm_buf, 0xAB, 16);
        confirm_buf[0] = (u8)(addr >> 8);
        confirm_buf[1] = (u8)(addr & 0xFF);
        gtp_i2c_read(client, confirm_buf, len + 2);

        if (!memcmp(buf, confirm_buf, len + 2))
            break;
    }
    if (retry < 3)
    {
        memcpy(rxbuf, confirm_buf + 2, len);
        return SUCCESS;
    }
    else
    {
        dev_err(&client->dev,
                "i2c read 0x%04X, %d bytes, double check failed!",
                addr, len);
        return FAIL;
    }
}

/*******************************************************
Function:
	Send config data.
Input:
	client: i2c device.
Output:
	result of i2c write operation.
	> 0: succeed, otherwise: failed
*********************************************************/
static int gtp_send_cfg(struct goodix_ts_data *ts)
{
    int ret;
#if GTP_DRIVER_SEND_CFG
    int retry = 0;

    if (ts->fixed_cfg)
    {
        dev_dbg(&ts->client->dev,
                "Ic fixed config, no config sent!");
        ret = 2;
    }
    else
    {
        for (retry = 0; retry < 5; retry++)
        {
            ret = gtp_i2c_write(ts->client,
                                ts->config_data,
                                GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
            if (ret > 0)
                break;
        }
    }
#endif

    return ret;
}

/*******************************************************
Function:
	Disable irq function
Input:
	ts: goodix i2c_client private data
Output:
	None.
*********************************************************/
void gtp_irq_disable(struct goodix_ts_data *ts)
{
    unsigned long irqflags;
#ifdef SOC_mt3360
    int index = 0;
    struct mtk_ext_int ctp_int;

    index = scan_int_config(ts->client->irq);
    ctp_int = ctp_int_config[index];
#endif
    GTP_DEBUG_FUNC();


    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!ts->irq_is_disabled)
    {
        ts->irq_is_disabled = true;
#ifdef SOC_mt3360
        disable_irq_nosync(ctp_int.vector_irq_num);
        BIM_DisableEInt(ctp_int.ext_int_number);
#else
        disable_irq_nosync(ts->client->irq);
#endif
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Enable irq function
Input:
	ts: goodix i2c_client private data
Output:
	None.
*********************************************************/
void gtp_irq_enable(struct goodix_ts_data *ts)
{
    unsigned long irqflags = 0;
#ifdef SOC_mt3360
    int index = 0;
    struct mtk_ext_int ctp_int;

    index = scan_int_config(ts->client->irq);
    ctp_int = ctp_int_config[index];
#endif
    GTP_DEBUG_FUNC();

    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (ts->irq_is_disabled)
    {
#ifdef SOC_mt3360
        enable_irq(ctp_int.vector_irq_num);
        BIM_EnableEInt(ctp_int.ext_int_number);
#else
        enable_irq(ts->client->irq);
#endif
        ts->irq_is_disabled = false;
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Report touch point event
Input:
	ts: goodix i2c_client private data
	id: trackId
	x:  input x coordinate
	y:  input y coordinate
	w:  input pressure
Output:
	None.
*********************************************************/
/*static void gtp_touch_down(struct goodix_ts_data *ts, int id, int x, int y,
		int w)
{

	input_mt_slot(ts->input_dev, id);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
	input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
	touch_cnt++;
    if (touch_cnt == 100)
    {
        touch_cnt = 0;
        lidbg("%d[%d,%d];\n", id, x, y);
    }
}
*/
/*******************************************************
Function:
	Report touch release event
Input:
	ts: goodix i2c_client private data
Output:
	None.
*********************************************************/
static void gtp_touch_up(struct goodix_ts_data *ts, int id)
{
    input_mt_slot(ts->input_dev, id);
    input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
    GTP_DEBUG("Touch id[%2d] release!", id);
}



/*******************************************************
Function:
	Goodix touchscreen work function
Input:
	work: work struct of goodix_workqueue
Output:
	None.
*********************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    u8 end_cmd[3] = { GTP_READ_COOR_ADDR >> 8,
                      GTP_READ_COOR_ADDR & 0xFF, 0
                    };
    u8 point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1] =
    {
        GTP_READ_COOR_ADDR >> 8,
        GTP_READ_COOR_ADDR & 0xFF
    };
    u8 touch_num = 0;
    u8 finger = 0;
    static u16 pre_touch;
    static u8 pre_key;
#if GTP_WITH_PEN
    static u8 pre_pen;
#endif
    u8 key_value = 0;
    u8 *coor_data = NULL;
    s32 input_x = 0;
    s32 input_y = 0;
    s32 input_w = 0;
    s32 id = 0;
    s32 i = 0;
    int ret = -1;
    struct goodix_ts_data *ts = NULL;

#if GTP_SLIDE_WAKEUP
    u8 doze_buf[3] = {0x81, 0x4B};
#endif

    GTP_DEBUG_FUNC();

    ts = container_of(work, struct goodix_ts_data, work);
#ifdef CONFIG_GT9XX_TOUCHPANEL_UPDATE
    if (ts->enter_update)
        return;
#endif

#if GTP_SLIDE_WAKEUP
    if (DOZE_ENABLED == doze_status)
    {
        ret = gtp_i2c_read(ts->client, doze_buf, 3);
        GTP_DEBUG("0x814B = 0x%02X", doze_buf[2]);
        if (ret > 0)
        {
            if (doze_buf[2] == 0xAA)
            {
                dev_dbg(&ts->client->dev,
                        "Slide(0xAA) To Light up the screen!");
                doze_status = DOZE_WAKEUP;
                input_report_key(
                    ts->input_dev, KEY_POWER, 1);
                input_sync(ts->input_dev);
                input_report_key(
                    ts->input_dev, KEY_POWER, 0);
                input_sync(ts->input_dev);
                /* clear 0x814B */
                doze_buf[2] = 0x00;
                gtp_i2c_write(ts->client, doze_buf, 3);
            }
            else if (doze_buf[2] == 0xBB)
            {
                dev_dbg(&ts->client->dev,
                        "Slide(0xBB) To Light up the screen!");
                doze_status = DOZE_WAKEUP;
                input_report_key(ts->input_dev, KEY_POWER, 1);
                input_sync(ts->input_dev);
                input_report_key(ts->input_dev, KEY_POWER, 0);
                input_sync(ts->input_dev);
                /* clear 0x814B*/
                doze_buf[2] = 0x00;
                gtp_i2c_write(ts->client, doze_buf, 3);
            }
            else if (0xC0 == (doze_buf[2] & 0xC0))
            {
                dev_dbg(&ts->client->dev,
                        "double click to light up the screen!");
                doze_status = DOZE_WAKEUP;
                input_report_key(ts->input_dev, KEY_POWER, 1);
                input_sync(ts->input_dev);
                input_report_key(ts->input_dev, KEY_POWER, 0);
                input_sync(ts->input_dev);
                /* clear 0x814B */
                doze_buf[2] = 0x00;
                gtp_i2c_write(ts->client, doze_buf, 3);
            }
            else
            {
                gtp_enter_doze(ts);
            }
        }
        if (ts->use_irq)
            gtp_irq_enable(ts);

        return;
    }
#endif

    ret = gtp_i2c_read(ts->client, point_data, 12);
    if (ret < 0)
    {
        dev_err(&ts->client->dev,
                "I2C transfer error. errno:%d\n ", ret);
        goto exit_work_func;
    }

    finger = point_data[GTP_ADDR_LENGTH];
    if (finger == 0x00)
    {
        if (ts->use_irq)
        {
            gtp_irq_enable(ts);
        }
        return;
    }

    touch_num = finger & 0x0f;
    if (touch_num > GTP_MAX_TOUCH)
        goto exit_work_func;

    if (touch_num > 1)
    {
        u8 buf[8 * GTP_MAX_TOUCH] = { (GTP_READ_COOR_ADDR + 10) >> 8,
                                      (GTP_READ_COOR_ADDR + 10) & 0xff
                                    };

        ret = gtp_i2c_read(ts->client, buf,
                           2 + 8 * (touch_num - 1));
        memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
    }

#if GTP_HAVE_TOUCH_KEY
    key_value = point_data[3 + 8 * touch_num];

    if (key_value || pre_key)
    {
        for (i = 0; i < GTP_MAX_KEY_NUM; i++)
        {
#if GTP_DEBUG_ON
            for (ret = 0; ret < 4; ++ret)
            {
                if (key_codes[ret] == touch_key_array[i])
                {
                    GTP_DEBUG("Key: %s %s",
                              key_names[ret],
                              (key_value & (0x01 << i))
                              ? "Down" : "Up");
                    break;
                }
            }
#endif

            input_report_key(ts->input_dev,
                             touch_key_array[i], key_value & (0x01 << i));
        }
        touch_num = 0;
        pre_touch = 0;
    }
#endif
    pre_key = key_value;

    GTP_DEBUG("pre_touch:%02x, finger:%02x.", pre_touch, finger);

#if GTP_WITH_PEN
    if (pre_pen && (touch_num == 0))
    {
        GTP_DEBUG("Pen touch UP(Slot)!");
        input_report_key(ts->input_dev, BTN_TOOL_PEN, 0);
        input_mt_slot(ts->input_dev, 5);
        input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
        pre_pen = 0;
    }
#endif
    if (pre_touch || touch_num)
    {
        s32 pos = 0;
        u16 touch_index = 0;

        coor_data = &point_data[3];
        if (touch_num)
        {
            id = coor_data[pos] & 0x0F;
#if GTP_WITH_PEN
            id = coor_data[pos];
            if (id == 128)
            {
                GTP_DEBUG("Pen touch DOWN(Slot)!");
                input_x  = coor_data[pos + 1]
                           | (coor_data[pos + 2] << 8);
                input_y  = coor_data[pos + 3]
                           | (coor_data[pos + 4] << 8);
                input_w  = coor_data[pos + 5]
                           | (coor_data[pos + 6] << 8);

                input_report_key(ts->input_dev,
                                 BTN_TOOL_PEN, 1);
                input_mt_slot(ts->input_dev, 5);
                input_report_abs(ts->input_dev,
                                 ABS_MT_TRACKING_ID, 5);
                input_report_abs(ts->input_dev,
                                 ABS_MT_POSITION_X, input_x);
                input_report_abs(ts->input_dev,
                                 ABS_MT_POSITION_Y, input_y);
                input_report_abs(ts->input_dev,
                                 ABS_MT_TOUCH_MAJOR, input_w);
                GTP_DEBUG("Pen/Stylus: (%d, %d)[%d]",
                          input_x, input_y, input_w);
                pre_pen = 1;
                pre_touch = 0;
            }
#endif

            touch_index |= (0x01 << id);
        }

        GTP_DEBUG("id = %d,touch_index = 0x%x, pre_touch = 0x%x\n",
                  id, touch_index, pre_touch);
        for (i = 0; i < GTP_MAX_TOUCH; i++)
        {
#if GTP_WITH_PEN
            if (pre_pen == 1)
                break;
#endif
            if (touch_index & (0x01 << i))
            {
                input_x = coor_data[pos + 1] |
                          coor_data[pos + 2] << 8;
                input_y = coor_data[pos + 3] |
                          coor_data[pos + 4] << 8;
                input_w = coor_data[pos + 5] |
                          coor_data[pos + 6] << 8;

                //gtp_touch_down(ts, id,input_x, input_y, input_w);
                ts_data_report(TOUCH_DOWN, id, input_x, input_y, input_w);
                pre_touch |= 0x01 << i;

                pos += 8;
                id = coor_data[pos] & 0x0F;
                touch_index |= (0x01 << id);
            }
            else
            {
                //gtp_touch_up(ts, i);
                ts_data_report(TOUCH_UP, i, 0, 0, 0);
                pre_touch &= ~(0x01 << i);
            }
            if (touch_index & (0x01 << 0))
            {
                if(1 == g_var.recovery_mode)
                {
                    if( (input_y >= 0) && (input_x >= 0) )
                    {
                        touch.x = point_data[6] | (point_data[7] << 8);
                        touch.y = point_data[4] | (point_data[5] << 8);
                        if (1 == ts_should_revert)
                            GTP_REVERT(touch.x, touch.y);
                        touch.pressed = 1;
                        set_touch_pos(&touch);
                        //lidbg("[%d,%d]==========%d\n", touch.x, touch.y, touch.pressed);
                    }
                }
            }
            else
            {
                if(1 == g_var.recovery_mode)
                {
                    touch.pressed = 0;
                    set_touch_pos(&touch);
                    //lidbg("[%d,%d]==========%d\n", touch.x, touch.y, touch.pressed);
                }

            }
        }
    }
    //input_sync(ts->input_dev);
    ts_data_report(TOUCH_SYNC, 0, 0, 0, 0);

exit_work_func:
    if (!ts->gtp_rawdiff_mode)
    {
        ret = gtp_i2c_write(ts->client, end_cmd, 3);
        if (ret < 0)
            dev_warn(&ts->client->dev, "I2C write end_cmd error!\n");

    }
    if (ts->use_irq)
        gtp_irq_enable(ts);

    return;
}

/*******************************************************
Function:
	Timer interrupt service routine for polling mode.
Input:
	timer: timer struct pointer
Output:
	Timer work mode.
	HRTIMER_NORESTART: no restart mode
*********************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
    struct goodix_ts_data
    *ts = container_of(timer, struct goodix_ts_data, timer);

    GTP_DEBUG_FUNC();

    queue_work(ts->goodix_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME + 6) * 1000000),
                  HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Function:
	External interrupt service routine for interrupt mode.
Input:
	irq:  interrupt number.
	dev_id: private data pointer
Output:
	Handle Result.
	IRQ_HANDLED: interrupt handled successfully
*********************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    struct goodix_ts_data *ts = dev_id;

    GTP_DEBUG_FUNC();

    gtp_irq_disable(ts);
#ifdef SOC_mt3360
    ac83xx_mask_ack_bim_irq(irq);
#endif
    queue_work(ts->goodix_wq, &ts->work);

    return IRQ_HANDLED;
}
/*******************************************************
Function:
	Synchronization.
Input:
	ms: synchronization time in millisecond.
Output:
	None.
*******************************************************/
void gtp_int_sync(struct goodix_ts_data *ts, int ms)
{
    SOC_IO_Output(0, GTP_INT_PORT, 0);
    msleep(ms);
    SOC_IO_Input(0, GTP_INT_PORT, 0);
}

/*******************************************************
Function:
	Reset chip.
Input:
	ms: reset time in millisecond, must >10ms
Output:
	None.
*******************************************************/
static void gtp_reset_guitar(struct goodix_ts_data *ts, int ms)
{
    GTP_DEBUG_FUNC();
#ifdef SOC_mt3360
    int ret = -1000;

    ret  = GPIO_MultiFun_Set(GTP_RST_PORT, PINMUX_LEVEL_GPIO_END_FLAG);
#endif
    /* This reset sequence will selcet I2C slave address */
    SOC_IO_Output(0, GTP_RST_PORT, 0);
    msleep(ms);

    if (ts->client->addr == GTP_I2C_ADDRESS_HIGH)
        SOC_IO_Output(0, GTP_INT_PORT, 1);
    else
        SOC_IO_Output(0, GTP_INT_PORT, 0);

    udelay(RESET_DELAY_T3_US);

    SOC_IO_Output(0, GTP_RST_PORT, 1);
    msleep(RESET_DELAY_T4);

    //SOC_IO_Input(0,GTP_RST_PORT,0);

    gtp_int_sync(ts, 50);

#if GTP_ESD_PROTECT
    gtp_init_ext_watchdog(ts->client);
#endif
}

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_FB)
#if GTP_SLIDE_WAKEUP
/*******************************************************
Function:
	Enter doze mode for sliding wakeup.
Input:
	ts: goodix tp private data
Output:
	1: succeed, otherwise failed
*******************************************************/
static s8 gtp_enter_doze(struct goodix_ts_data *ts)
{
    int ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] =
    {
        (u8)(GTP_REG_SLEEP >> 8),
        (u8)GTP_REG_SLEEP, 8
    };

    GTP_DEBUG_FUNC();

#if GTP_DBL_CLK_WAKEUP
    i2c_control_buf[2] = 0x09;
#endif
    gtp_irq_disable(ts);

    GTP_DEBUG("entering doze mode...");
    while (retry++ < 5)
    {
        i2c_control_buf[0] = 0x80;
        i2c_control_buf[1] = 0x46;
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        if (ret < 0)
        {
            GTP_DEBUG(
                "failed to set doze flag into 0x8046, %d",
                retry);
            continue;
        }
        i2c_control_buf[0] = 0x80;
        i2c_control_buf[1] = 0x40;
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        if (ret > 0)
        {
            doze_status = DOZE_ENABLED;
            dev_dbg(&ts->client->dev,
                    "GTP has been working in doze mode!");
            gtp_irq_enable(ts);
            return ret;
        }
        msleep(20);
    }
    dev_err(&ts->client->dev, "GTP send doze cmd failed.\n");
    gtp_irq_enable(ts);
    return ret;
}
#else
/*******************************************************
Function:
	Enter sleep mode.
Input:
	ts: private data.
Output:
	Executive outcomes.
	1: succeed, otherwise failed.
*******************************************************/
static s8 gtp_enter_sleep(struct goodix_ts_data  *ts)
{
    int ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] =
    {
        (u8)(GTP_REG_SLEEP >> 8),
        (u8)GTP_REG_SLEEP, 5
    };

    GTP_DEBUG_FUNC();

    SOC_IO_Output(0, GTP_INT_PORT, 0);
    mdelay(5);
#if (defined SOC_mt3360)
    return 0;
#endif
    while (retry++ < 5)
    {
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        if (ret > 0)
        {
            dev_dbg(&ts->client->dev,
                    "GTP enter sleep!");
            return ret;
        }
        msleep(20);
    }
    dev_err(&ts->client->dev, "GTP send sleep cmd failed.\n");
    return ret;
}
#endif

/*******************************************************
Function:
	Wakeup from sleep.
Input:
	ts: private data.
Output:
	Executive outcomes.
	>0: succeed, otherwise: failed.
*******************************************************/
static s8 gtp_wakeup_sleep(struct goodix_ts_data *ts)
{
    u8 retry = 0;
    s8 ret = -1;

    GTP_DEBUG_FUNC();
    gtp_reset_guitar(ts, 20);

#if GTP_POWER_CTRL_SLEEP
    gtp_reset_guitar(ts, 20);

    ret = gtp_send_cfg(ts);
    if (ret > 0)
    {
        dev_dbg(&ts->client->dev,
                "Wakeup sleep send config success.");
        return 1;
    }
#else
    while (retry++ < 10)
    {
#if GTP_SLIDE_WAKEUP
        /* wakeup not by slide */
        if (DOZE_WAKEUP != doze_status)
            gtp_reset_guitar(ts, 10);
        else
            /* wakeup by slide */
            doze_status = DOZE_DISABLED;
#else
        if (chip_gt9xxs == 1)
        {
            gtp_reset_guitar(ts, 10);
        }
        else
        {
            SOC_IO_Output(0, GTP_INT_PORT, 1);
            msleep(5);
        }
#endif
        ret = gtp_i2c_test(ts->client);
        if (ret > 0)
        {
            dev_dbg(&ts->client->dev, "GTP wakeup sleep.");
#if (!GTP_SLIDE_WAKEUP)
            if (chip_gt9xxs == 0)
            {
                gtp_int_sync(ts, 25);
                msleep(20);
#if GTP_ESD_PROTECT
                gtp_init_ext_watchdog(ts->client);
#endif
            }
#endif
            return ret;
        }
        gtp_reset_guitar(ts, 20);
    }
#endif

    dev_err(&ts->client->dev, "GTP wakeup sleep failed.\n");
    return ret;
}
#endif /* !CONFIG_HAS_EARLYSUSPEND && !CONFIG_FB*/

/*******************************************************
Function:
	Initialize gtp.
Input:
	ts: goodix private data
Output:
	Executive outcomes.
	> =0: succeed, otherwise: failed
*******************************************************/


static int gtp_init_panel(struct goodix_ts_data *ts, char *ic_type)
{
    struct i2c_client *client = ts->client;
    unsigned char *config_data;
    int ret = -EIO;
#ifdef SOC_mt3360
    u8 irq_cfg = 0;
#endif

#if GTP_DRIVER_SEND_CFG
    int i;
    u8 check_sum = 0;
    u8 opr_buf[16];
    u8 sensor_id = 0;

    extern u8 *send_cfg_buf[];
    extern u8 cfg_info_len[];

#if 0
    if(g_var.hw_info.hw_version == 1)
    {
        send_cfg_buf[0] = cfg_info_group1_v1;
        send_cfg_buf[1] = cfg_info_group2_v1;

        cfg_info_len[0] = CFG_GROUP_LEN(cfg_info_group1_v1);
        cfg_info_len[1] = CFG_GROUP_LEN(cfg_info_group2_v1);
    }
#endif

    lidbg("Config Groups\' Lengths: %d, %d, %d, %d, %d, %d\n",
          cfg_info_len[0], cfg_info_len[1], cfg_info_len[2],
          cfg_info_len[3], cfg_info_len[4], cfg_info_len[5]);

    ret = gtp_i2c_read_dbl_check(ts->client, 0x41E4, opr_buf, 1);
    if (SUCCESS == ret)
    {
        if (opr_buf[0] != 0xBE)
        {
            ts->fw_error = 1;
            dev_err(&client->dev,
                    "Firmware error, no config sent!");
            return -EINVAL;
        }
    }
    if ((!cfg_info_len[1]) && (!cfg_info_len[2]) && (!cfg_info_len[3])
            && (!cfg_info_len[4]) && (!cfg_info_len[5]))
    {
        sensor_id = 0;
    }
    else
    {
        ret = gtp_i2c_read_dbl_check(ts->client, GTP_REG_SENSOR_ID,
                                     &sensor_id, 1);
        if (SUCCESS == ret)
        {
            if (sensor_id >= 0x06)
            {
                dev_err(&client->dev,
                        "Invalid sensor_id(0x%02X), No Config Sent!",
                        sensor_id);
                return -EINVAL;
            }
        }
        else
        {
            dev_err(&client->dev,
                    "Failed to get sensor_id, No config sent!");
            return -EINVAL;
        }
    }
    lidbg("Sensor_ID: %d", sensor_id);

    if(!strcmp(ic_type, "927"))
    {
        lidbg("ic_type, 927\n");
        sensor_id = sensor_id + 6;
    }

    if(g_var.hw_info.ts_config != 0)
    {
        sensor_id = g_var.hw_info.ts_config - 1;
        lidbg("Sensor_ID change to: %d", sensor_id);
    }
    lidbg_fs_log(TS_LOG_PATH, "SENSOR ID:%d\n", sensor_id);
    ts->gtp_cfg_len = cfg_info_len[sensor_id];

    if (ts->gtp_cfg_len < GTP_CONFIG_MIN_LENGTH)
    {
        dev_err(&client->dev,
                "Sensor_ID(%d) matches with NULL or INVALID CONFIG GROUP! NO Config Sent! You need to check you header file CFG_GROUP section!\n",
                sensor_id);
        return -EINVAL;
    }
    ret = gtp_i2c_read_dbl_check(ts->client, GTP_REG_CONFIG_DATA,
                                 &opr_buf[0], 1);

    if (ret == SUCCESS)
    {
        if (opr_buf[0] <= 90)
        {
            /* backup group config version */
            grp_cfg_version = send_cfg_buf[sensor_id][0];
            send_cfg_buf[sensor_id][0] = 0x00;
            ts->fixed_cfg = 0;
        }
        else
        {
            /* treated as fixed config, not send config */
            dev_warn(&client->dev,
                     "Ic fixed config with config version(%d, 0x%02X)",
                     opr_buf[0], opr_buf[0]);
            ts->fixed_cfg = 1;
        }
    }
    else
    {
        dev_err(&client->dev,
                "Failed to get ic config version!No config sent!");
        return -EINVAL;
    }

{
	int i = 0;
 	lidbg("send_cfg_buf=");
	while(i < 20)
	{
 		lidbg("%d=0x%x \n", i ,send_cfg_buf[sensor_id][i]);
		i++;
	}
 	lidbg("x=%d,y=%d,switch=%d\n",(send_cfg_buf[sensor_id][1]|send_cfg_buf[sensor_id][2]<<8),
								  (send_cfg_buf[sensor_id][3]|send_cfg_buf[sensor_id][4]<<8),
								 ((send_cfg_buf[sensor_id][6])&(0x01<<3)) );
#if 0
	if(((send_cfg_buf[sensor_id][6])&(0x01<<3)) == 0)
	{
		send_cfg_buf[sensor_id][1] = 1024&0xff;
		send_cfg_buf[sensor_id][2] = (1024&(0xff<<8))>>8;
		send_cfg_buf[sensor_id][3] = 600&0xff;
		send_cfg_buf[sensor_id][4] = (600&(0xff<<8))>>8;	
	}
	else
	{
		send_cfg_buf[sensor_id][3] = 1024&0xff;
		send_cfg_buf[sensor_id][4] = (1024&(0xff<<8))>>8;
		send_cfg_buf[sensor_id][1] = 600&0xff;
		send_cfg_buf[sensor_id][2] = (600&(0xff<<8))>>8;	
	}	
 	lidbg("x=%d,y=%d,switch=%d\n",(send_cfg_buf[sensor_id][1]|send_cfg_buf[sensor_id][2]<<8),
								  (send_cfg_buf[sensor_id][3]|send_cfg_buf[sensor_id][4]<<8),
								  ((send_cfg_buf[sensor_id][6])&(0x01<<3)) );
#endif	
}

	
    /*
    	if (ts->pdata->gtp_cfg_len) {// use config from dts
    		config_data = ts->pdata->config_data;
    		ts->config_data = ts->pdata->config_data;
    		ts->gtp_cfg_len = ts->pdata->gtp_cfg_len;
    	} else*/
    {
        config_data = kzalloc(
                          GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH,
                          GFP_KERNEL);
        if (!config_data)
        {
            dev_err(&client->dev,
                    "Not enough memory for panel config data\n");
            return -ENOMEM;
        }
        ts->config_data = config_data;
        config_data[0] = GTP_REG_CONFIG_DATA >> 8;
        config_data[1] = GTP_REG_CONFIG_DATA & 0xff;
#ifdef SOC_mt3360
        irq_cfg = send_cfg_buf[sensor_id][6];
        send_cfg_buf[sensor_id][6] = send_cfg_buf[sensor_id][6] | 0x07;	//confirm ctp irq config is IRQ_TYPE_LEVEL_HIGH on AC8317
        lidbg("********** Change ctp irq config 0x%x -> 0x%x **********\n", irq_cfg, send_cfg_buf[sensor_id][6]);
#endif
        memset(&config_data[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
        memcpy(&config_data[GTP_ADDR_LENGTH], send_cfg_buf[sensor_id],
               ts->gtp_cfg_len);
    }


#if GTP_CUSTOM_CFG
    config_data[RESOLUTION_LOC] =
        (unsigned char)(GTP_MAX_WIDTH && 0xFF);
    config_data[RESOLUTION_LOC + 1] =
        (unsigned char)(GTP_MAX_WIDTH >> 8);
    config_data[RESOLUTION_LOC + 2] =
        (unsigned char)(GTP_MAX_HEIGHT && 0xFF);
    config_data[RESOLUTION_LOC + 3] =
        (unsigned char)(GTP_MAX_HEIGHT >> 8);

    if (GTP_INT_TRIGGER == 0)
        config_data[TRIGGER_LOC] &= 0xfe;
    else if (GTP_INT_TRIGGER == 1)
        config_data[TRIGGER_LOC] |= 0x01;
#endif  /* !GTP_CUSTOM_CFG */

    check_sum = 0;
    for (i = GTP_ADDR_LENGTH; i < ts->gtp_cfg_len; i++)
        check_sum += config_data[i];

    config_data[ts->gtp_cfg_len] = (~check_sum) + 1;

#else /* DRIVER NOT SEND CONFIG */
    ts->gtp_cfg_len = GTP_CONFIG_MAX_LENGTH;
    ret = gtp_i2c_read(ts->client, config_data,
                       ts->gtp_cfg_len + GTP_ADDR_LENGTH);
    if (ret < 0)
    {
        dev_err(&client->dev,
                "Read Config Failed, Using DEFAULT Resolution & INT Trigger!\n");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
        ts->int_trigger_type = GTP_INT_TRIGGER;
    }
#endif /* !DRIVER NOT SEND CONFIG */

    GTP_DEBUG_FUNC();
    if ((ts->abs_x_max == 0) && (ts->abs_y_max == 0))
    {
        ts->abs_x_max = (config_data[RESOLUTION_LOC + 1] << 8)
                        + config_data[RESOLUTION_LOC];

        ts->abs_y_max = (config_data[RESOLUTION_LOC + 3] << 8)
                        + config_data[RESOLUTION_LOC + 2];
        lidbg("RESOLUTION_LOC=%d\n", RESOLUTION_LOC);

        ts->int_trigger_type = (config_data[TRIGGER_LOC]) & 0x03;
    }
    ret = gtp_send_cfg(ts);
    if (ret < 0)
        dev_err(&client->dev, "%s: Send config error.\n", __func__);

    GTP_DEBUG("X_MAX = %d, Y_MAX = %d, TRIGGER = 0x%02x",
              ts->abs_x_max, ts->abs_y_max,
              ts->int_trigger_type);
    msleep(20);
    return ret;
}

/*******************************************************
Function:
	Read chip version.
Input:
	client:  i2c device
	version: buffer to keep ic firmware version
Output:
	read operation return.
	2: succeed, otherwise: failed
*******************************************************/
int gtp_read_version(struct i2c_client *client, u16 *version, char *ic_type)
{
    int ret = -EIO;
    u8 buf[8] = { GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff };

    GTP_DEBUG_FUNC();

    ret = gtp_i2c_read(client, buf, sizeof(buf));
    if (ret < 0)
    {
        dev_err(&client->dev, "GTP read version failed.\n");
        return ret;
    }

    if (version)
        *version = (buf[7] << 8) | buf[6];

    if (buf[5] == 0x00)
    {
        lidbg("==IC Version: %c%c%c_%02x%02x\n", buf[2],
              buf[3], buf[4], buf[7], buf[6]);
    }
    else
    {
        if (buf[5] == 'S' || buf[5] == 's')
            chip_gt9xxs = 1;
        lidbg("****IC Version: %c%c%c%c_%02x%02x\n", buf[2],
              buf[3], buf[4], buf[5], buf[7], buf[6]);
    }
    sprintf(ic_type, "%c%c%c", buf[2], buf[3], buf[4]);
    return ret;
}

/*******************************************************
Function:
	I2c test Function.
Input:
	client:i2c client.
Output:
	Executive outcomes.
	2: succeed, otherwise failed.
*******************************************************/
static int gtp_i2c_test(struct i2c_client *client)
{
    u8 buf[3] = { GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff };
    int retry = 5;
    int ret = -EIO;

    GTP_DEBUG_FUNC();

    while (retry--)
    {
        ret = gtp_i2c_read(client, buf, 3);
        if (ret > 0)
            return ret;
        dev_err(&client->dev, "GTP i2c test failed time %d.\n", retry);
        msleep(20);
    }
    return ret;
}

#ifdef SOC_mt3360
static int scan_int_config(int ext_in)
{
    int ext_int_flag = 0;
    int i = 0;

    for(i = 0; i < ARRAY_SIZE(ctp_int_config); i++)
        if(ext_in == ctp_int_config[i].ext_int_gpio_num)
        {

            ext_int_flag = 1;
            break;
        }
    ext_int_flag = 0;
    return i;
}
#endif
/*******************************************************
Function:
	Request interrupt.
Input:
	ts: private data.
Output:
	Executive outcomes.
	0: succeed, -1: failed.
*******************************************************/
static int gtp_request_irq(struct goodix_ts_data *ts)
{
    int ret;
    const u8 irq_table[] = GTP_IRQ_TAB;
#ifdef SOC_mt3360
    struct mtk_ext_int ctp_int;
    int index = 0;

    index = scan_int_config(ts->client->irq);
    ctp_int = ctp_int_config[index];

    BIM_SetEInt(ctp_int.ext_int_number , EINT_TYPE_HIGHLEVEL , 500);
    GPIO_MultiFun_Set(GTP_INT_PORT, ctp_int.pinmux_function);

    ret = request_irq(ctp_int.vector_irq_num, goodix_ts_irq_handler,
                      IRQ_TYPE_LEVEL_HIGH , ts->client->name, ts);
#else

    ts->client->irq = GPIO_TO_INT(GTP_INT_PORT);
    lidbg("INT trigger type:%x, irq=%d", ts->int_trigger_type,
          ts->client->irq);

    ret = request_irq(ts->client->irq, goodix_ts_irq_handler,
                      irq_table[ts->int_trigger_type],
                      ts->client->name, ts);
#endif
    if (ret)
    {
        dev_err(&ts->client->dev, "Request IRQ failed!ERRNO:%d.\n",
                ret);
        SOC_IO_Input(0, GTP_INT_PORT, 0);

        hrtimer_init(&ts->timer, CLOCK_MONOTONIC,
                     HRTIMER_MODE_REL);
        ts->timer.function = goodix_ts_timer_handler;
        hrtimer_start(&ts->timer, ktime_set(1, 0),
                      HRTIMER_MODE_REL);
        ts->use_irq = false;
        return ret;
    }
    else
    {
        gtp_irq_disable(ts);
        ts->use_irq = true;
        return 0;
    }
}

/*******************************************************
Function:
	Request input device Function.
Input:
	ts:private data.
Output:
	Executive outcomes.
	0: succeed, otherwise: failed.
*******************************************************/
static int gtp_request_input_dev(struct goodix_ts_data *ts)
{
    int ret;
    char phys[PHY_BUF_SIZE];
#if GTP_HAVE_TOUCH_KEY
    int index = 0;
#endif

    GTP_DEBUG_FUNC();

    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        dev_err(&ts->client->dev,
                "Failed to allocate input device.\n");
        return -ENOMEM;
    }

    ts->input_dev->evbit[0] =
        BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
    set_bit(BTN_TOOL_FINGER, ts->input_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    input_mt_init_slots(ts->input_dev, 10);/* in case of "out of memory" */
#else
    input_mt_init_slots(ts->input_dev, 10, 0);/* in case of "out of memory" */
#endif

#if GTP_HAVE_TOUCH_KEY
    for (index = 0; index < GTP_MAX_KEY_NUM; index++)
    {
        input_set_capability(ts->input_dev,
                             EV_KEY, touch_key_array[index]);
    }
#endif

#if GTP_SLIDE_WAKEUP
    input_set_capability(ts->input_dev, EV_KEY, KEY_POWER);
#endif

#if GTP_WITH_PEN
    /* pen support */
    __set_bit(BTN_TOOL_PEN, ts->input_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
    __set_bit(INPUT_PROP_POINTER, ts->input_dev->propbit);
#endif

    if((xy_revert_en == 1) || (1 == ts_should_revert))

        GTP_SWAP(ts->abs_x_max, ts->abs_y_max);

    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
                         0, ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
                         0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR,
                         0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,
                         0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
                         0, 255, 0, 0);

    snprintf(phys, PHY_BUF_SIZE, "input/ts");
    ts->input_dev->name = GOODIX_DEV_NAME;
    ts->input_dev->phys = phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;

    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        dev_err(&ts->client->dev,
                "Register %s input device failed.\n",
                ts->input_dev->name);
        goto exit_free_inputdev;
    }

    return 0;

exit_free_inputdev:
    input_free_device(ts->input_dev);
    ts->input_dev = NULL;
    return ret;
}
/*
static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
		regulator_set_optimum_mode(reg, load_uA) : 0;
}
*/
/**
 * goodix_power_on - Turn device power ON
 * @ts: driver private data
 *
 * Returns zero on success, else an error.
 */
/*
static int goodix_power_on(struct goodix_ts_data *ts)
{
int ret;

if (!IS_ERR(ts->avdd)) {
	ret = reg_set_optimum_mode_check(ts->avdd,
		GOODIX_VDD_LOAD_MAX_UA);
	if (ret < 0) {
		dev_err(&ts->client->dev,
			"Regulator avdd set_opt failed rc=%d\n", ret);
		goto err_set_opt_avdd;
	}
	ret = regulator_enable(ts->avdd);
	if (ret) {
		dev_err(&ts->client->dev,
			"Regulator avdd enable failed ret=%d\n", ret);
		goto err_enable_avdd;
	}
}

if (!IS_ERR(ts->vdd)) {
	ret = regulator_set_voltage(ts->vdd, GOODIX_VTG_MIN_UV,
				   GOODIX_VTG_MAX_UV);
	if (ret) {
		dev_err(&ts->client->dev,
			"Regulator set_vtg failed vdd ret=%d\n", ret);
		goto err_set_vtg_vdd;
	}
	ret = reg_set_optimum_mode_check(ts->vdd,
		GOODIX_VDD_LOAD_MAX_UA);
	if (ret < 0) {
		dev_err(&ts->client->dev,
			"Regulator vdd set_opt failed rc=%d\n", ret);
		goto err_set_opt_vdd;
	}
	ret = regulator_enable(ts->vdd);
	if (ret) {
		dev_err(&ts->client->dev,
			"Regulator vdd enable failed ret=%d\n", ret);
		goto err_enable_vdd;
	}
}

if (!IS_ERR(ts->vcc_i2c)) {
	ret = regulator_set_voltage(ts->vcc_i2c, GOODIX_I2C_VTG_MIN_UV,
				   GOODIX_I2C_VTG_MAX_UV);
	if (ret) {
		dev_err(&ts->client->dev,
			"Regulator set_vtg failed vcc_i2c ret=%d\n",
			ret);
		goto err_set_vtg_vcc_i2c;
	}
	ret = reg_set_optimum_mode_check(ts->vcc_i2c,
		GOODIX_VIO_LOAD_MAX_UA);
	if (ret < 0) {
		dev_err(&ts->client->dev,
			"Regulator vcc_i2c set_opt failed rc=%d\n",
			ret);
		goto err_set_opt_vcc_i2c;
	}
	ret = regulator_enable(ts->vcc_i2c);
	if (ret) {
		dev_err(&ts->client->dev,
			"Regulator vcc_i2c enable failed ret=%d\n",
			ret);
		regulator_disable(ts->vdd);
		goto err_enable_vcc_i2c;
		}
}

return 0;

err_enable_vcc_i2c:
err_set_opt_vcc_i2c:
if (!IS_ERR(ts->vcc_i2c))
	regulator_set_voltage(ts->vcc_i2c, 0, GOODIX_I2C_VTG_MAX_UV);
err_set_vtg_vcc_i2c:
if (!IS_ERR(ts->vdd))
	regulator_disable(ts->vdd);
err_enable_vdd:
err_set_opt_vdd:
if (!IS_ERR(ts->vdd))
	regulator_set_voltage(ts->vdd, 0, GOODIX_VTG_MAX_UV);
err_set_vtg_vdd:
if (!IS_ERR(ts->avdd))
	regulator_disable(ts->avdd);
err_enable_avdd:
err_set_opt_avdd:
return ret;
}
*/
/**
 * goodix_power_off - Turn device power OFF
 * @ts: driver private data
 *
 * Returns zero on success, else an error.
 */
/*
static int goodix_power_off(struct goodix_ts_data *ts)
{
int ret;

if (!IS_ERR(ts->vcc_i2c)) {
	ret = regulator_set_voltage(ts->vcc_i2c, 0,
		GOODIX_I2C_VTG_MAX_UV);
	if (ret < 0)
		dev_err(&ts->client->dev,
			"Regulator vcc_i2c set_vtg failed ret=%d\n",
			ret);
	ret = regulator_disable(ts->vcc_i2c);
	if (ret)
		dev_err(&ts->client->dev,
			"Regulator vcc_i2c disable failed ret=%d\n",
			ret);
}

if (!IS_ERR(ts->vdd)) {
	ret = regulator_set_voltage(ts->vdd, 0, GOODIX_VTG_MAX_UV);
	if (ret < 0)
		dev_err(&ts->client->dev,
			"Regulator vdd set_vtg failed ret=%d\n", ret);
	ret = regulator_disable(ts->vdd);
	if (ret)
		dev_err(&ts->client->dev,
			"Regulator vdd disable failed ret=%d\n", ret);
}

if (!IS_ERR(ts->avdd)) {
	ret = regulator_disable(ts->avdd);
	if (ret)
		dev_err(&ts->client->dev,
			"Regulator avdd disable failed ret=%d\n", ret);
}

return 0;
}
*/
/**
 * goodix_power_init - Initialize device power
 * @ts: driver private data
 *
 * Returns zero on success, else an error.
 */
/*
static int goodix_power_init(struct goodix_ts_data *ts)
{
int ret;

ts->avdd = regulator_get(&ts->client->dev, "avdd");
if (IS_ERR(ts->avdd)) {
	ret = PTR_ERR(ts->avdd);
	dev_info(&ts->client->dev,
		"Regulator get failed avdd ret=%d\n", ret);
}

ts->vdd = regulator_get(&ts->client->dev, "vdd");
if (IS_ERR(ts->vdd)) {
	ret = PTR_ERR(ts->vdd);
	dev_info(&ts->client->dev,
		"Regulator get failed vdd ret=%d\n", ret);
}

ts->vcc_i2c = regulator_get(&ts->client->dev, "vcc-i2c");
if (IS_ERR(ts->vcc_i2c)) {
	ret = PTR_ERR(ts->vcc_i2c);
	dev_info(&ts->client->dev,
		"Regulator get failed vcc_i2c ret=%d\n", ret);
}

return 0;
}
*/
/**
 * goodix_power_deinit - Deinitialize device power
 * @ts: driver private data
 *
 * Returns zero on success, else an error.
 *//*
static int goodix_power_deinit(struct goodix_ts_data *ts)
{
	regulator_put(ts->vdd);
	regulator_put(ts->vcc_i2c);
	regulator_put(ts->avdd);

	return 0;
}
*/
/*
static int goodix_ts_get_dt_coords(struct device *dev, char *name,
				struct goodix_ts_platform_data *pdata)
{
	struct property *prop;
	struct device_node *np = dev->of_node;
	int rc;
	u32 coords[GOODIX_COORDS_ARR_SIZE];

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	rc = of_property_read_u32_array(np, name, coords,
		GOODIX_COORDS_ARR_SIZE);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read %s\n", name);
		return rc;
	}

	if (!strcmp(name, "goodix,panel-coords")) {
		pdata->panel_minx = coords[0];
		pdata->panel_miny = coords[1];
		pdata->panel_maxx = coords[2];
		pdata->panel_maxy = coords[3];
	} else if (!strcmp(name, "goodix,display-coords")) {
		pdata->x_min = coords[0];
		pdata->y_min = coords[1];
		pdata->x_max = coords[2];
		pdata->y_max = coords[3];
	} else {
		dev_err(dev, "unsupported property %s\n", name);
		return -EINVAL;
	}

	return 0;
}
*/
/*
static int goodix_parse_dt(struct device *dev,
			struct goodix_ts_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	struct property *prop;
	u32 temp_val, num_buttons;
	u32 button_map[MAX_BUTTONS];

	rc = goodix_ts_get_dt_coords(dev, "goodix,panel-coords", pdata);
	if (rc && (rc != -EINVAL))
		return rc;

	rc = goodix_ts_get_dt_coords(dev, "goodix,display-coords", pdata);
	if (rc)
		return rc;

	pdata->i2c_pull_up = 1;

	pdata->no_force_update = of_property_read_bool(np,
						"goodix,no-force-update");

	pdata->reset_gpio = of_get_named_gpio_flags(np, "reset-gpios",
				0, &pdata->reset_gpio_flags);
	if (pdata->reset_gpio < 0)
		return pdata->reset_gpio;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "interrupt-gpios",
				0, &pdata->irq_gpio_flags);
	if (pdata->irq_gpio < 0)
		return pdata->irq_gpio;

	rc = of_property_read_u32(np, "goodix,family-id", &temp_val);
	if (!rc)
		pdata->family_id = temp_val;
	else
		return rc;

	prop = of_find_property(np, "goodix,button-map", NULL);
	if (prop) {
		num_buttons = prop->length / sizeof(temp_val);
		if (num_buttons > MAX_BUTTONS)
			return -EINVAL;

		rc = of_property_read_u32_array(np,
			"goodix,button-map", button_map,
			num_buttons);
		if (rc) {
			dev_err(dev, "Unable to read key codes\n");
			return rc;
		}
	}

	prop = of_find_property(np, "goodix,cfg-data", &pdata->gtp_cfg_len);
	if (prop && prop->value) {
		pdata->config_data = devm_kzalloc(dev,
			GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH, GFP_KERNEL);
		if (!pdata->config_data) {
			dev_err(dev, "Not enough memory for panel config data\n");
			return -ENOMEM;
		}

		pdata->config_data[0] = GTP_REG_CONFIG_DATA >> 8;
		pdata->config_data[1] = GTP_REG_CONFIG_DATA & 0xff;
		memset(&pdata->config_data[GTP_ADDR_LENGTH], 0,
					GTP_CONFIG_MAX_LENGTH);
		memcpy(&pdata->config_data[GTP_ADDR_LENGTH],
				prop->value, pdata->gtp_cfg_len);
	} else {
		dev_err(dev,
			"Unable to get configure data, default will be used.\n");
		pdata->gtp_cfg_len = 0;
	}

	return 0;
}
*/

#ifdef SUSPEND_ONLINE
static int lidbg_ts_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
	struct goodix_ts_data *ts =
        container_of(this, struct goodix_ts_data, fb_notif);
    DUMP_FUN;

    switch (event)
    {
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_ON):
		ts->gtp_is_suspend = 0;
		goodix_ts_resume(ts);
		break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, NOTIFIER_MINOR_ACC_OFF):
		ts->gtp_is_suspend = 1;
		goodix_ts_suspend(ts);
		break;
    default:
        break;
    }

    return NOTIFY_DONE;
}

static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_ts_event,
};
#endif

/*******************************************************
Function:
	I2c probe.
Input:
	client: i2c device struct.
	id: device id.
Output:
	Executive outcomes.
	0: succeed.
*******************************************************/

static int goodix_ts_probe(struct platform_device *pdev)
{
    //struct goodix_ts_platform_data *pdata;
    struct goodix_ts_data *ts;
    struct i2c_client *client;
    u16 version_info;
    int ret = 0;
    char ic_type[3];

    client = (struct i2c_client *)kzalloc( sizeof(struct i2c_client), GFP_KERNEL);
    if (!client)
    {
        dev_err(&client->dev, "GTP not enough memory for client\n");
        return -ENOMEM;
    }
    client->addr = 0x14;
    client->dev = pdev->dev;
	strcpy(client->name, "lidbg_ctp");

    lidbg("GTP I2C Address: 0x%02x\n", client->addr);
    /*if (client->dev.of_node) {
    	pdata = devm_kzalloc(&client->dev,
    		sizeof(struct goodix_ts_platform_data), GFP_KERNEL);
    	if (!pdata) {
    		dev_err(&client->dev,
    			"GTP Failed to allocate memory for pdata\n");
    		return -ENOMEM;
    	}

    	ret = goodix_parse_dt(&client->dev, pdata);
    	if (ret)
    		return ret;
    } else {
    	pdata = client->dev.platform_data;
    }

    if (!pdata) {
    	dev_err(&client->dev, "GTP invalid pdata\n");
    	return -EINVAL;
    }
    */
#if GTP_ESD_PROTECT
    i2c_connect_client = client;
#endif


    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (!ts)
    {
        dev_err(&client->dev, "GTP not enough memory for ts\n");
        return -ENOMEM;
    }

    memset(ts, 0, sizeof(*ts));
    ts->client = client;
    //ts->pdata = pdata;
    /* For 2.6.39 & later use spin_lock_init(&ts->irq_lock)
     * For 2.6.39 & before, use ts->irq_lock = SPIN_LOCK_UNLOCKED
     */
    spin_lock_init(&ts->irq_lock);
    i2c_set_clientdata(client, ts);
    ts->gtp_rawdiff_mode = 0;
    /*
    	ret = goodix_power_init(ts);
    	if (ret) {
    		dev_err(&client->dev, "GTP power init failed\n");
    		goto exit_free_client_data;
    	}

    	ret = goodix_power_on(ts);
    	if (ret) {
    		dev_err(&client->dev, "GTP power on failed\n");
    		goto exit_deinit_power;
    	}
    */

    gtp_reset_guitar(ts, 20);

    ret = gtp_i2c_test(client);
    if (ret != 2)
    {
        dev_err(&client->dev, "I2C communication ERROR!\n");
        goto exit_free_io_port;
    }

#if GTP_AUTO_UPDATE
    ret = gup_init_update_proc(ts);
    if (ret < 0)
    {
        dev_err(&client->dev,
                "GTP Create firmware update thread error.\n");
        goto exit_free_io_port;
    }
#endif
    ret = gtp_read_version(client, &version_info, ic_type);
    if (ret != 2)
    {
        dev_err(&client->dev, "Read version failed.\n");
        goto exit_free_irq;
    }
    ret = gtp_init_panel(ts, ic_type);
    if (ret < 0)
    {
        dev_err(&client->dev, "GTP init panel failed.\n");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
        ts->int_trigger_type = GTP_INT_TRIGGER;
    }

    ret = gtp_request_input_dev(ts);
    if (ret)
    {
        dev_err(&client->dev, "GTP request input dev failed.\n");
        goto exit_free_inputdev;
    }

#ifdef SUSPEND_ONLINE
	ts->fb_notif = lidbg_notifier;
	register_lidbg_notifier(&ts->fb_notif);
	if(0)
#endif
{
#if defined(CONFIG_FB)
    ts->fb_notif.notifier_call = fb_notifier_callback;
    ret = fb_register_client(&ts->fb_notif);
    if (ret)
        dev_err(&ts->client->dev,
                "Unable to register fb_notifier: %d\n",
                ret);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif
}


    ts->goodix_wq = create_singlethread_workqueue("goodix_wq");
    INIT_WORK(&ts->work, goodix_ts_work_func);

    ret = gtp_request_irq(ts);
    if (ret < 0)
        dev_info(&client->dev, "GTP works in polling mode.\n");
    else
        dev_info(&client->dev, "GTP works in interrupt mode.\n");
    if (ts->use_irq)
        gtp_irq_enable(ts);

#if GTP_CREATE_WR_NODE
    init_wr_node(client);
#endif

#if GTP_ESD_PROTECT
    gtp_esd_switch(client, SWITCH_ON);
#endif
    init_done = true;
    return 0;
exit_free_irq:
#if defined(CONFIG_FB)
    if (fb_unregister_client(&ts->fb_notif))
        dev_err(&client->dev,
                "Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&ts->early_suspend);
#endif
    if (ts->use_irq)
        free_irq(client->irq, ts);
    else
        hrtimer_cancel(&ts->timer);
    cancel_work_sync(&ts->work);
    flush_workqueue(ts->goodix_wq);
    destroy_workqueue(ts->goodix_wq);

    input_unregister_device(ts->input_dev);
    if (ts->input_dev)
    {
        input_free_device(ts->input_dev);
        ts->input_dev = NULL;
    }
exit_free_inputdev:
    kfree(ts->config_data);
exit_free_io_port:
    /*
    exit_power_off:
    	goodix_power_off(ts);
    exit_deinit_power:
    	goodix_power_deinit(ts);
    exit_free_client_data:
    	i2c_set_clientdata(client, NULL);
    	*/
    kfree(ts);
    return ret;
}

/*******************************************************
Function:
	Goodix touchscreen driver release function.
Input:
	client: i2c device struct.
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_remove(struct platform_device *pdev)
{
    return 0;
}

#ifdef SOC_mt3360
static int goodix_ac_ts_suspend(struct device *dev)
{
    struct goodix_ts_data *ts = dev_get_drvdata(dev);
    int ret = 0, i;
    int index = 0;
    struct mtk_ext_int ctp_int;

    index = scan_int_config(ts->client->irq);
    ctp_int = ctp_int_config[index];

    if (ts->gtp_is_suspend)
    {
        dev_dbg(&ts->client->dev, "Already in suspend state.\n");
        return 0;
    }

    //	mutex_lock(&ts->lock);
#if GTP_ESD_PROTECT
    gtp_esd_switch(ts->client, SWITCH_OFF);
#endif

#if GTP_SLIDE_WAKEUP
    ret = gtp_enter_doze(ts);
#else
    if (ts->use_irq)
    {
        ac83xx_mask_ack_bim_irq(ctp_int.vector_irq_num);
        gtp_irq_disable(ts);
    }
    else
        hrtimer_cancel(&ts->timer);

    //	for (i = 0; i < GTP_MAX_TOUCH; i++)
    //		gtp_touch_up(ts, i);

    //	input_sync(ts->input_dev);

    ret = gtp_enter_sleep(ts);
#endif
    if (ret < 0)
        dev_err(&ts->client->dev, "GTP early suspend failed.\n");
    /* to avoid waking up while not sleeping,
     * delay 48 + 10ms to ensure reliability
     */
    msleep(58);
    //	mutex_unlock(&ts->lock);
#ifdef SUSPEND_ONLINE
#else
    ts->gtp_is_suspend = 1;
#endif

    printk("[TP] goodix_ts_suspend ret=%d\n", ret);
    return ret;
}


static int goodix_ac_ts_resume_thread(void *data)
{
    int ret = -1;
    struct goodix_ts_data *ts = data;
    FUNCTION_IN;

    if (!ts->gtp_is_suspend)
    {
        dev_dbg(&ts->client->dev, "Already in awake state.\n");
        return 0;
    }

    //	mutex_lock(&ts->lock);
    ret = gtp_wakeup_sleep(ts);

#if GTP_SLIDE_WAKEUP
    doze_status = DOZE_DISABLED;
#endif

    if (ret <= 0)
        dev_err(&ts->client->dev, "GTP resume failed.\n");

    if(ret == GTP_ADDR_LENGTH)
        ret = 0;

    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
    else
    {
        hrtimer_start(&ts->timer,
                      ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#if GTP_ESD_PROTECT
    gtp_esd_switch(ts->client, SWITCH_ON);
#endif
    //	mutex_unlock(&ts->lock);

#ifdef SUSPEND_ONLINE
#else
    ts->gtp_is_suspend = 0;
#endif

    printk("[TP] goodix_ts_resume ret=%d\n", ret);
    return 0;
}



static int goodix_ac_ts_resume(struct device *dev)
{
    struct goodix_ts_data *ts = dev_get_drvdata(dev);
    CREATE_KTHREAD(goodix_ac_ts_resume_thread, (void *)ts);
    return 0;
}

static const struct dev_pm_ops goodix_ts_dev_pm_ops =
{
    .suspend = goodix_ac_ts_suspend,
    .resume = goodix_ac_ts_resume,
};
#else

static int goodix_ac_ts_suspend(struct device *dev)
{
    SOC_IO_Output(0, GTP_RST_PORT, !GTP_RST_PORT_ACTIVE);
    return 0;
}

static int goodix_ac_ts_resume(struct device *dev)
{
    return 0;
}

static const struct dev_pm_ops goodix_ts_dev_pm_ops =
{
    .suspend = goodix_ac_ts_suspend,
    .resume = goodix_ac_ts_resume,
};
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_FB)
/*******************************************************
Function:
	Early suspend function.
Input:
	h: early_suspend struct.
Output:
	None.
*******************************************************/
static void goodix_ts_suspend(struct goodix_ts_data *ts)
{
    int ret = -1, i;

    GTP_DEBUG_FUNC();

#if GTP_ESD_PROTECT

#ifdef SUSPEND_ONLINE
#else
    ts->gtp_is_suspend = 1;
#endif

    gtp_esd_switch(ts->client, SWITCH_OFF);
#endif

#if GTP_SLIDE_WAKEUP
    ret = gtp_enter_doze(ts);
#else
    if (ts->use_irq)
        gtp_irq_disable(ts);
    else
        hrtimer_cancel(&ts->timer);

    for (i = 0; i < GTP_MAX_TOUCH; i++)
        gtp_touch_up(ts, i);

    input_sync(ts->input_dev);

    ret = gtp_enter_sleep(ts);
#endif
    if (ret < 0)
        dev_err(&ts->client->dev, "GTP early suspend failed.\n");
    /* to avoid waking up while not sleeping,
     * delay 48 + 10ms to ensure reliability
     */
    msleep(58);
    SOC_IO_Output(0, GTP_RST_PORT, !GTP_RST_PORT_ACTIVE);
    lidbg( "gt911 GTP_RST_PORT.disable\n");
}

/*******************************************************
Function:
	Late resume function.
Input:
	h: early_suspend struct.
Output:
	None.
*******************************************************/


static int goodix_ts_resume_thread(void *data)
{
    int ret = -1;
    struct goodix_ts_data *ts = data;
    FUNCTION_IN;
    lidbg( "gt911 GTP_RST_PORT.enable\n");
    SOC_IO_Output(0, GTP_RST_PORT, GTP_RST_PORT_ACTIVE);
    msleep(100);
    ret = gtp_wakeup_sleep(ts);

#if GTP_SLIDE_WAKEUP
    doze_status = DOZE_DISABLED;
#endif

    if (ret < 0)
        dev_err(&ts->client->dev, "GTP resume failed.\n");

    if (ts->use_irq)
        gtp_irq_enable(ts);
    else
        hrtimer_start(&ts->timer,
                      ktime_set(1, 0), HRTIMER_MODE_REL);

#if GTP_ESD_PROTECT

#ifdef SUSPEND_ONLINE
#else
    ts->gtp_is_suspend = 0;
#endif

    gtp_esd_switch(ts->client, SWITCH_ON);
#endif
    return 1;

}
static void goodix_ts_resume(struct goodix_ts_data *ts)
{
    GTP_DEBUG_FUNC();
    CREATE_KTHREAD(goodix_ts_resume_thread, (void *)ts);
}

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
                                unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank;
    struct goodix_ts_data *ts =
        container_of(self, struct goodix_ts_data, fb_notif);

    if (evdata && evdata->data && event == FB_EVENT_BLANK &&
            ts && ts->client)
    {
        blank = evdata->data;

        if (*blank == FB_BLANK_UNBLANK)
            goodix_ts_resume(ts);
        else if (*blank == FB_BLANK_POWERDOWN)
            goodix_ts_suspend(ts);
    }

    return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
/*******************************************************
Function:
	Early suspend function.
Input:
	h: early_suspend struct.
Output:
	None.
*******************************************************/
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;

    ts = container_of(h, struct goodix_ts_data, early_suspend);
    goodix_ts_suspend(ts);
    return;
}

/*******************************************************
Function:
	Late resume function.
Input:
	h: early_suspend struct.
Output:
	None.
*******************************************************/
static void goodix_ts_late_resume(struct early_suspend *h)
{
    struct goodix_ts_data *ts;

    ts = container_of(h, struct goodix_ts_data, early_suspend);
    goodix_ts_late_resume(ts);
    return;
}
#endif
#endif /* !CONFIG_HAS_EARLYSUSPEND && !CONFIG_FB*/

#if GTP_ESD_PROTECT
/*******************************************************
Function:
	switch on & off esd delayed work
Input:
	client:  i2c device
	on:	SWITCH_ON / SWITCH_OFF
Output:
	void
*********************************************************/
void gtp_esd_switch(struct i2c_client *client, int on)
{
    struct goodix_ts_data *ts;

    ts = i2c_get_clientdata(client);
    if (SWITCH_ON == on)
    {
        /* switch on esd  */
        if (!ts->esd_running)
        {
            ts->esd_running = 1;
            dev_dbg(&client->dev, "Esd started\n");
            queue_delayed_work(gtp_esd_check_workqueue,
                               &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
        }
    }
    else
    {
        /* switch off esd */
        if (ts->esd_running)
        {
            ts->esd_running = 0;
            dev_dbg(&client->dev, "Esd cancelled\n");
            cancel_delayed_work_sync(&gtp_esd_check_work);
        }
    }
}

/*******************************************************
Function:
	Initialize external watchdog for esd protect
Input:
	client:  i2c device.
Output:
	result of i2c write operation.
		1: succeed, otherwise: failed
*********************************************************/
static int gtp_init_ext_watchdog(struct i2c_client *client)
{
    /* in case of recursively reset by calling gtp_i2c_write*/
    u8 opr_buffer[4] = {0x80, 0x40, 0xAA, 0xAA};
    int ret;
    int retries = 0;

    GTP_DEBUG("Init external watchdog...");
    GTP_DEBUG_FUNC();

    while (retries < 5)
    {
        ret = SOC_I2C_Send(TS_I2C_BUS, client->addr, opr_buffer, 4);
        if (ret == 1)
            return 1;
        retries++;
        msleep(300);
    }
    if (retries >= 5)
        dev_err(&client->dev, "init external watchdog failed!");
    return 0;
}

/*******************************************************
Function:
	Esd protect function.
	Added external watchdog by meta, 2013/03/07
Input:
	work: delayed work
Output:
	None.
*******************************************************/
static void gtp_esd_check_func(struct work_struct *work)
{
    s32 i;
    s32 ret = -1;
    struct goodix_ts_data *ts = NULL;
    u8 test[4] = {0x80, 0x40};

    GTP_DEBUG_FUNC();

    ts = i2c_get_clientdata(i2c_connect_client);

    if (ts->gtp_is_suspend)
    {
        dev_dbg(&ts->client->dev, "Esd terminated!\n");
        ts->esd_running = 0;
        return;
    }
#ifdef CONFIG_GT9XX_TOUCHPANEL_UPDATE
    if (ts->enter_update)
        return;
#endif

    for (i = 0; i < 3; i++)
    {
        ret = gtp_i2c_read(ts->client, test, 4);

        GTP_DEBUG("0x8040 = 0x%02X, 0x8041 = 0x%02X", test[2], test[3]);
        if ((ret < 0))
        {
            /* IC works abnormally..*/
            continue;
        }
        else
        {
            if ((test[2] == 0xAA) || (test[3] != 0xAA))
            {
                /* IC works abnormally..*/
                i = 3;
                break;
            }
            else
            {
                /* IC works normally, Write 0x8040 0xAA*/
                test[2] = 0xAA;
                gtp_i2c_write(ts->client, test, 3);
                break;
            }
        }
    }
    if (i >= 3)
    {
        dev_err(&ts->client->dev,
                "IC Working ABNORMALLY, Resetting Guitar...\n");
        gtp_reset_guitar(ts, 50);
    }

    if (!ts->gtp_is_suspend)
        queue_delayed_work(gtp_esd_check_workqueue,
                           &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
    else
    {
        dev_dbg(&ts->client->dev, "Esd terminated!\n");
        ts->esd_running = 0;
    }

    return;
}
#endif

static struct of_device_id goodix_match_table[] =
{
    { .compatible = "goodix,gt9xx", },
    { },
};

static struct platform_driver goodix_ts_driver =
{
    .probe      = goodix_ts_probe,
    .remove     = goodix_ts_remove,
    .driver = {
        .name     = GTP_I2C_NAME,
        .owner    = THIS_MODULE,
        .of_match_table = goodix_match_table,
        .pm = &goodix_ts_dev_pm_ops,
    },
};

////////////////////////////////////////////////////
#define feature_ts_nod

#ifdef feature_ts_nod
#define TS_DEVICE_NAME "tsnod"
static int major_number_ts = 0;
static struct class *class_install_ts;

struct ts_device
{
    unsigned int counter;
    struct cdev cdev;
};
struct ts_device *tsdev;

int ts_nod_open (struct inode *inode, struct file *filp)
{
    //do nothing
    filp->private_data = tsdev;
    lidbg("[futengfei]==================ts_nod_open\n");

    return 0;          /* success */
}

ssize_t ts_nod_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    char data_rec[20];
    //    struct ts_device *tsdev = filp->private_data;

    if (copy_from_user( data_rec, buf, count))
    {
        lidbg("copy_from_user ERR\n");
    }
    data_rec[count] =  '\0';
    lidbg("gt910-ts_nod_write:==%zd====[%s]\n", count, data_rec);
    // processing data
    if(!(strnicmp(data_rec, "TSMODE_XYREVERT", count - 1)))
    {
        xy_revert_en = 1;
        lidbg("[gt910]ts_nod_write:==========TSMODE_XYREVERT\n");
    }
    else if(!(strnicmp(data_rec, "TSMODE_NORMAL", count - 1)))
    {
        xy_revert_en = 0;
        lidbg("[gt910]ts_nod_write:==========TSMODE_NORMAL\n");
    }

    return count;
}
static  struct file_operations ts_nod_fops =
{
    .owner = THIS_MODULE,
    .write = ts_nod_write,
    .open = ts_nod_open,
};

static int init_cdev_ts(void)
{
    int ret, err, result;
    dev_t dev_number = MKDEV(major_number_ts, 0);

    //11creat cdev
    tsdev = (struct ts_device *)kmalloc( sizeof(struct ts_device), GFP_KERNEL );
    if (tsdev == NULL)
    {
        ret = -ENOMEM;
        lidbg("gt911===========init_cdev_ts:kmalloc err \n");
        return ret;
    }

    if(major_number_ts)
    {
        result = register_chrdev_region(dev_number, 1, TS_DEVICE_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&dev_number, 0, 1, TS_DEVICE_NAME);
        major_number_ts = MAJOR(dev_number);
    }
    lidbg("gt911===========alloc_chrdev_region result:%d \n", result);

    cdev_init(&tsdev->cdev, &ts_nod_fops);
    tsdev->cdev.owner = THIS_MODULE;
    tsdev->cdev.ops = &ts_nod_fops;
    err = cdev_add(&tsdev->cdev, dev_number, 1);
    if (err)
        lidbg( "gt911===========Error cdev_add\n");

    //cread cdev node in /dev
    class_install_ts = class_create(THIS_MODULE, "tsnodclass");
    if(IS_ERR(class_install_ts))
    {
        lidbg( "gt911=======class_create err\n");
        return -1;
    }
    device_create(class_install_ts, NULL, dev_number, NULL, "%s%d", TS_DEVICE_NAME, 0);
    return 0;
}
#endif
///////////////////////////////////////////////////////
/*******************************************************
Function:
    Driver Install function.
Input:
    None.
Output:
    Executive Outcomes. 0---succeed.
********************************************************/

static struct platform_device goodix_ts_devices =
{
    .name			= GTP_I2C_NAME,
    .id 			= 0,
};

static int goodix_ts_init(void)
{
    int ret;

    LIDBG_GET;
    is_ts_load = 1;
    GTP_DEBUG_FUNC();
#if GTP_ESD_PROTECT
    INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
#endif
    ret = platform_device_register(&goodix_ts_devices);
    ret = platform_driver_register(&goodix_ts_driver);
    init_cdev_ts();
    return ret;
}

/*******************************************************
Function:
	Driver uninstall function.
Input:
	None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");
