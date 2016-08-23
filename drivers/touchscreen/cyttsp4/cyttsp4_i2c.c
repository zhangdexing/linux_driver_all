/*
 * cyttsp4_i2c.c
 * Cypress TrueTouch(TM) Standard Product V4 I2C Module.
 * For use with Cypress touchscreen controllers.
 * Supported parts include:
 * CY8CTMA46X
 * CY8CTMA1036/768
 * CYTMA445
 *
 * Copyright (C) 2012-2015 Cypress Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include "cyttsp4_regs.h"
#include "cyttsp4_platform.h"
#include "lidbg.h"
LIDBG_DEFINE;


#define CY_I2C_DATA_SIZE  (2 * 256)
#define CYTTSP4_SLAVE_ADDRESS 0x24
#define CY_VKEYS_X 720
#define CY_VKEYS_Y 1280
#define CY_MAXX 880
#define CY_MAXY 1280
#define CY_MINX 0
#define CY_MINY 0


#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_W 255
#define CY_PROXIMITY_MIN_VAL	0
#define CY_PROXIMITY_MAX_VAL	1

#define CY_ABS_MIN_T 0
#define CY_ABS_MAX_T 15

#define CYTTSP4_I2C_TCH_ADR 0x24
#define CYTTSP4_LDR_TCH_ADR 0x24
#define CYTTSP4_I2C_IRQ_GPIO 38 /* J6.9, C19, GPMC_AD14/GPIO_38 */
#define CYTTSP4_I2C_RST_GPIO 37 /* J6.10, D18, GPMC_AD13/GPIO_37 */

static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_HOMEPAGE,		/* 172 */ /* Previously was KEY_HOME (102) */
				/* New Android versions use KEY_HOMEPAGE */
	KEY_MENU,		/* 139 */
	KEY_BACK,		/* 158 */
	KEY_SEARCH,		/* 217 */
	KEY_VOLUMEDOWN,		/* 114 */
	KEY_VOLUMEUP,		/* 115 */
	KEY_CAMERA,		/* 212 */
	KEY_POWER		/* 116 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};


static struct cyttsp4_core_platform_data _cyttsp4_core_platform_data = {
	.irq_gpio = -1,
	.rst_gpio = -1,
	.xres     = cyttsp4_xres,
	.init     = cyttsp4_init,
	.power    = cyttsp4_power,
	.detect   = cyttsp4_detect,
	.irq_stat = cyttsp4_irq_stat,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL,	/* &cyttsp4_sett_param_regs, */
		NULL,	/* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
	.flags = CY_CORE_FLAG_WAKE_ON_GESTURE,
	.easy_wakeup_gesture = CY_CORE_EWG_TAP_TAP | CY_CORE_EWG_TWO_FINGER_SLIDE,
};

static const int16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
	ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0,
	ABS_MT_TOUCH_MINOR, 0, 255, 0, 0,
	ABS_MT_ORIENTATION, -127, 127, 0, 0,
	ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0,
	ABS_DISTANCE, 0, 255, 0, 0,	/* Used with hover */
};

