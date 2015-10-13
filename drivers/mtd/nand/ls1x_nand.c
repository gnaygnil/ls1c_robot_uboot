/*
 * Copyright (c) 2014 Loongson gz
 *
 * Authors: Haifeng Tang <tanghaifeng-gz@loongson.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <nand.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>

#include <asm/io.h>
#include <asm/errno.h>

#include <asm/ls1x.h>

#define DMA_ACCESS_ADDR	0x1fe78040	/* DMA对NAND操作的地址 */
#define ORDER_ADDR_IN	0xbfd01160	/* DMA配置寄存器 */
static void __iomem *order_addr_in;
#define DMA_DESC_NUM	64	/* DMA描述符占用的字节数 7x4 */
/* DMA描述符 */
#define DMA_ORDERED		0x00
#define DMA_SADDR		0x04
#define DMA_DADDR		0x08
#define DMA_LENGTH		0x0c
#define DMA_STEP_LENGTH		0x10
#define DMA_STEP_TIMES		0x14
#define	DMA_CMD			0x18

/* registers and bit definitions */
#define NAND_CMD		0x00	/* Control register */
#define NAND_ADDR_L		0x04	/* 读、写、擦除操作起始地址低32位 */
#define NAND_ADDR_H		0x08	/* 读、写、擦除操作起始地址高8位 */
#define NAND_TIMING		0x0c
#define NAND_IDL		0x10	/* ID低32位 */
#define NAND_IDH		0x14	/* ID高8位 */
#define NAND_STATUS		0x14	/* Status Register */
#define NAND_PARAM		0x18	/* 外部颗粒容量大小 */
#define NAND_OPNUM		0x1c	/* NAND读写操作Byte数;擦除为块数 */
#define NAND_CS_RDY		0x20

/* NAND_TIMING寄存器定义 */
#define HOLD_CYCLE	0x02
#define WAIT_CYCLE	0x0c

#define DMA_REQ			(0x1 << 25)
#define ECC_DMA_REQ		(0x1 << 24)
#define INT_EN			(0x1 << 13)
#define RS_WR			(0x1 << 12)
#define RS_RD			(0x1 << 11)
#define DONE			(0x1 << 10)
#define SPARE			(0x1 << 9)
#define MAIN			(0x1 << 8)
#define READ_STATUS		(0x1 << 7)
#define RESET			(0x1 << 6)
#define READ_ID			(0x1 << 5)
#define BLOCKS_ERASE	(0x1 << 4)
#define ERASE			(0x1 << 3)
#define WRITE			(0x1 << 2)
#define READ			(0x1 << 1)
#define COM_VALID		(0x1 << 0)

/* macros for registers read/write */
#define nand_writel(info, off, val)	\
	__raw_writel((val), (info)->mmio_base + (off))

#define nand_readl(info, off)		\
	__raw_readl((info)->mmio_base + (off))

#define MAX_BUFF_SIZE	10240	/* 10KByte */
#define PAGE_SHIFT		12	/* 页内地址(列地址)A0-A11 */

#if defined(CONFIG_CPU_LOONGSON1A) || defined(CONFIG_CPU_LOONGSON1C)
	#define MAIN_ADDRH(x)		(x)
	#define MAIN_ADDRL(x)		((x) << PAGE_SHIFT)
	#define MAIN_SPARE_ADDRH(x)	(x)
	#define MAIN_SPARE_ADDRL(x)	((x) << PAGE_SHIFT)
#elif defined(CONFIG_CPU_LOONGSON1B)
	#define MAIN_ADDRH(x)		((x) >> (32 - (PAGE_SHIFT - 1)))
	#define MAIN_ADDRL(x)		((x) << (PAGE_SHIFT - 1))	/* 不访问spare区时A11无效 */
	#define MAIN_SPARE_ADDRH(x)	((x) >> (32 - PAGE_SHIFT))
	#define MAIN_SPARE_ADDRL(x)	((x) << PAGE_SHIFT)
#endif

#define	GPIO_CONF1	(0xbfd010c4)
#define	GPIO_CONF2	(0xbfd010c8)
#define	GPIO_MUX	(0xbfd00420)

struct ls1x_nand_info {
	struct nand_chip	nand_chip;

	struct mtd_info		*mtd;

	unsigned int	buf_start;
	unsigned int	buf_count;

	void __iomem	*mmio_base;

	void __iomem	*dma_desc;
	dma_addr_t		dma_desc_phys;
	size_t			dma_desc_size;

