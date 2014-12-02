/*
 * Copyright (C) 2014 Haifeng Tang <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <mmc.h>
#include <linux/compat.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/ls1x.h>

#include "ls1x_mmc.h"

#define MISC_CTRL 0xbfd00424

#define DMA_ACCESS_ADDR	0x1fe6c040	/* DMA对NAND操作的地址 */
#define ORDER_ADDR_IN	0xbfd01160	/* DMA配置寄存器 */
static void __iomem *order_addr_in;
#define DMA_DESC_NUM	4096	/* DMA描述符占用的字节数 7x4 */
/* DMA描述符 */
#define DMA_ORDERED		0x00
#define DMA_SADDR		0x04
#define DMA_DADDR		0x08
#define DMA_LENGTH		0x0c
#define DMA_STEP_LENGTH		0x10
#define DMA_STEP_TIMES		0x14
#define DMA_CMD		0x18

#define SDIO_USE_DMA0 0
#define SDIO_USE_DMA2 1
#define SDIO_USE_DMA3 2

/* 设置sdio控制器使用的dma通道，参考数据手册 */
#define DMA_NUM SDIO_USE_DMA3

/* fat扇区大小512，所以512x32=16KB 需预留16KB给sdio控制器的dma */
#define MAX_BUFF_SIZE	32768	/* 32KByte */

DECLARE_GLOBAL_DATA_PTR;

struct ls1x_sdio_regs {
	unsigned int SDICON;			//0
	unsigned int SDIPRE;			//4
	unsigned int SDICMDARG;		//8
	unsigned int SDICMDCON;		//c
	unsigned int SDICMDSTAT;	//10
	unsigned int SDIRSP0;		//14
	unsigned int SDIRSP1;		//18
	unsigned int SDIRSP2;		//1c
	unsigned int SDIRSP3;		//20
	unsigned int SDITIMER;		//24
	unsigned int SDIBSIZE;		//28
	unsigned int SDIDCON;		//2c
	unsigned int SDIDCNT;		//30
	unsigned int SDIDSTA;		//34
	unsigned int SDIFSTA;		//38
	unsigned int SDIIMSK;		//3c
	unsigned int SDIDATA;		//40
	unsigned int SDIDATA0;		//44
	unsigned int SDIDATA1;		//48
	unsigned int SDIDATA2;		//4c
	unsigned int SDIDATA3;		//50
	unsigned int SDIDATA4;		//54
	unsigned int SDIDATA5;		//58
	unsigned int SDIDATA6;		//5c
	unsigned int SDIDATA7;		//60
	unsigned int SDIINTEN;		//64
};

struct ls1x_mmc_priv {
//	struct ls1x_mmc_regs *regs;
	void __iomem *base;
	int		bus_width:2; /* 0 = 1 bit, 1 = 4 bit, 2 = 8 bit */
	unsigned	clock;	/* current clock in Hz */
	int cd_gpio;
	int wp_gpio;

	void __iomem	*dma_desc;
	dma_addr_t		dma_desc_phys;
	size_t			dma_desc_size;

	unsigned char	*data_buff;
	dma_addr_t		data_buff_phys;
	size_t			data_buff_size;
};

static struct ls1x_sdio_regs *regs = (void *)LS1X_SDIO_BASE;

/**
 * Finish a request
 * @param hw_dev Host interface instance
 *
 * Just a little bit paranoia.
 */
static void ls1x_finish_request(struct ls1x_mmc_priv *priv)
{
	/* TODO ensure the engines are stopped */
}

/**
 * Setup a new clock frequency on this MCI bus
 * @param hw_dev Host interface instance
 * @param nc New clock value in Hz (can be 0)
 * @return New clock value (may differ from 'nc')
 */