struct touch_framework cyttsp4_framework = {
	.abs = cyttsp4_abs,
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static struct cyttsp4_mt_platform_data _cyttsp4_mt_platform_data = {
	.frmwrk = &cyttsp4_framework,
	//.flags = CY_MT_FLAG_FLIP | CY_MT_FLAG_INV_X | CY_MT_FLAG_INV_Y,
	.flags =  CY_MT_FLAG_INV_X,
	.inp_dev_name = CYTTSP4_MT_NAME,
	.vkeys_x = CY_VKEYS_X,
	.vkeys_y = CY_VKEYS_Y,
};

static struct cyttsp4_btn_platform_data _cyttsp4_btn_platform_data = {
	.inp_dev_name = CYTTSP4_BTN_NAME,
};

static const int16_t cyttsp4_prox_abs[] = {
	ABS_DISTANCE, CY_PROXIMITY_MIN_VAL, CY_PROXIMITY_MAX_VAL, 0, 0,
};

struct touch_framework cyttsp4_prox_framework = {
	.abs          = cyttsp4_prox_abs,
	.size         = ARRAY_SIZE(cyttsp4_prox_abs),
};

static struct cyttsp4_proximity_platform_data _cyttsp4_proximity_platform_data = {
	.frmwrk        = &cyttsp4_prox_framework,
	.inp_dev_name  = CYTTSP4_PROXIMITY_NAME,
};

static struct cyttsp4_platform_data _cyttsp4_platform_data = {
	.core_pdata   = &_cyttsp4_core_platform_data,
	.mt_pdata     = &_cyttsp4_mt_platform_data,
	.btn_pdata    = &_cyttsp4_btn_platform_data,
	.prox_pdata   = &_cyttsp4_proximity_platform_data,
	.loader_pdata = &_cyttsp4_loader_platform_data,
};


static struct i2c_board_info _cyttsp4_device = {
    I2C_BOARD_INFO(CYTTSP4_I2C_NAME, CYTTSP4_SLAVE_ADDRESS),
    .platform_data = &_cyttsp4_platform_data,
};

static int cyttsp4_i2c_read_block_data(struct device *dev, u16 addr,
	int length, void *values, int max_xfer)
{
	struct i2c_client *client = to_i2c_client(dev);
	int trans_len;
	u16 slave_addr = client->addr;
	u8 client_addr;
	u8 addr_lo;
	struct i2c_msg msgs[2];
	int rc = -EINVAL;
	int msg_cnt = 0;

	while (length > 0) {
		client_addr = slave_addr | ((addr >> 8) & 0x1);
		addr_lo = addr & 0xFF;
		trans_len = min(length, max_xfer);
#if 0
		msg_cnt = 0;
		memset(msgs, 0, sizeof(msgs));
		msgs[msg_cnt].addr = client_addr;
		msgs[msg_cnt].flags = 0;
		msgs[msg_cnt].len = 1;
		msgs[msg_cnt].buf = &addr_lo;
		msg_cnt++;

		msgs[msg_cnt].addr = client_addr;
		msgs[msg_cnt].flags = I2C_M_RD;
		msgs[msg_cnt].len = trans_len;
		msgs[msg_cnt].buf = values;
		msg_cnt++;

		rc = i2c_transfer(client->adapter, msgs, msg_cnt);
#endif
		rc = SOC_I2C_Rec(TS_I2C_BUS,client_addr,addr_lo,values,trans_len);
		if (rc != 2)
			goto exit;

		length -= trans_len;
		values += trans_len;
		addr += trans_len;
	}

exit:
	return (rc < 0) ? rc : rc != msg_cnt ? -EIO : 0;
}

static int cyttsp4_i2c_write_block_data(struct device *dev, u16 addr,
	u8 *wr_buf, int length, const void *values, int max_xfer)
{
	struct i2c_client *client = to_i2c_client(dev);
	u16 slave_addr = client->addr;
	u8 client_addr;
	u8 addr_lo;
	int trans_len;
	struct i2c_msg msg;
	int rc = -EINVAL;

	while (length > 0) {
		client_addr = slave_addr | ((addr >> 8) & 0x1);
		addr_lo = addr & 0xFF;
		trans_len = min(length, max_xfer);
#if 0
		memset(&msg, 0, sizeof(msg));
		msg.addr = client_addr;
		msg.flags = 0;
		msg.len = trans_len + 1;
		msg.buf = wr_buf;
#endif
		wr_buf[0] = addr_lo;
		memcpy(&wr_buf[1], values, trans_len);
#if 0
		/* write data */
		rc = i2c_transfer(client->adapter, &msg, 1);
#endif
		rc = SOC_I2C_Send(TS_I2C_BUS,client_addr,wr_buf,trans_len+1);
		if (rc != 1)
			goto exit;

		length -= trans_len;
		values += trans_len;
		addr += trans_len;
	}

exit:
	return (rc < 0) ? rc : rc != 1 ? -EIO : 0;
}

static int cyttsp4_i2c_write(struct device *dev, u16 addr, u8 *wr_buf,
	const void *buf, int size, int max_xfer)
{
	int rc;

	pm_runtime_get_noresume(dev);
	rc = cyttsp4_i2c_write_block_data(dev, addr, wr_buf, size, buf,
		max_xfer);
	pm_runtime_put_noidle(dev);

	return rc;
}

static int cyttsp4_i2c_read(struct device *dev, u16 addr, void *buf, int size,
	int max_xfer)
{
	int rc;

	pm_runtime_get_noresume(dev);
	rc = cyttsp4_i2c_read_block_data(dev, addr, size, buf, max_xfer);
	pm_runtime_put_noidle(dev);

	return rc;
}

static struct cyttsp4_bus_ops cyttsp4_i2c_bus_ops = {
	.write = cyttsp4_i2c_write,
	.read = cyttsp4_i2c_read,
};

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
static struct of_device_id cyttsp4_i2c_of_match[] = {
	{ .compatible = "cy,cyttsp4_i2c_adapter", }, { }
};
MODULE_DEVICE_TABLE(of, cyttsp4_i2c_of_match);
#endif

static int cyttsp4_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *i2c_id)
{
	struct device *dev = &client->dev;
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
	const struct of_device_id *match;
#endif
	int rc;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(dev, "I2C functionality not Supported\n");
		return -EIO;
	}

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
	match = of_match_device(of_match_ptr(cyttsp4_i2c_of_match), dev);
	if (match) {
		rc = cyttsp4_devtree_create_and_get_pdata(dev);
		if (rc < 0)
			return rc;
	}