	unsigned char	*data_buff;
	dma_addr_t		data_buff_phys;
	size_t			data_buff_size;

	/* relate to the command */
	unsigned int	state;
//	int	use_ecc;	/* use HW ECC ? */
	unsigned int	cmd;

	unsigned int	seqin_column;
	unsigned int	seqin_page_addr;
};

static void nand_gpio_init(void)
{
//    *((unsigned int *)GPIO_MUX_CTRL) = NAND_GPIO_MUX;
//    *((unsigned int *)GPIO_MUX_CTRL) = NAND_GPIO_MUX;
#ifdef	CONFIG_CPU_LOONGSON1A
{
	int val;
#ifdef CONFIG_NAND_USE_LPC_PWM01 //NAND复用LPC PWM01
	val = __raw_readl(GPIO_MUX);
	val |= 0x2a000000;
	__raw_writel(val, GPIO_MUX);

	val = __raw_readl(GPIO_CONF2);
	val &= ~(0xffff<<6);			//nand_D0~D7 & nand_control pin
	__raw_writel(val, GPIO_CONF2);
#elif CONFIG_NAND_USE_SPI1_PWM23 //NAND复用SPI1 PWM23
	val = __raw_readl(GPIO_MUX);
	val |= 0x14000000;
	__raw_writel(val, GPIO_MUX);

	val = __raw_readl(GPIO_CONF1);
	val &= ~(0xf<<12);				//nand_D0~D3
	__raw_writel(val, GPIO_CONF1);

	val = __raw_readl(GPIO_CONF2);
	val &= ~(0xfff<<12);			//nand_D4~D7 & nand_control pin
	__raw_writel(val, GPIO_CONF2);
#endif
}
#endif
}

static int ls1x_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
	return 0;
}

static void ls1x_nand_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

static int ls1x_nand_dev_ready(struct mtd_info *mtd)
{
	/* 多片flash的rdy信号如何判断？ */
	struct ls1x_nand_info *info = mtd->priv;
	unsigned int ret;
	ret = nand_readl(info, NAND_CMD) & 0x000f0000;
	if (ret) {
		return 1;
	}
	return 0;
}

static void ls1x_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct ls1x_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(buf, info->data_buff + info->buf_start, real_len);
	info->buf_start += real_len;
}

static u16 ls1x_nand_read_word(struct mtd_info *mtd)
{
	struct ls1x_nand_info *info = mtd->priv;
	u16 retval = 0xFFFF;

	if (!(info->buf_start & 0x1) && info->buf_start < info->buf_count) {
		retval = *(u16 *)(info->data_buff + info->buf_start);
		info->buf_start += 2;
	}
	return retval;
}

static uint8_t ls1x_nand_read_byte(struct mtd_info *mtd)
{
	struct ls1x_nand_info *info = mtd->priv;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
		retval = info->data_buff[(info->buf_start)++];

	return retval;
}

static void ls1x_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct ls1x_nand_info *info = mtd->priv;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(info->data_buff + info->buf_start, buf, real_len);
	info->buf_start += real_len;
}

static int ls1x_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i = 0;

	while (len--) {
		if (buf[i++] != ls1x_nand_read_byte(mtd)) {
			printf("verify error..., i= %d !\n\n", i-1);
			return -1;
		}
	}
	return 0;
}

static int ls1x_nand_done(struct ls1x_nand_info *info)
{
	int ret, timeout = 40000;

	do {
		ret = nand_readl(info, NAND_CMD);
		timeout--;
//		printf("NAND_CMD=0x%2X\n", nand_readl(info, NAND_CMD));
	} while (((ret & 0x400) != 0x400) && timeout);

	return timeout;
}

static void inline ls1x_nand_start(struct ls1x_nand_info *info)
{
	nand_writel(info, NAND_CMD, nand_readl(info, NAND_CMD) | 0x1);
}

static void inline ls1x_nand_stop(struct ls1x_nand_info *info)
{
}

