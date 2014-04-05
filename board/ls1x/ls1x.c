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
	return get_ram_size ((long *)CONFIG_SYS_SDRAM_BASE, 0x04000000);
}

int checkboard(void)
{
	set_io_port_base(0x0);

	printf("checkboard\n");
	printf("Board: ls1b ");
	printf("(CPU Speed %ld MHz/ Mem @ %ld MHz/ Bus @ %ld MHz)\n", gd->cpu_clk/1000000, gd->mem_clk/1000000, gd->bus_clk/1000000);
#if defined(CONFIG_STATUS_LED) && defined(STATUS_LED_BOOT)
	status_led_set(STATUS_LED_BOOT, STATUS_LED_ON);
#endif
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
	unsigned int md_pllfreq;

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
		md_pllfreq = (12+(pll&0x3f))*APB_CLK/2 + ((pll>>8)&0x3ff)*APB_CLK/2/1024;
		gd->cpu_clk = ((ctrl&0x300)==0x300) ? APB_CLK : (ctrl&(1<<25)) ? md_pllfreq/((ctrl>>20)&0x1f) : md_pllfreq/2;
		gd->mem_clk = ((ctrl&0xc00)==0xc00) ? APB_CLK : (ctrl&(1<<19)) ? md_pllfreq/((ctrl>>14)&0x1f) : md_pllfreq/2;
		gd->bus_clk = gd->mem_clk / 2;
	}
#elif defined(CONFIG_CPU_LOONGSON1C)
	{
		unsigned int pll_freq = readl(LS1X_CLK_PLL_FREQ);
		unsigned int clk_div = readl(LS1X_CLK_PLL_DIV);
		md_pllfreq = ((pll_freq >> 8) & 0xff) * APB_CLK / 4;
		if (clk_div & DIV_CPU_SEL) {
			if(clk_div & DIV_CPU_EN) {
				gd->cpu_clk = md_pllfreq / ((clk_div & DIV_CPU) >> DIV_CPU_SHIFT);
			} else {
				gd->cpu_clk = md_pllfreq / 2;
			}
		} else {
			gd->cpu_clk = APB_CLK;
		}
		gd->mem_clk = gd->cpu_clk / ((1 << ((pll_freq & 0x3) + 1)) % 5);
		gd->bus_clk = gd->mem_clk;
	}
#endif
}

int board_early_init_f(void)
{
	calc_clocks();

	return 0;
}