#endif

	rc = cyttsp4_probe(&cyttsp4_i2c_bus_ops, &client->dev, client->irq,
			CY_I2C_DATA_SIZE);

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
	if (rc && match)
		cyttsp4_devtree_clean_pdata(dev);
#endif

	return rc;
}

static int cyttsp4_i2c_remove(struct i2c_client *client)
{
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
	struct device *dev = &client->dev;
	const struct of_device_id *match;
#endif
	struct cyttsp4_core_data *cd = i2c_get_clientdata(client);

	cyttsp4_release(cd);

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
	match = of_match_device(of_match_ptr(cyttsp4_i2c_of_match), dev);
	if (match)
		cyttsp4_devtree_clean_pdata(dev);
#endif

	return 0;
}

static const struct i2c_device_id cyttsp4_i2c_id[] = {
	{ CYTTSP4_I2C_NAME, 0 },  { }
};
MODULE_DEVICE_TABLE(i2c, cyttsp4_i2c_id);

static struct i2c_driver cyttsp4_i2c_driver = {
	.driver = {
		.name = CYTTSP4_I2C_NAME,
		.owner = THIS_MODULE,
		.pm = &cyttsp4_pm_ops,
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
		.of_match_table = cyttsp4_i2c_of_match,
#endif
	},
	.probe = cyttsp4_i2c_probe,
	.remove = cyttsp4_i2c_remove,
	.id_table = cyttsp4_i2c_id,
};

static int __init cyttsp4_i2c_init(void)
{
	DUMP_BUILD_TIME;
	LIDBG_GET;
	struct i2c_adapter *adap;
	struct i2c_client *client;
	adap = i2c_get_adapter(TS_I2C_BUS);
	if (!adap) {
		lidbg("cyttsp4 get i2c adapter %d\n",TS_I2C_BUS);
 		return -ENODEV;
	} else {
		lidbg("cyttsp4 get i2c adapter %d ok\n", TS_I2C_BUS);
 		client = i2c_new_device(adap, &_cyttsp4_device);
	}
	if (!client) {
		lidbg("cyttsp4 get i2c client %s @ 0x%02x fail!\n", _cyttsp4_device.type,
                _cyttsp4_device.addr);
		return -ENODEV;
	} else {
		lidbg("cyttsp4 get i2c client ok!\n");
	}
	i2c_put_adapter(adap);
	int rc = i2c_add_driver(&cyttsp4_i2c_driver);

	pr_info("%s: Cypress TTSP I2C Driver (Built %s) rc=%d\n",
		 __func__, CY_DRIVER_VERSION, rc);

	return rc;
}


static void __exit cyttsp4_i2c_exit(void)
{
	i2c_del_driver(&cyttsp4_i2c_driver);
}
module_init(cyttsp4_i2c_init);
module_exit(cyttsp4_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard Product I2C driver");
MODULE_AUTHOR("Cypress Semiconductor <ttdrivers@cypress.com>");
