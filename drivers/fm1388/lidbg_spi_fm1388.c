/*
 * fm1388-spi.c  --  FM1388 audio driver
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_qos.h>
#include <linux/sysfs.h>
#include <linux/clk.h>
//#include "fm1388.h"
#include "spi-fm1388.h"
#include "lidbg.h"
#define TAG "dfm1388_spi:"


LIDBG_DEFINE;
#define FM1388_SPI_BUS 0



static struct spi_device *fm1388_spi;

int fm1388_spi_read(unsigned int addr, unsigned int *val, size_t len)
{
	struct spi_device *spi = fm1388_spi;
	struct spi_message message;
	struct spi_transfer x[1];
	int status;
	u8 write_buf[13];
	u8 read_buf[13];

	write_buf[0] =
		(len == 4) ? FM1388_SPI_CMD_32_READ : FM1388_SPI_CMD_16_READ;
	write_buf[1] = (addr & 0xff000000) >> 24;
	write_buf[2] = (addr & 0x00ff0000) >> 16;
	write_buf[3] = (addr & 0x0000ff00) >> 8;
	write_buf[4] = (addr & 0x000000ff) >> 0;

	spi_message_init(&message);
	memset(x, 0, sizeof(x));
#if 0
	x[0].len = 5;
	x[0].tx_buf = write_buf;
	spi_message_add_tail(&x[0], &message);

	x[1].len = 4;
	x[1].tx_buf = write_buf;
	spi_message_add_tail(&x[1], &message);

	x[2].len = len;
	x[2].rx_buf = read_buf;
	spi_message_add_tail(&x[2], &message);
#endif
#if 1
	x[0].len = 9+len;
	x[0].tx_buf = write_buf;
	x[0].rx_buf = read_buf;
	spi_message_add_tail(&x[0], &message);
#endif
	status = spi_sync(spi, &message);

	if (len == 4)
		*val = read_buf[12] | read_buf[11] << 8 | read_buf[10] << 16 |
			read_buf[9] << 24;
	else
		*val = read_buf[1] | read_buf[0] << 8;

	lidbg(TAG"%s status:%d\n", __FUNCTION__, status);
	return status;
}
EXPORT_SYMBOL(fm1388_spi_read);

int fm1388_spi_write(unsigned int addr, unsigned int val, size_t len)
{
	struct spi_device *spi = fm1388_spi;
	int status;
	u8 write_buf[10];
	//lidbg(TAG"%s begin write: addr:%d,val:%d,len:%d\n", __FUNCTION__, addr, val, len);

	write_buf[1] = (addr & 0xff000000) >> 24;
	write_buf[2] = (addr & 0x00ff0000) >> 16;
	write_buf[3] = (addr & 0x0000ff00) >> 8;
	write_buf[4] = (addr & 0x000000ff) >> 0;

	if (len == 4) {
		write_buf[0] = FM1388_SPI_CMD_32_WRITE;
		write_buf[5] = (val & 0xff000000) >> 24;
		write_buf[6] = (val & 0x00ff0000) >> 16;
		write_buf[7] = (val & 0x0000ff00) >> 8;
		write_buf[8] = (val & 0x000000ff) >> 0;
	} else {
		write_buf[0] = FM1388_SPI_CMD_16_WRITE;
		write_buf[5] = (val & 0x0000ff00) >> 8;
		write_buf[6] = (val & 0x000000ff) >> 0;
	}

	status = spi_write(fm1388_spi, write_buf,
		(len == 4) ? sizeof(write_buf) : sizeof(write_buf) - 2);

	if (status)
		lidbg(TAG"%s error %d\n", __FUNCTION__, status);
	//lidbg(TAG"%s over write: addr:%d,val:%d,len:%d\n", __FUNCTION__, addr, val, len);
	return status;
}
EXPORT_SYMBOL_GPL(fm1388_spi_write);

/**
 * fm1388_spi_burst_read - Read data from SPI by fm1388 dsp memory address.
 * @addr: Start address.
 * @rxbuf: Data Buffer for reading.
 * @len: Data length, it must be a multiple of 8.
 *
 *
 * Returns true for success.
 */
