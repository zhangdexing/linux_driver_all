/* -------------------------------------------------------------------------
 * i2c-algo-bit.c i2c driver algorithms for bit-shift adapters
 * -------------------------------------------------------------------------
 *   Copyright (C) 1995-2000 Simon G. Vogl

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
 * ------------------------------------------------------------------------- */

/* With some changes from Frodo Looijaard <frodol@dds.nl>, Ky\u679asti M\u76f2lkki
   <kmalkki@cc.hut.fi> and Jean Delvare <khali@linux-fr.org> */

#include "i2c_gpio.h"
#include "soc.h"

#define	setsda_in(dev)			gpio_set_direction(dev->sda_pin, GPIO_INPUT)
#define	setsda_out(dev)			gpio_set_direction(dev->sda_pin, GPIO_OUTPUT)

#define setsda(dev, val)		gpio_set_val(dev->sda_pin, val)
#define setscl(dev, val)		gpio_set_val(dev->scl_pin, val)
#define getsda(dev)				gpio_get_val(dev->sda_pin)
#define getscl(dev)				gpio_get_val(dev->scl_pin)

static struct i2c_gpio_dev *dev = NULL;

static inline void sdalo(struct i2c_gpio_dev *dev)
{
    setsda(dev, 0);
    udelay((dev->udelay + 1) / 2);
}

static inline void sdahi(struct i2c_gpio_dev *dev)
{
    setsda(dev, 1);
    udelay((dev->udelay + 1) / 2);
}

static inline void scllo(struct i2c_gpio_dev *dev)
{
    setscl(dev, 0);
    udelay(dev->udelay / 2);
}

/*
 * Raise scl line, and do checking for delays. This is necessary for slower
 * devices.
 */
static int sclhi(struct i2c_gpio_dev *dev)
{
    unsigned long start;

    setscl(dev, 1);
#if 0
    //	start = jiffies;
    while (!getscl(dev))
    {
        /* This hw knows how to read the clock line, so we wait
         * until it actually gets high.  This is safer as some
         * chips may hold it low ("clock stretching") while they
         * are processing data internally.
         */
        //		if (time_after(jiffies, start + dev->timeout)) {
        /* Test one last time, as we may have been preempted
         * between last check and timeout test.
         */
        if (getscl(dev))
            break;
        return -ETIMEDOUT;
        //		}
    }
#endif
    udelay(dev->udelay);
    return 0;
}


/* --- other auxiliary functions --------------------------------------	*/
static void i2c_start(struct i2c_gpio_dev *dev)
{
    /* assert: scl, sda are high */
    setsda(dev, 0);
    udelay(dev->udelay);
    scllo(dev);
}

static void i2c_repstart(struct i2c_gpio_dev *dev)
{
    /* assert: scl is low */
    sdahi(dev);
    sclhi(dev);
    setsda(dev, 0);
    udelay(dev->udelay);
    scllo(dev);
}


static void i2c_stop(struct i2c_gpio_dev *dev)
{
    /* assert: scl is low */
    sdalo(dev);
    sclhi(dev);
    setsda(dev, 1);
    udelay(dev->udelay);
}



/* send a byte without start cond., look for arbitration,
   check ackn. from slave */
/* returns:
 * 1 if the device acknowledged
 * 0 if the device did not ack
 * -ETIMEDOUT if an error occurred (while raising the scl line)
 */