static unsigned ls1x_setup_clock_speed(struct ls1x_mmc_priv *priv, unsigned nc)
{
	unsigned clock;
	uint32_t mci_psc;

	if (nc == 0)
		return 0;

	/* Calculate the required prescaler value to get the requested frequency */
	for (mci_psc = 0; mci_psc < 255; mci_psc++) {
		clock = gd->bus_clk / (mci_psc+1);

		if (clock <= nc)
			break;
	}

	if (mci_psc > 255) {
		mci_psc = 255;
		debug("SD/MMC clock might be too high!\n");
	}

	writel(mci_psc, &regs->SDIPRE);

	return clock;
}

/**
 * Reset the MCI engine (the hard way)
 * @param hw_dev Host interface instance
 *
 * This will reset everything in all registers of this unit!
 */
static void ls1x_mci_reset(struct ls1x_mmc_priv *priv)
{
	/* reset the hardware */
	writel(SDICON_SDRESET, &regs->SDICON);
	/* wait until reset it finished */
	while (readl(&regs->SDICON) & SDICON_SDRESET)
		;
}

/**
 * Initialize hard and software
 * @param hw_dev Host interface instance
 * @param mci_dev MCI device instance (might be NULL)
 */
static int ls1x_mci_initialize(struct ls1x_mmc_priv *priv, struct mmc *mmc)
{
	ls1x_mci_reset(priv);

	writel(0xffffffff, &regs->SDIINTEN);
	/* restore last settings */
	priv->clock = ls1x_setup_clock_speed(priv, priv->clock);
	writel(0x00FFFFFF, &regs->SDITIMER);
	writel(512, &regs->SDIBSIZE);
	return 0;
}

/**
 * Prepare engine's bits for the next command transfer
 * @param cmd_flags MCI's command flags
 * @return Register bits for this transfer
 */
static uint32_t ls1x_prepare_command_setup(unsigned cmd_flags)
{
	uint32_t reg = SDICMDCON_SENDERHOST;

	if (cmd_flags & MMC_RSP_PRESENT) {
		reg |= SDICMDCON_WAITRSP;
		debug("Command with response\n");
	}
	if (cmd_flags & MMC_RSP_136) {
		reg |= SDICMDCON_LONGRSP;
		debug("Command with long response\n");
	}
/*	if (cmd_flags & MMC_RSP_CRC) {
//		reg |= SDICMDCON_CRCCHECK;
//		debug("Command with crc check\n");
	}
	if (cmd_flags & MMC_RSP_BUSY)
		; 
	if (cmd_flags & MMC_RSP_OPCODE)
		;*/

	return reg;
}

/**
 * Prepare engine's bits for the next data transfer
 * @param hw_dev Host interface device instance
 * @param data_flags MCI's data flags
 * @return Register bits for this transfer
 */
static uint32_t ls1x_prepare_data_setup(struct ls1x_mmc_priv *priv, unsigned data_flags)
{
	uint32_t reg = 0;

	if (priv->bus_width == 1)
		reg |= SDIDCON_WIDEBUS;

	/* enable any kind of data transfers on demand only */
	if (data_flags & MMC_DATA_WRITE) {
//		reg |= SDIDCON_TXAFTERRESP | SDIDCON_XFER_TXSTART;
	}

	if (data_flags & MMC_DATA_READ) {
//		reg |= SDIDCON_RXAFTERCMD | SDIDCON_XFER_RXSTART;
	}

	reg |= SDIDCON_DMAEN | SDIDCON_DATSTART;

	return reg;
}

/**
 * Terminate a current running transfer
 * @param hw_dev Host interface device instance
 * @return 0 on success
 *
 * Note: Try to stop a running transfer. This should not happen, as all
 * transfers must complete in this driver. But who knows... ;-)
 */