int fm1388_spi_burst_read(unsigned int addr, u8 *rxbuf, size_t len)
{
	u8 spi_cmd = FM1388_SPI_CMD_BURST_READ;
	u8 write_buf[512];
	int status;
	unsigned int i, end, offset = 0;

	struct spi_message message;
	struct spi_transfer x[1];

		write_buf[0] = spi_cmd;
		write_buf[1] = ((addr + offset) & 0xff000000) >> 24;
		write_buf[2] = ((addr + offset) & 0x00ff0000) >> 16;
		write_buf[3] = ((addr + offset) & 0x0000ff00) >> 8;
		write_buf[4] = ((addr + offset) & 0x000000ff) >> 0;

		spi_message_init(&message);
		memset(x, 0, sizeof(x));
#if 0
		x[0].len = 5;
		x[0].tx_buf = write_buf;
		spi_message_add_tail(&x[0], &message);

		x[1].len = 4;
		x[1].tx_buf = write_buf;
		spi_message_add_tail(&x[1], &message);

		x[2].len = len;
		x[2].rx_buf = rxbuf + offset;
		spi_message_add_tail(&x[2], &message);
#endif
		x[0].len = 9+len;
		x[0].tx_buf = write_buf;
		x[0].rx_buf = rxbuf + offset;
		spi_message_add_tail(&x[0], &message);
		status = spi_sync(fm1388_spi, &message);

		if (status)
			return false;

	for (i = 0; i < len; i += 8) {
		write_buf[0] = rxbuf[i + 0+9];
		write_buf[1] = rxbuf[i + 1+9];
		write_buf[2] = rxbuf[i + 2+9];
		write_buf[3] = rxbuf[i + 3+9];
		write_buf[4] = rxbuf[i + 4+9];
		write_buf[5] = rxbuf[i + 5+9];
		write_buf[6] = rxbuf[i + 6+9];
		write_buf[7] = rxbuf[i + 7+9];

		rxbuf[i + 0+9] = write_buf[7];
		rxbuf[i + 1+9] = write_buf[6];
		rxbuf[i + 2+9] = write_buf[5];
		rxbuf[i + 3+9] = write_buf[4];
		rxbuf[i + 4+9] = write_buf[3];
		rxbuf[i + 5+9] = write_buf[2];
		rxbuf[i + 6+9] = write_buf[1];
		rxbuf[i + 7+9] = write_buf[0];
	}

	return true;
}
EXPORT_SYMBOL_GPL(fm1388_spi_burst_read);

/**
 * fm1388_spi_burst_write - Write data to SPI by fm1388 dsp memory address.
 * @addr: Start address.
 * @txbuf: Data Buffer for writng.
 * @len: Data length, it must be a multiple of 8.
 *
 *
 * Returns true for success.
 */

int fm1388_spi_burst_write(u32 addr, const u8 *txbuf, size_t len)
{

	u8 spi_cmd = FM1388_SPI_CMD_BURST_WRITE;
	u8 *write_buf;
	unsigned int i , end, offset = 0;
	int status;

	write_buf = kmalloc(FM1388_SPI_BUF_LEN + 6, GFP_KERNEL);

	if (write_buf == NULL)
		return -ENOMEM;

	while (offset < len) {

		if (offset + FM1388_SPI_BUF_LEN <= len)
			end = FM1388_SPI_BUF_LEN;
		else
			end = len % FM1388_SPI_BUF_LEN;

		write_buf[0] = spi_cmd;
		write_buf[1] = ((addr + offset) & 0xff000000) >> 24;
		write_buf[2] = ((addr + offset) & 0x00ff0000) >> 16;
		write_buf[3] = ((addr + offset) & 0x0000ff00) >> 8;
		write_buf[4] = ((addr + offset) & 0x000000ff) >> 0;

		for (i = 0; i < end; i += 8) {
			write_buf[i + 12] = txbuf[offset + i + 0];
			write_buf[i + 11] = txbuf[offset + i + 1];
			write_buf[i + 10] = txbuf[offset + i + 2];
			write_buf[i +  9] = txbuf[offset + i + 3];
			write_buf[i +  8] = txbuf[offset + i + 4];
			write_buf[i +  7] = txbuf[offset + i + 5];
			write_buf[i +  6] = txbuf[offset + i + 6];
			write_buf[i +  5] = txbuf[offset + i + 7];
		}
		

		write_buf[end + 5] = spi_cmd;

		//lidbg(TAG"%s: spi_write.offset(%d).len(%d)\n", __func__, offset, len);
		status = spi_write(fm1388_spi, write_buf, end + 6);
		if (status) {
			lidbg(TAG"%s error %d\n", __FUNCTION__,
				status);
			kfree(write_buf);
			return status;
		}

		offset += FM1388_SPI_BUF_LEN;
	}

	kfree(write_buf);

	return 0;

}

EXPORT_SYMBOL_GPL(fm1388_spi_burst_write);

