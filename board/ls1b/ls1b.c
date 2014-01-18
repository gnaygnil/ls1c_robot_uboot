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
#include <asm/io.h>
#include <asm/reboot.h>

#include <asm/ls1x.h>
//#include <pci.h>
//#include <netdev.h>
//#include <configs/ls1a.h>
//#include <linux/mtd/nand.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
}
#endif

void _machine_restart(void)
{
	void (*f)(void) = (void *) 0xbfc00000;

#if 0
	/* dark the lcd */
	outb(0xfe, 0xbfd00381);
	outb(0x01, 0xbfd00382);
	outb(0x00, 0xbfd00383);
#endif
	/* Tell EC to reset the whole system */
	outb(0xf4, 0xbfd00381);
	outb(0xec, 0xbfd00382);
	outb(0x01, 0xbfd00383);

	while (1);
	/* Not reach here normally */
	f();
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
}

int board_eth_init(bd_t *bis)
{/*
	int i = 0;
#ifdef CONFIG_GMAC 
	char* name = "syn0";
	unsigned long long synopGMACMappedAddr = 3219193856;
#if 0
	i = gmac_initialize(name, synopGMACMappedAddr);
#else
	#define Gmac_base 0xbfe10000
	i = gmac_initialize(name, Gmac_base);
#endif
#endif
	return i;*/
	return 0;
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