static int i2c_outb(struct i2c_gpio_dev *dev, unsigned char c)
{
    int i;
    int sb;
    int ack;

    /* assert: scl is low */
    for (i = 7; i >= 0; i--)
    {
        sb = (c >> i) & 1;
        setsda(dev, sb);
        udelay((dev->udelay + 1) / 2);
        if (sclhi(dev) < 0)   /* timed out */
        {
            dprintf(INFO, "i2c_outb: 0x%02x timeout at bit #%d\n", (int)c, i);
            return -ETIMEDOUT;
        }
        /* FIXME do arbitration here:
         * if (sb && !getsda(adap)) -> ouch! Get out of here.
         *
         * Report a unique code, so higher level code can retry
         * the whole (combined) message and *NOT* issue STOP.
         */
        scllo(dev);
    }
    sdahi(dev);
#ifdef BOOTLOADER_MSM8996
	setsda_in(dev);
#endif
    if (sclhi(dev) < 0)   /* timeout */
    {
        dprintf(INFO, "i2c_outb: 0x%02x, "
                "timeout at ack\n", (int)c);
        return -ETIMEDOUT;
    }

    /* read ack: SDA should be pulled down by slave, or it may
     * NAK (usually to report problems with the data we wrote).
     */
    setsda_in(dev);
    ack = !getsda(dev);    /* ack: sda is pulled low -> success */
    //	dprintf(INFO, "i2c_outb: 0x%02x %s\n", (int)c,
    //		ack ? "A" : "NA");
#ifdef BOOTLOADER_IMX6Q
	scllo(dev);
	setsda_out(dev);
#else
    setsda_out(dev);
    scllo(dev);
#endif
    return ack;
    /* assert: scl is low (sda undef) */
}


static int i2c_inb(struct i2c_gpio_dev *dev)
{
    /* read byte via i2c port, without start/stop sequence	*/
    /* acknowledge is sent in i2c_read.			*/
    int i;
    unsigned char indata = 0;

    /* assert: scl is low */
    sdahi(dev);
    for (i = 0; i < 8; i++)
    {
#ifdef BOOTLOADER_MSM8996
       setsda_in(dev);
#endif
        if (sclhi(dev) < 0)   /* timeout */
        {
            dprintf(INFO, "i2c_inb: timeout at bit #%d\n", 7 - i);
            return -ETIMEDOUT;
        }
        setsda_in(dev);
        indata *= 2;
        if (getsda(dev))
            indata |= 0x01;
        setscl(dev, 0);
        udelay(i == 7 ? dev->udelay / 2 : dev->udelay);
    }
    setsda_out(dev);
    /* assert: scl is low */
    return indata;
}

/*
 * Sanity check for the adapter hardware - check the reaction of
 * the bus lines only if it seems to be idle.
 */
static int test_bus(struct i2c_gpio_dev *dev)
{
    const char *name = dev->name;
    int scl, sda, ret;

    sda = getsda(dev);
    scl =  getscl(dev);
    if (!scl || !sda)
    {
        dprintf(INFO, "%s: bus seems to be busy (scl=%d, sda=%d)\n", name, scl, sda);
        goto bailout;
    }

    sdalo(dev);
    sda = getsda(dev);
    scl = getscl(dev);
    if (sda)
    {
        dprintf(INFO, "%s: SDA stuck high!\n", name);
        goto bailout;
    }
    if (!scl)
    {
        dprintf(INFO, "%s: SCL unexpected low "
                "while pulling SDA low!\n", name);
        goto bailout;
    }

    sdahi(dev);
    sda = getsda(dev);
    scl =  getscl(dev);
    if (!sda)
    {
        dprintf(INFO, "%s: SDA stuck low!\n", name);
        goto bailout;
    }
    if (!scl)
    {
        dprintf(INFO, "%s: SCL unexpected low "
                "while pulling SDA high!\n", name);
        goto bailout;
    }

    scllo(dev);
    sda = getsda(dev);
    scl =  getscl(dev);
    if (scl)
    {
        dprintf(INFO, "%s: SCL stuck high!\n", name);
        goto bailout;
    }
    if (!sda)
    {
        dprintf(INFO, "%s: SDA unexpected low "
                "while pulling SCL low!\n", name);
        goto bailout;
    }

    sclhi(dev);
    sda = getsda(dev);
    scl =  getscl(dev);
    if (!scl)
    {
        dprintf(INFO, "%s: SCL stuck low!\n", name);
        goto bailout;
    }
    if (!sda)
    {
        dprintf(INFO, "%s: SDA unexpected low "
                "while pulling SCL high!\n", name);
        goto bailout;
    }

    dprintf(INFO, "%s: Test OK\n", name);
    return 0;
bailout:
    sdahi(dev);
    sclhi(dev);

    return -ENODEV;
}

/* ----- Utility functions
 */

/* try_address tries to contact a chip for a number of
 * times before it gives up.
 * return values:
 * 1 chip answered
 * 0 chip did not answer
 * -x transmission error
 */