static int ls1x_terminate_transfer(struct ls1x_mmc_priv *priv)
{
	unsigned stoptries = 3;

	while (readl(&regs->SDIDSTA) & (SDIDSTA_TXDATAON | SDIDSTA_RXDATAON)) {
		debug("Transfer still in progress.\n");

		writel(SDIDCON_STOP, &regs->SDIDCON);
		ls1x_mci_initialize(priv, NULL);

		if ((stoptries--) == 0) {
			debug("Cannot stop the engine!\n");
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * Setup registers for data transfer
 * @param hw_dev Host interface device instance
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 */
static int ls1x_prepare_data_transfer(struct ls1x_mmc_priv *priv, struct mmc_data *data)
{
	uint32_t reg;

	writel(data->blocksize, &regs->SDIBSIZE);
	reg = ls1x_prepare_data_setup(priv, data->flags);
	reg |= (data->blocks & SDIDCON_BLKNUM);
	writel(reg, &regs->SDIDCON);
	writel(0x00FFFFFF, &regs->SDITIMER);

	return 0;
}

/**
 * Send a command and receive the response
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 */
static int ls1x_send_command(struct ls1x_mmc_priv *priv, struct mmc_cmd *cmd,
				struct mmc_data *data)
{
	uint32_t reg;
	int rc;

	writel(0x00FFFFFF, &regs->SDITIMER);

	/* setup argument */
	writel(cmd->cmdarg, &regs->SDICMDARG);

	/* setup command and transfer characteristic */
	reg = ls1x_prepare_command_setup(cmd->resp_type);
	reg |= cmd->cmdidx & SDICMDCON_INDEX;

	/* run the command right now */
	writel(reg | SDICMDCON_CMDSTART, &regs->SDICMDCON);
	udelay(5);
	/* wait until command is done */
#if 0
	/* 分频系数调低后（即sdio时钟比较高），使用下列判断命令是否发送成功，但判断总是失败 */
	while (1) {
		reg = readl(&regs->SDICMDSTAT);
		/* done? */
		if (cmd->resp_type & MMC_RSP_PRESENT) {
			if (reg & SDICMDSTAT_RSPFIN) {
				writel(SDICMDSTAT_RSPFIN, &regs->SDICMDSTAT);
				rc = 0;
				break;
			} 
		} else {
			if (reg & SDICMDSTAT_CMDSENT) {
					writel(SDICMDSTAT_CMDSENT, &regs->SDICMDSTAT);
					rc = 0;
					break;
			} 
		}
		/* timeout? */
		if (reg & SDICMDSTAT_CMDTIMEOUT) {
			writel(SDICMDSTAT_CMDTIMEOUT, &regs->SDICMDSTAT);
			rc = -ETIMEDOUT;
			break;
		}
	}
#else
	while (1) {
		reg = readl(&regs->SDIIMSK);
		if (reg & 0x1c0) {
			break;
		}
	}
	if (reg & 0x40) {
		rc = 0;
	} else {
		printf("cmd sdiimsk err:%x\n", reg);
		rc = -ETIMEDOUT;
	}
#endif
	writel(0xffffffff, &regs->SDIIMSK);

	if ((rc == 0) && (cmd->resp_type & MMC_RSP_PRESENT)) {
		cmd->response[0] = readl(&regs->SDIRSP0);
		cmd->response[1] = readl(&regs->SDIRSP1);
		cmd->response[2] = readl(&regs->SDIRSP2);
		cmd->response[3] = readl(&regs->SDIRSP3);
	}
	/* do not disable the clock! */
	return rc;
}

/**
 * Clear major registers prior a new transaction
 * @param hw_dev Host interface device instance
 * @return 0 on success
 *
 * FIFO clear is only necessary on 2440, but doesn't hurt on 2410
 */
static int ls1x_prepare_engine(struct ls1x_mmc_priv *priv)
{
	int rc;

	rc = ls1x_terminate_transfer(priv);
	if (rc != 0)
		return rc;

	writel(-1, &regs->SDICMDSTAT);
	writel(-1, &regs->SDIDSTA);
	writel(-1, &regs->SDIFSTA);

	return 0;
}

/**
 * Handle MCI commands without data
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @return 0 on success
 *
 * This functions handles the following MCI commands:
 * - "broadcast command (BC)" without a response
 * - "broadcast commands with response (BCR)"
 * - "addressed command (AC)" with response, but without data
 */
static int ls1x_mci_std_cmds(struct ls1x_mmc_priv *priv, struct mmc_cmd *cmd)
{
	int rc;

	rc = ls1x_prepare_engine(priv);
	if (rc != 0)
		return 0;

	return ls1x_send_command(priv, cmd, NULL);
}

static int ls1x_mci_data_check(struct ls1x_mmc_priv *priv)
{
	int sdiintmsk = 0;

	while ((sdiintmsk & 0x1f) == 0) {
		sdiintmsk = readl(&regs->SDIIMSK);
	}
	writel(0xffffffff, &regs->SDIIMSK);

	if (sdiintmsk & 0x1) {
		return 0;
	} else {
		printf("sdiimsk err:%x\n", sdiintmsk);
		return -1;
	}
}

/**
 * Read or Write one block of data into the FIFO
 * @param priv Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
 *
 * We must ensure data in the FIFO when the command phase changes into the
 * data phase. To ensure this, the FIFO gets filled first, then the command.
 */
static int ls1x_mci_block_transfer(struct ls1x_mmc_priv *priv, struct mmc_cmd *cmd,
				struct mmc_data *data)
{
	unsigned int trans_len = data->blocksize * data->blocks;
	int timeout = 5000;
	int ret;

	if (trans_len > MAX_BUFF_SIZE) {
		printf("error: ls1x sdio dma buff too small!\n");
		return -1;
	}

	if (data->flags & MMC_DATA_READ) {
		writel(0x00000001, priv->dma_desc + DMA_CMD);
	} else {
		memcpy(priv->data_buff, data->src, trans_len);
		writel(0x00003001, priv->dma_desc + DMA_CMD);
	}

	writel(0, priv->dma_desc + DMA_ORDERED);
	writel(priv->data_buff_phys, priv->dma_desc + DMA_SADDR);
	writel(DMA_ACCESS_ADDR, priv->dma_desc + DMA_DADDR);
	writel((trans_len + 3) >> 2, priv->dma_desc + DMA_LENGTH);
	writel(0, priv->dma_desc + DMA_STEP_LENGTH);
	writel(1, priv->dma_desc + DMA_STEP_TIMES);

	writel((priv->dma_desc_phys & ~0x1F) | 0x8 | DMA_NUM, order_addr_in);	/* 启动DMA */
	while ((readl(order_addr_in) & 0x8)/* && (timeout-- > 0)*/) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}

	ret = ls1x_send_command(priv, cmd, data);
	if (ret != 0)
		return ret;
#if 0
	/* 分频系数调低后（即sdio时钟比较高），使用下列判断命令是否发送成功，但判断总是失败 */
	while (!(readl(&regs->SDIDSTA) & SDIDSTA_XFERFINISH)) {
	}
#else
	ret = ls1x_mci_data_check(priv);
	if (ret != 0)
		return ret;
#endif

	while (timeout) {
		writel((priv->dma_desc_phys & (~0x1F)) | 0x4 | DMA_NUM, order_addr_in);
		do {
		} while (readl(order_addr_in) & 0x4);
		ret = readl(priv->dma_desc + DMA_CMD);
//		if ((ret & 0x08) || flags) {
		if (ret & 0x08) {
			break;
		}
		timeout--;
	}

	if (!timeout) {
		printf("%s. %x\n",__func__, ret);
		return -EIO;
	}

	if (data->flags & MMC_DATA_READ) {
		memcpy(data->dest, priv->data_buff, trans_len);
	}

	return 0;
}

/**
 * Handle MCI commands with or without data
 * @param hw_dev Host interface device instance
 * @param cmd The command to handle
 * @param data The data information (buffer, direction aso.)
 * @return 0 on success
*/
static int ls1x_mci_adtc(struct ls1x_mmc_priv *priv, struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	int rc;

	rc = ls1x_prepare_engine(priv);
	if (rc != 0)
		return rc;

	rc = ls1x_prepare_data_transfer(priv, data);
	if (rc != 0)
		return rc;

	rc = ls1x_mci_block_transfer(priv, cmd, data);
	if (rc != 0) {
		ls1x_terminate_transfer(priv);
	}

	writel(0, &regs->SDIDCON);

	return rc;
}

static int ls1x_mmc_request(struct mmc *mmc, struct mmc_cmd *cmd,
				struct mmc_data *data)
{
	struct ls1x_mmc_priv *priv = (struct ls1x_mmc_priv *)mmc->priv;
	int rc;

	/* enable clock */
	writel(readl(&regs->SDICON) | SDICON_CLKEN,
		&regs->SDICON);

	if ((cmd->resp_type == 0) || (data == NULL))
		rc = ls1x_mci_std_cmds(priv, cmd);
	else
		rc = ls1x_mci_adtc(priv, cmd, data);	/* with response and data */

	ls1x_finish_request(priv);

	/* disable clock */
	writel(readl(&regs->SDICON) & ~SDICON_CLKEN,
		&regs->SDICON);
	return rc;
}

static void ls1x_mmc_set_ios(struct mmc *mmc)
{
	struct ls1x_mmc_priv *priv = (struct ls1x_mmc_priv *)mmc->priv;
	uint32_t reg;

	switch (mmc->bus_width) {
	case 4:
		priv->bus_width = 1;
		break;
	default :
		priv->bus_width = 0;
		break;
	}

	reg = readl(&regs->SDICON);
	if (mmc->clock) {
		/* setup the IO clock frequency and enable it */
		priv->clock = ls1x_setup_clock_speed(priv, mmc->clock);
		reg |= SDICON_CLKEN;	/* enable the clock */
	} else {
		reg &= ~SDICON_CLKEN;	/* disable the clock */
		priv->clock = 0;
	}
	writel(reg, &regs->SDICON);

	debug("IO settings: bus width=%d, frequency=%u Hz\n",
		priv->bus_width, priv->clock);
}

static int ls1x_mmc_init(struct mmc *mmc)
{
	struct ls1x_mmc_priv *priv = (struct ls1x_mmc_priv *)mmc->priv;

	/* 设置复用 */
	writel(readl(MISC_CTRL) | ((DMA_NUM + 1) << 23) | (1 << 16), MISC_CTRL);

	return ls1x_mci_initialize(priv, mmc);
}

static int ls1x_mmc_dma_init(struct ls1x_mmc_priv *priv)
{
	priv->dma_desc_size = ALIGN(DMA_DESC_NUM, PAGE_SIZE);	/* 申请内存大小，页对齐 */
	priv->dma_desc = (void __iomem *)(((unsigned int)malloc(priv->dma_desc_size) & 0x0fffffff) | 0xa0000000);
	priv->dma_desc = (void __iomem *)ALIGN((unsigned int)priv->dma_desc, 32);	/* 地址32字节对齐 */
	if (!priv->dma_desc) {
		return -ENOMEM;
	}
	priv->dma_desc_phys = virt_to_phys(priv->dma_desc);
	order_addr_in = (unsigned int *)ORDER_ADDR_IN;

//	priv->data_buff_size = MAX_BUFF_SIZE;
	priv->data_buff_size = ALIGN(MAX_BUFF_SIZE, PAGE_SIZE);	/* 申请内存大小，页对齐 */
	priv->data_buff = (unsigned char *)(((unsigned int)malloc(priv->data_buff_size) & 0x0fffffff) | 0xa0000000);
	priv->data_buff = (unsigned char *)ALIGN((unsigned int)priv->data_buff, 32);	/* 地址32字节对齐 */
	if (!priv->data_buff) {
		dev_err(&pdev->dev, "failed to allocate dma buffer\n");
		return -ENOMEM;
	}
	priv->data_buff_phys = virt_to_phys(priv->data_buff);

	writel(0, priv->dma_desc + DMA_ORDERED);
	writel(priv->data_buff_phys, priv->dma_desc + DMA_SADDR);
	writel(DMA_ACCESS_ADDR, priv->dma_desc + DMA_DADDR);
//	writel((data->blocksize * data->blocks + 3) / 4, priv->dma_desc + DMA_LENGTH);
	writel(0, priv->dma_desc + DMA_STEP_LENGTH);
	writel(1, priv->dma_desc + DMA_STEP_TIMES);

	return 0;
}

#if defined(CONFIG_LS1X_GPIO)
static int ls1x_mmc_setup_gpio_in(int gpio, const char *label)
{
	if (gpio_request(gpio, label) < 0)
		return -1;

	if (gpio_direction_input(gpio) < 0)
		return -1;

	return gpio;
}

static int ls1x_mmc_getcd(struct mmc *mmc)
{
	int cd_gpio = ((struct ls1x_mmc_priv *)mmc->priv)->cd_gpio;
	return !gpio_get_value(cd_gpio);
}

static int ls1x_mmc_getwp(struct mmc *mmc)
{
	int wp_gpio = ((struct ls1x_mmc_priv *)mmc->priv)->wp_gpio;
	return !gpio_get_value(wp_gpio);
}
#else
static inline int ls1x_mmc_setup_gpio_in(int gpio, const char *label)
{
	return -1;
}

#define ls1x_mmc_getcd NULL
#define ls1x_mmc_getwp NULL
#endif

int ls1x_mmc_register(int card_index, int cd_gpio,
		int wp_gpio)
{
	struct mmc *mmc;
	struct ls1x_mmc_priv *priv;
	int ret = -ENOMEM;

	mmc = kzalloc(sizeof(struct mmc), GFP_KERNEL);
	if (!mmc)
		goto err0;

	priv = kzalloc(sizeof(struct ls1x_mmc_priv), GFP_KERNEL);
	if (!priv)
		goto err1;

	switch (card_index) {
	case 0:
		priv->base = (unsigned int *)LS1X_SDIO_BASE;
		regs = (void *)LS1X_SDIO_BASE;
		break;
	case 1:
		priv->base = (unsigned int *)LS1X_SDIO_BASE;
		regs = (void *)LS1X_SDIO_BASE;
		break;
	default:
		printf("LS1X MMC: Invalid MMC controller ID (card_index = %d)\n",
			card_index);
		goto err2;
	}

	if (ls1x_mmc_dma_init(priv)) {
		printf("\n\nerror: mmc dma init error!\n\n");
		goto err2;
	}

	mmc->priv = priv;

	sprintf(mmc->name, "LS1X MMC");
	mmc->send_cmd	= ls1x_mmc_request;
	mmc->set_ios	= ls1x_mmc_set_ios;
	mmc->init	= ls1x_mmc_init;
//	mmc->getcd	= NULL;
	priv->cd_gpio = ls1x_mmc_setup_gpio_in(cd_gpio, "mmc_cd");
	if (priv->cd_gpio != -1)
		mmc->getcd = ls1x_mmc_getcd;

	priv->wp_gpio = ls1x_mmc_setup_gpio_in(wp_gpio, "mmc_wp");
	if (priv->wp_gpio != -1)
		mmc->getwp = ls1x_mmc_getwp;

	mmc->voltages	= MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
	mmc->f_max	= gd->bus_clk / 2;
	mmc->f_min	= gd->bus_clk / 256;
	mmc->host_caps	= MMC_MODE_4BIT;

	/* 设置sdio控制器一次传输的最大块数，ls1x sdio的最大块数为
	     4095，驱动中使用dma传输，一次dam传输有大小限制，太大会出现传输失败，
	     这里把b_max设置为32  */
	/* fat扇区大小512，所以512x32=16KB 需预留16KB给sdio控制器的dma */
	mmc->b_max = 32;

	priv->clock = ls1x_setup_clock_speed(priv, mmc->f_min);

	mmc_register(mmc);

	return 0;

err2:
	free(priv);
err1:
	free(mmc);
err0:
	return ret;
}
