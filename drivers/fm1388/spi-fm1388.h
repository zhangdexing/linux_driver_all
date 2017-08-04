/*
 * fm1388-spi.h  --  FM1388 ALSA SoC audio codec driver
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FM1388_SPI_H__
#define __FM1388_SPI_H__

//#define FM1388_SPI_BUF_LEN 240
#define FM1388_SPI_BUF_LEN 256	//changed to 48 due to dma issue

/* SPI Command */
enum {
	FM1388_SPI_CMD_16_READ = 0,
	FM1388_SPI_CMD_16_WRITE,
	FM1388_SPI_CMD_32_READ,
	FM1388_SPI_CMD_32_WRITE,
	FM1388_SPI_CMD_BURST_READ,
	FM1388_SPI_CMD_BURST_WRITE,
};
#if 0
int fm1388_spi_read(unsigned int addr, unsigned int *val, size_t len);
int fm1388_spi_write(unsigned int addr, unsigned int val, size_t len);
int fm1388_spi_burst_read(unsigned int addr, u8 *rxbuf, size_t len);
int fm1388_spi_burst_write(u32 addr, const u8 *txbuf, size_t len);
#endif
#endif /* __FM1388_SPI_H__ */
