/*
 * Board initialize code for Loongson1.
 *
 * (C) 2014 Tang Haifeng <tanghaifeng-gz@loongson.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/reboot.h>

#include <asm/ls1x.h>
#include <asm/arch/regs-clk.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif

void _machine_restart(void)
{
	int wdt_base = LS1X_WDT_BASE;
	
	writel(1, wdt_base + WDT_EN);
	writel(0x5000000, wdt_base + WDT_TIMER);
	writel(1, wdt_base + WDT_SET);
	
	while (1) {
		__asm__(".set push;\n"
			".set mips3;\n"
			"wait;\n"
			".set pop;\n"
		);
	}
}

phys_size_t initdram(int board_type)
{
	return get_ram_size ((long *)CONFIG_SYS_SDRAM_BASE, CONFIG_MEM_SIZE);
}

int checkboard(void)
{
	set_io_port_base(0x0);

	printf("checkboard\n");
	printf("Board: %s ", CONFIG_CPU_NAME);
	printf("(CPU Speed %ld MHz/ Mem @ %ld MHz/ Bus @ %ld MHz)\n", gd->cpu_clk/1000000, gd->mem_clk/1000000, gd->bus_clk/1000000);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int ret = 0;

#if defined(CONFIG_CPU_LOONGSON1A)
	*((volatile unsigned int*)0xbfd00420) &= ~0x00800000;	/* 使能GMAC0 */
	#ifdef CONFIG_GMAC0_100M
	*((volatile unsigned int*)0xbfd00420) |= 0x500;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00420) &= ~0x500;		/* 否则配置成千兆模式 */
	#endif
	if (synopGMACMappedAddr == 0xbfe20000) {
		*((volatile unsigned int*)0xbfd00420) &= ~0x01000000;	/* 使能GMAC1 */
		#ifdef CONFIG_GMAC1_100M
		*((volatile unsigned int*)0xbfd00420) |= 0xa00;		/* 配置成百兆模式 */
		#else
		*((volatile unsigned int*)0xbfd00420) &= ~0xa00;		/* 否则配置成千兆模式 */
		#endif
		#ifdef GMAC1_USE_UART01
		*((volatile unsigned int*)0xbfd00420) |= 0xc0;
		#else
		*((volatile unsigned int*)0xbfd00420) &= ~0xc0;
		#endif
	}
#elif defined(CONFIG_CPU_LOONGSON1B)
	/* 寄存器0xbfd00424有GMAC的使能开关 */
	*((volatile unsigned int*)0xbfd00424) &= ~0x1000;	/* 使能GMAC0 */
	#ifdef CONFIG_GMAC0_100M
	*((volatile unsigned int*)0xbfd00424) |= 0x5;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00424) &= ~0x5;	/* 否则配置成千兆模式 */
	#endif
#elif defined(CONFIG_CPU_LOONGSON1C)
	*((volatile unsigned int *)0xbfd00424) &= ~(7 << 28);
#ifdef RMII
    *((volatile unsigned int *)0xbfd00424) |= (1 << 30); //wl rmii
#endif
#endif

#if defined(CONFIG_DESIGNWARE_ETH)
	u32 interface = PHY_INTERFACE_MODE_MII;
	if (designware_initialize(0, LS1X_GMAC0_BASE, CONFIG_DW0_PHY,
				interface) >= 0)
		ret++;
#endif
	return ret;
}

static void calc_clocks(void)
{
	unsigned long pll_freq;

#if defined(CONFIG_CPU_LOONGSON1A)
	{
//		int val= readl(LS1X_CLK_PLL_FREQ);
//		gd->cpu_clk = ((val&7)+1)*APB_CLK;
//		gd->mem_clk = (((val>>8)&7)+3)*APB_CLK;

//		unsigned int val = strtoul(getenv("pll_reg0"), 0, 0);
		gd->cpu_clk = ((val&7)+4)*APB_CLK;
		gd->mem_clk = (((val>>8)&7)+3)*APB_CLK;
		gd->bus_clk = gd->mem_clk / 2;
	}
#elif defined(CONFIG_CPU_LOONGSON1B)
	{
		unsigned int pll = readl(LS1X_CLK_PLL_FREQ);
		unsigned int ctrl = readl(LS1X_CLK_PLL_DIV);
		pll_freq = (12 + (pll & 0x3f)) * APB_CLK / 2 + ((pll >> 8) & 0x3ff) * APB_CLK / 1024 / 2;
		gd->cpu_clk = pll_freq / ((ctrl & DIV_CPU) >> DIV_CPU_SHIFT);
		gd->mem_clk = pll_freq / ((ctrl & DIV_DDR) >> DIV_DDR_SHIFT);
		gd->bus_clk = gd->mem_clk / 2;
		gd->arch.pll_clk = pll_freq;
	}
#elif defined(CONFIG_CPU_LOONGSON1C)
	{
		unsigned int pll_freq = readl(LS1X_CLK_PLL_FREQ);
		unsigned int clk_div = readl(LS1X_CLK_PLL_DIV);
		pll_freq = ((pll_freq >> 8) & 0xff) * APB_CLK / 4;
		if (clk_div & DIV_CPU_SEL) {
			if(clk_div & DIV_CPU_EN) {
				gd->cpu_clk = pll_freq / ((clk_div & DIV_CPU) >> DIV_CPU_SHIFT);
			} else {
				gd->cpu_clk = pll_freq / 2;
			}
		} else {
			gd->cpu_clk = APB_CLK;
		}
		gd->mem_clk = gd->cpu_clk / ((1 << ((pll_freq & 0x3) + 1)) % 5);
		gd->bus_clk = gd->mem_clk;
		gd->arch.pll_clk = pll_freq;
	}
#endif
}

int board_early_init_f(void)
{
	calc_clocks();

	return 0;
}

#if defined(CONFIG_USB_OHCI_LS1X) && defined(CONFIG_SYS_USB_OHCI_BOARD_INIT)
static usb_inited = 0;

int usb_board_init(void)
{
	if (!usb_inited) {
		/*end usb reset*/
#if defined(CONFIG_CPU_LOONGSON1A)
		/* enable USB */
		*(volatile int *)0xbfd00420 &= ~0x200000;
		/*ls1a usb reset stop*/
		*(volatile int *)0xbff10204 |= 0x40000000;
#elif defined(CONFIG_CPU_LOONGSON1B)
		/* enable USB */
		*(volatile int *)0xbfd00424 &= ~0x800;
		/*ls1b usb reset stop*/
		*(volatile int *)0xbfd00424 |= 0x80000000;
#elif defined(CONFIG_CPU_LOONGSON1C)
		*(volatile int *)0xbfd00424 &= ~(1 << 31);
		udelay(100);
		*(volatile int *)0xbfd00424 |= (1 << 31);
#endif
		usb_inited = 1;
	}
	return 0;
}

int usb_board_stop(void)
{
	return 0;
}

int usb_board_init_fail(void)
{
	return 0;
}
#endif
