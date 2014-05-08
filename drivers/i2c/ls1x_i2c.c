/*
 * Copyright (c) 2014 TangHaifeng <tanghaifeng-gz@loongson.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>

#include <asm/ls1x.h>

DECLARE_GLOBAL_DATA_PTR;

struct ls1x_i2c_regs {
	unsigned char prerlo;
	unsigned char prerhi;
	unsigned char control;
	unsigned char data;
	unsigned char cr_sr;
};

#define OCI2C_CTRL_IEN		0x40
#define OCI2C_CTRL_EN		0x80

#define OCI2C_CMD_START		0x90
#define OCI2C_CMD_STOP		0x40
#define OCI2C_CMD_READ		0x20
#define OCI2C_CMD_WRITE		0x10
#define OCI2C_CMD_READ_ACK	0x20
#define OCI2C_CMD_READ_NACK	0x28
#define OCI2C_CMD_IACK		0x00

#define OCI2C_STAT_IF		0x01
#define OCI2C_STAT_TIP		0x02
#define OCI2C_STAT_ARBLOST	0x20
#define OCI2C_STAT_BUSY		0x40
#define OCI2C_STAT_NACK		0x80

/* U-Boot I2C framework allows only one active device at a time.  */
static volatile struct ls1x_i2c_regs *ls1x_i2c = (void *)LS1X_I2C0_BASE;

struct i2c_msg {
	__u16 addr;	/* slave address			*/
	__u16 flags;
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_RD		0x0001	/* read data, from slave to master */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
	__u16 len;		/* msg length				*/
	__u8 *buf;		/* pointer to msg data			*/
};

/*
 * Poll the i2c status register until the specified bit is set.
 * Returns 0 if timed out (100 msec).
 */
static short ls1x_poll_status(unsigned long bit)
{
	int loop_cntr = 20000;

	do {
		udelay(1);
	} while ((ls1x_i2c->cr_sr & bit) && (--loop_cntr > 0));

	return (loop_cntr > 0);
}

static int ls1x_xfer_read(unsigned char *buf, int length) 
{
	int x;

	for (x=0; x<length; x++) {
		/* send ACK last not send ACK */
		if (x != (length -1)) 
			ls1x_i2c->cr_sr = OCI2C_CMD_READ_ACK;
		else
			ls1x_i2c->cr_sr = OCI2C_CMD_READ_NACK;

		if (!ls1x_poll_status(OCI2C_STAT_TIP)) {
			printf("READ timeout\n");
			return -1;
		}
		*buf++ = ls1x_i2c->data;
	}
	ls1x_i2c->cr_sr = OCI2C_CMD_STOP;
		
	return 0;
}

static int ls1x_xfer_write(unsigned char *buf, int length)
{
	int x;

	for (x=0; x<length; x++) {
		ls1x_i2c->data = *buf++;
		ls1x_i2c->cr_sr = OCI2C_CMD_WRITE;
		if (!ls1x_poll_status(OCI2C_STAT_TIP)) {
			printf("WRITE timeout\n");
			return -1;
		}
		if (ls1x_i2c->cr_sr & OCI2C_STAT_NACK) {
			ls1x_i2c->cr_sr = OCI2C_CMD_STOP;
			return length;
		}
	}
	ls1x_i2c->cr_sr = OCI2C_CMD_STOP;

	return 0;
}

/**
 * i2c_transfer - setup an i2c transfer
 *	@return: 0 if things worked, non-0 if things failed
 *
 *	Here we just get the i2c stuff all prepped and ready, and then tail off
 *	into wait_for_completion() for all the bits to go.
 */
static int i2c_transfer(struct i2c_msg *pmsg, int num)
{
	int i, ret;

//	dev_dbg(&adap->dev, "ls1x_xfer: processing %d messages:\n", num);

	for (i = 0; i < num; i++) {
/*		dev_dbg(&adap->dev, " #%d: %sing %d byte%s %s 0x%02x\n", i,
			pmsg->flags & I2C_M_RD ? "read" : "writ",
			pmsg->len, pmsg->len > 1 ? "s" : "",
			pmsg->flags & I2C_M_RD ? "from" : "to",	pmsg->addr);*/

		if (!ls1x_poll_status(OCI2C_STAT_BUSY)) {
			return -1;
		}

		ls1x_i2c->data = (pmsg->addr << 1) | ((pmsg->flags & I2C_M_RD) ? 1 : 0);
		ls1x_i2c->cr_sr = OCI2C_CMD_START;

		/* Wait until transfer is finished */
		if (!ls1x_poll_status(OCI2C_STAT_TIP)) {
			printf("TXCOMP timeout\n");
			return -1;
		}

		if (ls1x_i2c->cr_sr & OCI2C_STAT_NACK) {
			printf("slave addr no ack !!\n");
			ls1x_i2c->cr_sr = OCI2C_CMD_STOP;
			return -1;
		}

 		if (pmsg->flags & I2C_M_RD)
			ret = ls1x_xfer_read(pmsg->buf, pmsg->len);
  		else
			ret = ls1x_xfer_write(pmsg->buf, pmsg->len);

		if (ret)
			return ret;
//		dev_dbg(&adap->dev, "transfer complete\n");
		pmsg++;
	}
	return 0;
}