static int try_address(struct i2c_gpio_dev *dev, unsigned char addr, int retries)
{
    int i, ret = 0;

    for (i = 0; i <= retries; i++)
    {
        ret = i2c_outb(dev, addr);
        if (ret == 1 || i == retries)
            break;
        //		dprintf(INFO, "emitting stop condition\n");
        i2c_stop(dev);
        udelay(dev->udelay);
        //		yield();
        //		dprintf(INFO, "emitting start condition\n");
        i2c_start(dev);
    }
    if (i && ret)
        dprintf(INFO, "Used %d tries to %s client at 0x%02x: %s\n", i + 1,
                addr & 1 ? "read from" : "write to", addr >> 1,
                ret == 1 ? "success" : "failed, timeout?");
    return ret;
}

static int sendbytes(struct i2c_gpio_dev *dev, struct i2c_msg *msg)
{
    const unsigned char *temp = msg->buf;
    int count = msg->len;
    unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;
    int retval;
    int wrcount = 0;

    while (count > 0)
    {
        retval = i2c_outb(dev, *temp);

        /* OK/ACK; or ignored NAK */
        if ((retval > 0) || (nak_ok && (retval == 0)))
        {
            count--;
            temp++;
            wrcount++;

            /* A slave NAKing the master means the slave didn't like
             * something about the data it saw.  For example, maybe
             * the SMBus PEC was wrong.
             */
        }
        else if (retval == 0)
        {
            dprintf(INFO, "sendbytes: NAK bailout.\n");
            return -EIO;

            /* Timeout; or (someday) lost arbitration
             *
             * FIXME Lost ARB implies retrying the transaction from
             * the first message, after the "winning" master issues
             * its STOP.  As a rule, upper layer code has no reason
             * to know or care about this ... it is *NOT* an error.
             */
        }
        else
        {
            dprintf(INFO, "sendbytes: error %d\n",
                    retval);
            return retval;
        }
    }
    return wrcount;
}

static int acknak(struct i2c_gpio_dev *dev, int is_ack)
{
    /* assert: sda is high */
    if (is_ack)		/* send ack */
        setsda(dev, 0);
    udelay((dev->udelay + 1) / 2);
    if (sclhi(dev) < 0)  	/* timeout */
    {
        dprintf(INFO, "readbytes: ack/nak timeout\n");
        return -ETIMEDOUT;
    }
    scllo(dev);
    return 0;
}

static int readbytes(struct i2c_gpio_dev *dev, struct i2c_msg *msg)
{
    int inval;
    int rdcount = 0;	/* counts bytes read */
    unsigned char *temp = msg->buf;
    int count = msg->len;
    const unsigned flags = msg->flags;

    while (count > 0)
    {
        inval = i2c_inb(dev);
        if (inval >= 0)
        {
            *temp = inval;
            rdcount++;
        }
        else       /* read timed out */
        {
            break;
        }

        temp++;
        count--;

        /* Some SMBus transactions require that we receive the
           transaction length as the first read byte. */
        if (rdcount == 1 && (flags & I2C_M_RECV_LEN))
        {
            if (inval <= 0 || inval > I2C_SMBUS_BLOCK_MAX)
            {
                if (!(flags & I2C_M_NO_RD_ACK))
                    acknak(dev, 0);
                dprintf(INFO, "readbytes: invalid "
                        "block length (%d)\n", inval);
                return -1;
            }
            /* The original count value accounts for the extra
               bytes, that is, either 1 for a regular transaction,
               or 2 for a PEC transaction. */
            count += inval;
            msg->len += inval;
        }

        //		dprintf(INFO, "readbytes: 0x%02x %s\n",
        //			inval,
        //			(flags & I2C_M_NO_RD_ACK)
        //				? "(no ack/nak)"
        //				: (count ? "A" : "NA"));

        if (!(flags & I2C_M_NO_RD_ACK))
        {
            inval = acknak(dev, count);
            if (inval < 0)
                return inval;
        }
    }
    return rdcount;
}