static void start_dma_nand(unsigned int flags, struct ls1x_nand_info *info)
{
	int timeout = 5000;
	int ret;

	writel(0, info->dma_desc + DMA_ORDERED);
	writel(info->data_buff_phys, info->dma_desc + DMA_SADDR);
	writel(DMA_ACCESS_ADDR, info->dma_desc + DMA_DADDR);
	writel((info->buf_count + 3) / 4, info->dma_desc + DMA_LENGTH);
	writel(0, info->dma_desc + DMA_STEP_LENGTH);
	writel(1, info->dma_desc + DMA_STEP_TIMES);

	if (flags) {
		writel(0x00003001, info->dma_desc + DMA_CMD);
	} else {
		writel(0x00000001, info->dma_desc + DMA_CMD);
	}

	ls1x_nand_start(info);	/* 使能nand命令 */
	writel((info->dma_desc_phys & ~0x1F) | 0x8, order_addr_in);	/* 启动DMA */
	while ((readl(order_addr_in) & 0x8)/* && (timeout-- > 0)*/) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}
/* K9F5608U0D在读的时候 ls1x的nand flash控制器读取不到完成状态
   猜测是控制器对该型号flash兼容不好,目前解决办法是添加一段延时 */
#ifdef CONFIG_NAND_LS1X_READ_DELAY
	if (flags) {
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			ls1x_nand_stop(info);
		}
	} else {
		udelay(80);
	}
#else
	if (!ls1x_nand_done(info)) {
		printf("Wait time out!!!\n");
		ls1x_nand_stop(info);
	}
#endif

	while (timeout) {
		writel((info->dma_desc_phys & (~0x1F)) | 0x4, order_addr_in);
		do {
		} while (readl(order_addr_in) & 0x4);
		ret = readl(info->dma_desc + DMA_CMD);
//		if ((ret & 0x08) || flags) {
		if (ret & 0x08) {
			break;
		}
		timeout--;
	}

	if (!timeout) {
		printf("%s. %x\n",__func__, ret);
	}
}

static void ls1x_nand_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	struct ls1x_nand_info *info = mtd->priv;

	switch (command) {
	case NAND_CMD_READOOB:
		info->buf_count = mtd->oobsize;
		info->buf_start = column;
		nand_writel(info, NAND_CMD, SPARE | READ);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr) + mtd->writesize);
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16));
		start_dma_nand(0, info);
		break;
	case NAND_CMD_READ0:
		info->buf_count = mtd->writesize + mtd->oobsize;
		info->buf_start = column;
		nand_writel(info, NAND_CMD, SPARE | MAIN | READ);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(page_addr));
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
		start_dma_nand(0, info);
		break;
	case NAND_CMD_SEQIN:
		info->buf_count = mtd->writesize + mtd->oobsize - column;
		info->buf_start = 0;
		info->seqin_column = column;
		info->seqin_page_addr = page_addr;
		break;
	case NAND_CMD_PAGEPROG:
		if (info->seqin_column < mtd->writesize)
			nand_writel(info, NAND_CMD, SPARE | MAIN | WRITE);
		else
			nand_writel(info, NAND_CMD, SPARE | WRITE);
		nand_writel(info, NAND_ADDR_L, MAIN_SPARE_ADDRL(info->seqin_page_addr) + info->seqin_column);
		nand_writel(info, NAND_ADDR_H, MAIN_SPARE_ADDRH(info->seqin_page_addr));
		nand_writel(info, NAND_OPNUM, info->buf_count);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (info->buf_count << 16)); /* 1C注意 */
		start_dma_nand(1, info);
		break;
	case NAND_CMD_RESET:
		info->buf_count = 0x0;
		info->buf_start = 0x0;
		nand_writel(info, NAND_CMD, RESET);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			ls1x_nand_stop(info);
		}
		break;
	case NAND_CMD_ERASE1:
		info->buf_count = 0x0;
		info->buf_start = 0x0;
		nand_writel(info, NAND_ADDR_L, MAIN_ADDRL(page_addr));
		nand_writel(info, NAND_ADDR_H, MAIN_ADDRH(page_addr));
		nand_writel(info, NAND_OPNUM, 0x01);
		nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xc000ffff) | (0x1 << 16)); /* 1C注意 */
		nand_writel(info, NAND_CMD, ERASE);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			ls1x_nand_stop(info);
		}
		break;
	case NAND_CMD_STATUS:
		info->buf_count = 0x1;
		info->buf_start = 0x0;
		nand_writel(info, NAND_CMD, READ_STATUS);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			ls1x_nand_stop(info);
		}
		*(info->data_buff) = (nand_readl(info, NAND_IDH) >> 8) & 0xff;
		*(info->data_buff) |= 0x80;
		break;
	case NAND_CMD_READID:
		info->buf_count = 0x5;
		info->buf_start = 0;
		nand_writel(info, NAND_CMD, READ_ID);
		ls1x_nand_start(info);
		if (!ls1x_nand_done(info)) {
			printf("Wait time out!!!\n");
			ls1x_nand_stop(info);
			break;
		}
		*(info->data_buff) = nand_readl(info, NAND_IDH);
		*(info->data_buff + 1) = (nand_readl(info, NAND_IDL) >> 24) & 0xff;
		*(info->data_buff + 2) = (nand_readl(info, NAND_IDL) >> 16) & 0xff;
		*(info->data_buff + 3) = (nand_readl(info, NAND_IDL) >> 8) & 0xff;
		*(info->data_buff + 4) = nand_readl(info, NAND_IDL) & 0xff;
		break;
	case NAND_CMD_ERASE2:
	case NAND_CMD_READ1:
		info->buf_count = 0x0;
		info->buf_start = 0x0;
		break;
	case NAND_CMD_RNDOUT:
		info->buf_count = mtd->oobsize + mtd->writesize;
		info->buf_start = column;
		break;
	default :
		info->buf_count = 0x0;
		info->buf_start = 0x0;
		printf("ls1x nand non-supported command %x.\n", command);
		break;
	}

	nand_writel(info, NAND_CMD, 0x00);
}

