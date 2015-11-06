/*
 * Loongson1 spi driver
 *
 * based on bfin_spi.c
 * Copyright (c) 2005-2008 Analog Devices Inc.
 * Copyright (C) 2014 Tang Haifeng <tanghaifeng-gz@loongson.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <spi.h>
#include <asm/gpio.h>

#include <asm/ls1x.h>

DECLARE_GLOBAL_DATA_PTR;

struct ls1x_spi_regs {
	unsigned char spcr;
	unsigned char spsr;
	unsigned char fifo;	/* TX/Rx data reg */
	unsigned char sper;
	unsigned char param;
	unsigned char softcs;
	unsigned char timing;
};

struct ls1x_spi_host {
	uint base;
	uint freq;
	uint baudwidth;
};
static const struct ls1x_spi_host ls1x_spi_host_list[] = {
	{
		.base = LS1X_SPI0_BASE,
	},
	{
		.base = LS1X_SPI1_BASE,
	},
};

struct ls1x_spi_slave {
	struct spi_slave slave;
	const struct ls1x_spi_host *host;
	uint mode;
	uint div;
	uint flg;
};
#define to_ls1x_spi_slave(s) container_of(s, struct ls1x_spi_slave, slave)

static int inited = 0;

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
#if defined(CONFIG_SPI_CS_USED_GPIO)
	return bus < ARRAY_SIZE(ls1x_spi_host_list) && gpio_is_valid(cs);
#elif defined(CONFIG_SPI_CS)
	return bus < ARRAY_SIZE(ls1x_spi_host_list);
#endif
}

void spi_cs_activate(struct spi_slave *slave)
{
	struct ls1x_spi_slave *ls1x_spi = to_ls1x_spi_slave(slave);
	unsigned int cs = slave->cs;

#if defined(CONFIG_SPI_CS_USED_GPIO)
	gpio_set_value(cs, ls1x_spi->flg);
//	debug("%s: SPI_CS_GPIO:%x\n", __func__, gpio_get_value(cs));
#elif defined(CONFIG_SPI_CS)
	{
	struct ls1x_spi_regs *regs = (void *)ls1x_spi->host->base;
	u8 ret;
	ret = readb(&regs->softcs);
	ret = (ret & 0xf0) | (0x01 << cs);
	
	if (ls1x_spi->flg) {
		ret = ret | (0x10 << cs);
		writeb(ret, &regs->softcs);
	} else {
		ret = ret & (~(0x10 << cs));
		writeb(ret, &regs->softcs);
	}
	}
#endif
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	struct ls1x_spi_slave *ls1x_spi = to_ls1x_spi_slave(slave);
	unsigned int cs = slave->cs;

#if defined(CONFIG_SPI_CS_USED_GPIO)
	gpio_set_value(cs, !ls1x_spi->flg);
//	debug("%s: SPI_CS_GPIO:%x\n", __func__, gpio_get_value(cs));
#elif defined(CONFIG_SPI_CS)
	{
	struct ls1x_spi_regs *regs = (void *)ls1x_spi->host->base;
	u8 ret;
	ret = readb(&regs->softcs);
	ret = (ret & 0xf0) | (0x01 << cs);
	
	if (ls1x_spi->flg) {
		ret = ret & (~(0x10 << cs));
		writeb(ret, &regs->softcs);
	} else {
		ret = ret | (0x10 << cs);
		writeb(ret, &regs->softcs);
	}
	}
#endif
}