/* doAddress initiates the transfer by generating the start condition (in
 * try_address) and transmits the address in the necessary format to handle
 * reads, writes as well as 10bit-addresses.
 * returns:
 *  0 everything went okay, the chip ack'ed, or IGNORE_NAK flag was set
 * -x an error occurred (like: -ENXIO if the device did not answer, or
 *	-ETIMEDOUT, for example if the lines are stuck...)
 */
static int bit_doAddress(struct i2c_gpio_dev *dev, struct i2c_msg *msg)
{
    unsigned short flags = msg->flags;
    unsigned short nak_ok = msg->flags & I2C_M_IGNORE_NAK;

    unsigned char addr;
    int ret, retries;

    retries = nak_ok ? 0 : dev->retries;

    if (flags & I2C_M_TEN)
    {
        /* a ten bit address */
        addr = 0xf0 | ((msg->addr >> 7) & 0x06);
        dprintf(INFO, "Ten bits addr0: 0x%x\n", addr);
        /* try extended address code...*/
        ret = try_address(dev, addr, retries);
        if ((ret != 1) && !nak_ok)
        {
            dprintf(INFO,
                    "died at extended address code\n");
            return -1;
        }
        /* the remaining 8 bit address */
        ret = i2c_outb(dev, msg->addr & 0xff);
        if ((ret != 1) && !nak_ok)
        {
            /* the chip did not ack / xmission error occurred */
            dprintf(INFO, "died at 2nd address code\n");
            return -1;
        }
        if (flags & I2C_M_RD)
        {
            dprintf(INFO, "emitting repeated "
                    "start condition\n");
            i2c_repstart(dev);
            /* okay, now switch into reading mode */
            addr |= 0x01;
            ret = try_address(dev, addr, retries);
            if ((ret != 1) && !nak_ok)
            {
                dprintf(INFO,
                        "died at repeated address code\n");
                return -EIO;
            }
        }
    }
    else  		/* normal 7bit address	*/
    {
        addr = msg->addr << 1;
        //		dprintf(INFO, "Seven bits addr: 0x%x\n", addr);

        if (flags & I2C_M_RD)
            addr |= 1;
        if (flags & I2C_M_REV_DIR_ADDR)
            addr ^= 1;
        ret = try_address(dev, addr, retries);
        if ((ret != 1) && !nak_ok)
        {
            dprintf(INFO, "I2C=====Seven bits addr error: 0x%x\n", addr);
            return -1;
        }
    }

    return 0;
}

int bit_xfer(struct i2c_gpio_dev *dev, struct i2c_msg msgs[], int num)
{
    struct i2c_msg *pmsg;
    int i, ret;
    unsigned short nak_ok;

    //	dprintf(INFO, "emitting start condition\n");
    i2c_start(dev);
    for (i = 0; i < num; i++)
    {
        pmsg = &msgs[i];
        nak_ok = pmsg->flags & I2C_M_IGNORE_NAK;
        if (!(pmsg->flags & I2C_M_NOSTART))
        {
            if (i)
            {
                //dprintf(INFO, "emitting repeated start condition\n");
                i2c_repstart(dev);
            }
            ret = bit_doAddress(dev, pmsg);
            if ((ret != 0) && !nak_ok)
            {
                dprintf(INFO, "NAK from device addr 0x%02x msg #%d\n", msgs[i].addr, i);
                goto bailout;
            }
        }
        if (pmsg->flags & I2C_M_RD)
        {
            /* read bytes into buffer*/
            ret = readbytes(dev, pmsg);
            if (ret >= 1)
                //				dprintf(INFO, "read %d byte%s\n",
                //					ret, ret == 1 ? "" : "s");
                ;
            if (ret < pmsg->len)
            {
                if (ret >= 0)
                    ret = -EIO;
                goto bailout;
            }
        }
        else
        {
            /* write bytes from buffer */
            ret = sendbytes(dev, pmsg);
            if (ret >= 1)
                //				dprintf(INFO, "wrote %d byte%s\n",
                //					ret, ret == 1 ? "" : "s");
                ;
            if (ret < pmsg->len)
            {
                if (ret >= 0)
                    ret = -EIO;
                goto bailout;
            }
        }
    }
    ret = i;

bailout:
    //	dprintf(INFO, "emitting stop condition\n");
    i2c_stop(dev);

    return ret;
}