static int ls1x_nand_init_buff(struct ls1x_nand_info *info)
{
	info->dma_desc_size = ALIGN(DMA_DESC_NUM, PAGE_SIZE);	/* 申请内存大小，页对齐 */
	info->dma_desc = (void __iomem *)(((unsigned int)malloc(info->dma_desc_size) & 0x0fffffff) | 0xa0000000);
	info->dma_desc = (void __iomem *)ALIGN((unsigned int)info->dma_desc, 32);	/* 地址32字节对齐 */
	if (!info->dma_desc) {
		dev_err(&pdev->dev,"fialed to allocate memory\n");
		return -ENOMEM;
	}
	info->dma_desc_phys = virt_to_phys(info->dma_desc);

//	info->data_buff_size = MAX_BUFF_SIZE;
	info->data_buff_size = ALIGN(MAX_BUFF_SIZE, PAGE_SIZE);	/* 申请内存大小，页对齐 */
	info->data_buff = (unsigned char *)(((unsigned int)malloc(info->data_buff_size) & 0x0fffffff) | 0xa0000000);
	info->data_buff = (unsigned char *)ALIGN((unsigned int)info->data_buff, 32);	/* 地址32字节对齐 */
	if (!info->data_buff) {
		dev_err(&pdev->dev, "failed to allocate dma buffer\n");
		return -ENOMEM;
	}
	info->data_buff_phys = virt_to_phys(info->data_buff);

	order_addr_in = (unsigned int *)ORDER_ADDR_IN;
	
	return 0;
}

static void ls1x_dma_init(struct ls1x_nand_info *info)
{
	writel(0, info->dma_desc + DMA_ORDERED);
	writel(info->data_buff_phys, info->dma_desc + DMA_SADDR);
	writel(DMA_ACCESS_ADDR, info->dma_desc + DMA_DADDR);
//	writel((info->buf_count + 3) / 4, info->dma_desc + DMA_LENGTH);
	writel(0, info->dma_desc + DMA_STEP_LENGTH);
	writel(1, info->dma_desc + DMA_STEP_TIMES);
	writel(0, info->dma_desc + DMA_CMD);
}

static void ls1x_nand_init_hw(struct ls1x_nand_info *info)
{
	nand_writel(info, NAND_CMD, 0x00);
	nand_writel(info, NAND_ADDR_L, 0x00);
	nand_writel(info, NAND_ADDR_H, 0x00);
	nand_writel(info, NAND_TIMING, (HOLD_CYCLE << 8) | WAIT_CYCLE);
#if defined(CONFIG_CPU_LOONGSON1A) || defined(CONFIG_CPU_LOONGSON1B)
	nand_writel(info, NAND_PARAM, 0x00000100);	/* 设置外部颗粒大小，1A 2Gb? */
#elif defined(CONFIG_CPU_LOONGSON1C)
	nand_writel(info, NAND_PARAM, 0x08005000);
#endif
	nand_writel(info, NAND_OPNUM, 0x00);
	nand_writel(info, NAND_CS_RDY, 0x88442211);	/* 重映射rdy1/2/3信号到rdy0 rdy用于判断是否忙 */
}

