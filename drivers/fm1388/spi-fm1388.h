/*
 * drivers/spi/spi-fm1388.h
 * (C) Copyright 2014-2018
 * Fortemedia, Inc. <www.fortemedia.com>
 * Author: HenryZhang <henryhzhang@fortemedia.com>;
 * 			LiFu <fuli@fortemedia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __FM1388_SPI_H__
#define __FM1388_SPI_H__

#define ESUCCESS			0
#define EUNKNOWN			10000
#define EDUPOPEN			10001
#define EFAILOPEN			10002
#define ENOTOPEN			10003
#define EPARAMINVAL			10004
#define EDATAINVAL			10005
#define ESPIREAD			10006
#define ESPIWRITE			10007
#define ESPISETFAIL			10008
#define EDSPNOTWORK			10009
#define ENOMEMORY			10010
#define EMEMFAULT			10011
#define ECANNOTENRECORD		10012
#define ENOSDCARD			10013
#define EFILECANNOTWRITE	10014
#define EINPROCESSING		10015
#define EFAILTOSETMODE		10016
#define ECOMMANDINVAL		10017


//#define FM1388_SPI_BUF_LEN 240
#define FM1388_SPI_BUF_LEN 48	//changed to 48 due to dma issue
#define FM1388_SPI_READ_BUF_LEN 48
#define MAX_BLOCK_SIZE			512		//24k:384, 16k:320

/* SPI Command */
enum {
	FM1388_SPI_CMD_16_READ = 0,
	FM1388_SPI_CMD_16_WRITE,
	FM1388_SPI_CMD_32_READ,
	FM1388_SPI_CMD_32_WRITE,
	FM1388_SPI_CMD_BURST_READ,
	FM1388_SPI_CMD_BURST_WRITE,
	FM1388_SPI_CMD_16_READ_16ADDR = 8,
	FM1388_SPI_CMD_16_WRITE_16ADDR,
};
#if 0
int fm1388_spi_read(u32 addr, u32 *val, size_t len);
int fm1388_spi_write(u32 addr, u32 val, size_t len);
int fm1388_spi_burst_read(u32 addr, u8 *rxbuf, size_t len);
int fm1388_spi_burst_write(u32 addr, const u8 *txbuf, size_t len);
#endif
#endif /* __FM1388_SPI_H__ */