void spi_set_speed(struct spi_slave *slave, uint hz)
{
	struct ls1x_spi_slave *ls1x_spi = to_ls1x_spi_slave(slave);

	unsigned int div, div_tmp, bit;
	unsigned long clk;

	clk = gd->bus_clk;
	div = DIV_ROUND_UP(clk, hz);

	if (div < 2)
		div = 2;

	if (div > 4096)
		div = 4096;

	bit = fls(div) - 1;
	switch(1 << bit) {
		case 16: 
			div_tmp = 2;
			if (div > (1<<bit)) {
				div_tmp++;
			}
			break;
		case 32:
			div_tmp = 3;
			if (div > (1<<bit)) {
				div_tmp += 2;
			}
			break;
		case 8:
			div_tmp = 4;
			if (div > (1<<bit)) {
				div_tmp -= 2;
			}
			break;
		default:
			div_tmp = bit - 1;
			if (div > (1<<bit)) {
				div_tmp++;
			}
			break;
	}
//	debug("clk = %ld hz = %d div_tmp = %d bit = %d\n", clk, hz, div_tmp, bit);

	ls1x_spi->div = div_tmp;
}

void spi_init(void)
{
	const struct ls1x_spi_host *host;
	struct ls1x_spi_regs *regs;
	int i;
	u8 val;

	if (inited) {
		inited = 1;
		return;
	}

	for (i = 0; i < ARRAY_SIZE(ls1x_spi_host_list); i += 1) {
		host = &ls1x_spi_host_list[i];
		regs = (void *)host->base;
		/* 使能SPI控制器，master模式，关闭中断 */
		writeb(0x53, &regs->spcr);
		/* 清空状态寄存器 */
		writeb(0xc0, &regs->spsr);
		/* 1字节产生中断，采样(读)与发送(写)时机同时 */
		writeb(0x03, &regs->sper);
	#if defined(CONFIG_SPI_CS_USED_GPIO)
		writeb(0x00, &regs->softcs);
	#elif defined(CONFIG_SPI_CS)
		writeb(0xff, &regs->softcs);
	#endif
		/* 关闭SPI flash */
		val = readb(&regs->param);
		val &= 0xfe;
		writeb(val, &regs->param);
		/* SPI flash时序控制寄存器 */
		writeb(0x05, &regs->timing);
	}
#if defined(CONFIG_LS1X_SPI1_ENABLE) && defined(CONFIG_CPU_LOONGSON1B)
	writel(readl(LS1X_MUX_CTRL1) | SPI1_USE_CAN | SPI1_CS_USE_PWM01, LS1X_MUX_CTRL1);
#endif
}

struct spi_slave *spi_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int hz, unsigned int mode)
{
	struct ls1x_spi_slave *ls1x_spi;

#if defined(CONFIG_SPI_CS_USED_GPIO)
	if (!spi_cs_is_valid(bus, cs) || gpio_request(cs, "ls1x_spi"))
		return NULL;
#elif defined(CONFIG_SPI_CS)
	if (!spi_cs_is_valid(bus, cs))
		return NULL;
#endif

	ls1x_spi = spi_alloc_slave(struct ls1x_spi_slave, bus, cs);
	if (!ls1x_spi)
		return NULL;

	ls1x_spi->host = &ls1x_spi_host_list[bus];
	ls1x_spi->mode = mode & (SPI_CPOL | SPI_CPHA);
	ls1x_spi->flg = mode & SPI_CS_HIGH ? 1 : 0;
	spi_set_speed(&ls1x_spi->slave, hz);

	spi_init();

//	debug("%s: bus:%i cs:%i base:%x\n", __func__, bus, cs, ls1x_spi->host->base);

	return &ls1x_spi->slave;
}

void spi_free_slave(struct spi_slave *slave)
{
	struct ls1x_spi_slave *ls1x_spi = to_ls1x_spi_slave(slave);

#if defined(CONFIG_SPI_CS_USED_GPIO)
	gpio_free(slave->cs);
#endif
	free(ls1x_spi);
}