static int ls1x_nand_scan(struct mtd_info *mtd)
{
	struct ls1x_nand_info *info = mtd->priv;
	struct nand_chip *chip = (struct nand_chip *)info;
	uint64_t chipsize;
	int exit_nand_size;

	if (nand_scan_ident(mtd, 1, NULL))
		return -ENODEV;

	chipsize = (chip->chipsize << 3) >> 20;	/* Mbit */

	switch (mtd->writesize) {
	case 2048:
		switch (chipsize) {
		case 1024:
		#if defined(CONFIG_CPU_LOONGSON1A)
			exit_nand_size = 0x1;
		#else
			exit_nand_size = 0x0;
		#endif
			break;
		case 2048:
			exit_nand_size = 0x1; break;
		case 4096:
			exit_nand_size = 0x2; break;
		case 8192:
			exit_nand_size = 0x3; break;
		default:
			exit_nand_size = 0x3; break;
		}
		break;
	case 4096:
		exit_nand_size = 0x4; break;
	case 8192:
		switch (chipsize) {
		case 32768:
			exit_nand_size = 0x5; break;
		case 65536:
			exit_nand_size = 0x6; break;
		case 131072:
			exit_nand_size = 0x7; break;
		default:
			exit_nand_size = 0x8; break;
		}
		break;
	case 512:
		switch (chipsize) {
		case 64:
			exit_nand_size = 0x9; break;
		case 128:
			exit_nand_size = 0xa; break;
		case 256:
			exit_nand_size = 0xb; break;
		case 512:
			exit_nand_size = 0xc;break;
		default:
			exit_nand_size = 0xd; break;
		}
		break;
	default:
		printf("exit nand size error!\n");
		return -ENODEV;
	}
	nand_writel(info, NAND_PARAM, (nand_readl(info, NAND_PARAM) & 0xfffff0ff) | (exit_nand_size << 8));
	chip->cmdfunc(mtd, NAND_CMD_RESET, 0, 0);

	return nand_scan_tail(mtd);
}

static int ls1x_nand_chip_init(int devnum, ulong base_addr)
{
	struct ls1x_nand_info *info;
	struct nand_chip *chip;
	struct mtd_info *mtd;
	int ret = 0;

	mtd = &nand_info[devnum];

	info = kzalloc(sizeof(struct ls1x_nand_info), GFP_KERNEL);
	if (!info) {
		printf("failed to allocate memory\n");
		return -ENOMEM;
	}

	chip = (struct nand_chip *)info;
	info->mtd = mtd;
	mtd->priv = info;
//	mtd->owner = THIS_MODULE;

	chip->options		= NAND_CACHEPRG;
//	chip->ecc.mode		= NAND_ECC_NONE;
//	chip->ecc.mode		= NAND_ECC_SOFT_BCH;
	chip->ecc.mode		= NAND_ECC_SOFT;
//	chip->controller	= &info->controller;
	chip->waitfunc		= ls1x_nand_waitfunc;
	chip->select_chip	= ls1x_nand_select_chip;
	chip->dev_ready		= ls1x_nand_dev_ready;
	chip->cmdfunc		= ls1x_nand_cmdfunc;
	chip->read_word		= ls1x_nand_read_word;
	chip->read_byte		= ls1x_nand_read_byte;
	chip->read_buf		= ls1x_nand_read_buf;
	chip->write_buf		= ls1x_nand_write_buf;
	chip->verify_buf	= ls1x_nand_verify_buf;

	info->mmio_base = (unsigned int *)LS1X_NAND_BASE;

	if (ls1x_nand_init_buff(info)) {
		printf("\n\nerror: PMON nandflash driver have some error!\n\n");
		return -ENXIO;
	}

	nand_gpio_init();
	ls1x_dma_init(info);
	ls1x_nand_init_hw(info);

	if (ls1x_nand_scan(mtd)) {
		printf("failed to scan nand\n");
		return -ENXIO;
	}

	mtd->name = "ls1x-nand";

	ret = nand_register(0);
	if (ret)
		return ret;

	return 0;
}

#ifndef CONFIG_SYS_NAND_BASE_LIST
#define CONFIG_SYS_NAND_BASE_LIST { CONFIG_SYS_NAND_BASE }
#endif

static unsigned long base_address[CONFIG_SYS_MAX_NAND_DEVICE] =
	CONFIG_SYS_NAND_BASE_LIST;

void board_nand_init(void)
{
	int i;

	for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++)
		ls1x_nand_chip_init(i, base_address[i]);
}