/**
 * i2c_set_bus_speed - set i2c bus speed
 *	@speed: bus speed (in HZ)
 */
int i2c_set_bus_speed(unsigned int speed)
{
	unsigned long clk;
	int prescale;

	clk = gd->bus_clk;
	prescale = (clk / (5*speed)) - 1;
	ls1x_i2c->prerlo = prescale & 0xff;
	ls1x_i2c->prerhi = prescale >> 8;

	return 0;
}

/**
 * i2c_get_bus_speed - get i2c bus speed
 *	@speed: bus speed (in HZ)
 */
unsigned int i2c_get_bus_speed(void)
{
	unsigned long clk;
	int prescale;

	clk = gd->bus_clk;
	prescale = (ls1x_i2c->prerhi & 0xff) << 8 | (ls1x_i2c->prerlo & 0xff);
	return (clk/ (5*(prescale + 1)));
}

/**
 * i2c_init - initialize the i2c bus
 *	@speed: bus speed (in HZ)
 *	@slaveaddr: address of device in slave mode (0 - not slave)
 *
 *	Slave mode isn't actually implemented.  It'll stay that way until
 *	we get a real request for it.
 */
void i2c_init(int speed, int slaveaddr)
{
	u8 ctrl = ls1x_i2c->control;

	/* make sure the device is disabled */
	ls1x_i2c->control = ctrl & ~(OCI2C_CTRL_EN | OCI2C_CTRL_IEN);

	/* Set TWI interface clock as specified */
	i2c_set_bus_speed(speed);

	/* Init the device */
	ls1x_i2c->cr_sr = OCI2C_CMD_IACK;
	ls1x_i2c->control = ctrl | OCI2C_CTRL_EN;
}

/**
 * i2c_read - read data from an i2c device
 *	@chip: i2c chip addr
 *	@addr: memory (register) address in the chip
 *	@alen: byte size of address
 *	@buffer: buffer to store data read from chip
 *	@len: how many bytes to read
 *	@return: 0 on success, non-0 on failure
 */
int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	uchar addr_buffer[] = {
		(addr >>  0),
		(addr >>  8),
		(addr >> 16),
	};
	struct i2c_msg msg[2] = { { chip, 0, alen, addr_buffer },
	                          { chip, I2C_M_RD, len, buffer }
	                        };

	return i2c_transfer(msg, 2);
}

/**
 * i2c_write - write data to an i2c device
 *	@chip: i2c chip addr
 *	@addr: memory (register) address in the chip
 *	@alen: byte size of address
 *	@buffer: buffer holding data to write to chip
 *	@len: how many bytes to write
 *	@return: 0 on success, non-0 on failure
 */
int i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	uchar addr_buffer[] = {
		(addr >>  0),
		(addr >>  8),
		(addr >> 16),
	};
	struct i2c_msg msg[2] = { { chip, 0, alen, addr_buffer },
	                          { chip, 0, len, buffer }
	                        };

	return i2c_transfer(msg, 2);
}

/**
 * i2c_probe - test if a chip exists at a given i2c address
 *	@chip: i2c chip addr to search for
 *	@return: 0 if found, non-0 if not found
 */
int i2c_probe(uchar chip)
{
	u8 byte;
	return i2c_read(chip, 0, 0, &byte, 1);
}

/**
 * i2c_set_bus_num - change active I2C bus
 *	@bus: bus index, zero based
 *	@returns: 0 on success, non-0 on failure
 */
int i2c_set_bus_num(unsigned int bus)
{
	switch (bus) {
#if CONFIG_SYS_MAX_I2C_BUS > 0
		case 0: ls1x_i2c = (void *)LS1X_I2C0_BASE; return 0;
#endif
#if CONFIG_SYS_MAX_I2C_BUS > 1
		case 1: ls1x_i2c = (void *)LS1X_I2C1_BASE; return 0;
#endif
#if CONFIG_SYS_MAX_I2C_BUS > 2
		case 2: ls1x_i2c = (void *)LS1X_I2C2_BASE; return 0;
#endif
		default: return -1;
	}
}

/**
 * i2c_get_bus_num - returns index of active I2C bus
 */
unsigned int i2c_get_bus_num(void)
{
	switch ((unsigned long)ls1x_i2c) {
#if CONFIG_SYS_MAX_I2C_BUS > 0
		case LS1X_I2C0_BASE: return 0;
#endif
#if CONFIG_SYS_MAX_I2C_BUS > 1
		case LS1X_I2C1_BASE: return 1;
#endif
#if CONFIG_SYS_MAX_I2C_BUS > 2
		case LS1X_I2C2_BASE: return 2;
#endif
		default: return -1;
	}
}