int spi_claim_bus(struct spi_slave *slave)
{
	struct ls1x_spi_slave *ls1x_spi = to_ls1x_spi_slave(slave);
	struct ls1x_spi_regs *regs = (void *)ls1x_spi->host->base;
	u8 ret;

//	debug("%s: bus:%i cs:%i\n", __func__, slave->bus, slave->cs);
#if defined(CONFIG_SPI_CS_USED_GPIO)
	gpio_direction_output(slave->cs, !ls1x_spi->flg);
#endif
	ret = readb(&regs->spcr);
	ret = ret & 0xf0;
	ret = ret | (ls1x_spi->mode << 2) | (ls1x_spi->div & 0x03);
	writeb(ret, &regs->spcr);

	ret = readb(&regs->sper);
	ret = ret & 0xfc;
	ret = ret | (ls1x_spi->div >> 2);
	writeb(ret, &regs->sper);

	return 0;
}

void spi_release_bus(struct spi_slave *slave)
{
//	debug("%s: bus:%i cs:%i\n", __func__, slave->bus, slave->cs);
}

#ifndef CONFIG_LS1X_SPI_IDLE_VAL
# define CONFIG_LS1X_SPI_IDLE_VAL 0xff
#endif

static inline void ls1x_spi_wait_rxe(struct ls1x_spi_regs *regs)
{
	u8 ret;

	ret = readb(&regs->spsr);
	ret = ret | 0x80;
	writeb(ret, &regs->spsr);	/* Int Clear */

	ret = readb(&regs->spsr);
	if (ret & 0x40) {
		writeb(ret & 0xbf, &regs->spsr);	/* Write-Collision Clear */
	}
}

static inline void ls1x_spi_wait_txe(struct ls1x_spi_regs *regs)
{
	int timeout = 20000;

	while (timeout) {
		if (readb(&regs->spsr) & 0x80) {
			break;
		}
		timeout--;
	}
}

int spi_xfer(struct spi_slave *slave, unsigned int bitlen, const void *dout,
	     void *din, unsigned long flags)
{
	struct ls1x_spi_slave *ls1x_spi = to_ls1x_spi_slave(slave);
	struct ls1x_spi_regs *regs = (void *)ls1x_spi->host->base;
	const u8 *txp = dout;
	u8 *rxp = din;
	uint bytes = bitlen / 8;
	uint i;

//	debug("%s: bus:%i cs:%i bitlen:%i bytes:%i flags:%lx\n", __func__,
//		slave->bus, slave->cs, bitlen, bytes, flags);
	if (bitlen == 0)
		goto done;

	/* assume to do 8 bits transfers */
	if (bitlen % 8) {
		flags |= SPI_XFER_END;
		goto done;
	}

	if (flags & SPI_XFER_BEGIN)
		spi_cs_activate(slave);

	/* we need to tighten the transfer loop */
	if (txp && rxp) {
		for (i = 0; i < bytes; i += 1) {
			writeb(*txp++, &regs->fifo);
			ls1x_spi_wait_txe(regs);
			*rxp++ = readb(&regs->fifo);
			ls1x_spi_wait_rxe(regs);
		}
	} else if (rxp) {
		for (i = 0; i < bytes; i += 1) {
			writeb(CONFIG_LS1X_SPI_IDLE_VAL, &regs->fifo);
			ls1x_spi_wait_txe(regs);
			*rxp++ = readb(&regs->fifo);
			ls1x_spi_wait_rxe(regs);
		}
	} else if (txp) {
		for (i = 0; i < bytes; i += 1) {
			writeb(*txp++, &regs->fifo);
			ls1x_spi_wait_txe(regs);
			readb(&regs->fifo);
			ls1x_spi_wait_rxe(regs);
		}
	} else {
		for (i = 0; i < bytes; i += 1) {
			writeb(CONFIG_LS1X_SPI_IDLE_VAL, &regs->fifo);
			ls1x_spi_wait_txe(regs);
			readb(&regs->fifo);
			ls1x_spi_wait_rxe(regs);
		}
	}

 done:
	if (flags & SPI_XFER_END)
		spi_cs_deactivate(slave);

	return 0;
}