//add by flyaudio
void spi_test(void)
{
#if 0
	unsigned int address=0x50000000;
	unsigned int write_value=0x3795af28;
	unsigned int read_value;
	unsigned int len=256*100;
	unsigned int i=0,j=0;
	unsigned char *read_buf;
	int ret1,ret2;

	read_buf=kmalloc(len,GFP_KERNEL);
	if(!read_buf)
		lidbg(TAG"alloc buf for fm1388 burst read err!");
	ret1=fm1388_spi_write(address,write_value,4);
	ret2=fm1388_spi_read(address,&read_value,4);
	
	lidbg(TAG"write_addr:%x,write_value=0x%x,read_value=0x%0x,writeRet:%d,readRet:%d\n",address, write_value,read_value,ret1,ret2);

	
	for(j=0;j<len/0x100;j++)
	{
		lidbg(TAG"-----------0x%x\n",0x100*j);
		fm1388_spi_burst_read(address+0x100*j,read_buf,0x100);
		for(i=0;i<0x100;)
		lidbg(TAG"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x---0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
		read_buf[9+i++],read_buf[9+i++],read_buf[9+i++],read_buf[9+i++],
		read_buf[9+i++],read_buf[9+i++],read_buf[9+i++],read_buf[9+i++],
		read_buf[9+i++],read_buf[9+i++],read_buf[9+i++],read_buf[9+i++],
		read_buf[9+i++],read_buf[9+i++],read_buf[9+i++],read_buf[9+i++]);
		msleep(20);
	}
#endif
}
EXPORT_SYMBOL_GPL(spi_test);

void register_fm1388_spi_device(void)
{
	int status;
	struct spi_master *master;
	struct spi_device *spi;
	struct spi_board_info chip =
    {
        .modalias	= "fm1388_spi",
        .mode       = 0x00,
        .bus_num	= 0,
        .chip_select = 0,
	//	.max_speed_hz = 2000000,
    };
    lidbg(TAG"%s:%d\n",__func__,FM1388_SPI_BUS);
    master = spi_busnum_to_master(FM1388_SPI_BUS);
    if (!master)
    {
        status = -ENODEV;
        lidbg(TAG"%s:spi_busnum_to_master error\n",__func__);
        goto error_busnum;
    }
    spi = spi_new_device(master, &chip);
    if (!spi)
    {
        status = -EBUSY;
        lidbg(TAG"%s:spi_new_device error\n",__func__);
        goto error_mem;
    }
	return ;

error_mem:
error_busnum:
	lidbg(TAG"register fm1388 spi device err!\n");
    return ;

}
void fm1388_spi_device_reload(void)
{
	if (fm1388_spi)
	{
		fm1388_spi->master->setup(fm1388_spi);
	}
	lidbg(TAG"fm1388_spi_device_reload\n");
}
EXPORT_SYMBOL_GPL(fm1388_spi_device_reload);

#if 1
//Henry add for try
static const struct spi_device_id fm1388_spi_id[] = {
	{ "fm1388_spi", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, fm1388_spi_id);
//End
#endif
static const struct of_device_id fm1388_dt_ids[] = {
	{ .compatible = "fm,fm1388",},
	{},
};
MODULE_DEVICE_TABLE(of, fm1388_dt_ids);
static int fm1388_spi_probe(struct spi_device *spi)
{
	//spi->max_speed_hz=500*1000;
	//spi->bits_per_word=8;
	lidbg(TAG"%s: max_speed = %d, chip_select = %d, mode = %d, modalias = %s,bits_per_word = %d\n", __func__, spi->max_speed_hz, spi->chip_select, spi->mode, spi->modalias,spi->bits_per_word);
	fm1388_spi = spi;
	return 0;
}

static int fm1388_spi_remove(struct spi_device *spi)
{
	return 0;
}

#ifdef CONFIG_PM
static int fm1388_spi_suspend(struct device *dev)
{
    return 0;
}

static int fm1388_spi_resume(struct device *dev)
{
    return 0;
}

static const struct dev_pm_ops fm1388_spi_ops = {
	.suspend = fm1388_spi_suspend,
	.resume  = fm1388_spi_resume,
};
#endif

static struct spi_driver fm1388_spi_driver = {
	.driver = {
			.name = "fm1388",
//			.of_match_table = of_match_ptr(fm1388_dt_ids),
			.owner = THIS_MODULE,
#ifdef CONFIG_PM
			.pm   = &fm1388_spi_ops,
#endif
	},
	.probe  = fm1388_spi_probe,
	.remove = fm1388_spi_remove,
	.id_table = fm1388_spi_id,
};

//module_spi_driver(fm1388_spi_driver);

static int  fm1388_spi_init(void)
{

	int status;
	DUMP_BUILD_TIME;
	LIDBG_GET;
	lidbg(TAG"%s:\n",__func__);
	register_fm1388_spi_device();
	status = spi_register_driver(&fm1388_spi_driver);
	if (status < 0) {
		lidbg(TAG"%s:fm1388_spi_driver failure. status = %d\n", __func__, status);
	}
	lidbg(TAG"%s:fm1388_spi_driver success. status = %d\n", __func__, status);
    return status;
}


static void __exit fm1388_spi_exit(void)
{
	 if (fm1388_spi)
    {
        spi_unregister_device(fm1388_spi);
        fm1388_spi = NULL;
    }
	spi_unregister_driver(&fm1388_spi_driver);
}

module_init(fm1388_spi_init);

module_exit(fm1388_spi_exit);

MODULE_DESCRIPTION("FM1388 SPI driver");
MODULE_AUTHOR("sample code <dannylan@fortemedia.com>");
MODULE_LICENSE("GPL v2");
