/*
 * Board initialize code for Lemote YL8089.
 *
 * (C) Yanhua <yanh@lemote.com> 2009
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/ls1x.h>
//#include <pci.h>
//#include <netdev.h>
//#include <configs/ls1a.h>
//#include <linux/mtd/nand.h>

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
	printf("Boot reached stage %d\n", progress);
#if defined(CONFIG_STATUS_LED) && defined(STATUS_LED_BOOT)
	status_led_set(STATUS_LED_BOOT, STATUS_LED_OFF);
#endif
}
#endif

void _machine_restart(void)
{
	unsigned long hi, lo;
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
	return get_ram_size (CONFIG_SYS_SDRAM_BASE, 0x10000000);
}

int checkboard(void)
{
	int md_pipefreq, md_cpufreq, md_pllfreq;

#if defined(CONFIG_CPU_LOONGSON1A)
	{
//		int val= readl(LS1X_CLK_PLL_FREQ);
//		md_pipefreq = ((val&7)+1)*APB_CLK;
//		md_cpufreq  =  (((val>>8)&7)+3)*APB_CLK;

//		unsigned int val = strtoul(getenv("pll_reg0"), 0, 0);
		md_pipefreq = ((val&7)+4)*APB_CLK;
		md_cpufreq = (((val>>8)&7)+3)*APB_CLK;
	}
#elif defined(CONFIG_CPU_LOONGSON1B)
	{
		unsigned int pll = readl(LS1X_CLK_PLL_FREQ);
		unsigned int ctrl = readl(LS1X_CLK_PLL_DIV);
		md_pllfreq = (12+(pll&0x3f))*APB_CLK/2 + ((pll>>8)&0x3ff)*APB_CLK/2/1024;
		md_pipefreq = ((ctrl&0x300)==0x300) ? APB_CLK : (ctrl&(1<<25)) ? md_pllfreq/((ctrl>>20)&0x1f) : md_pllfreq/2;
		md_cpufreq  = ((ctrl&0xc00)==0xc00) ? APB_CLK : (ctrl&(1<<19)) ? md_pllfreq/((ctrl>>14)&0x1f) : md_pllfreq/2;
	}
#elif defined(CONFIG_CPU_LOONGSON1C)
	{
		unsigned int pll_freq = readl(LS1X_CLK_PLL_FREQ);
		unsigned int clk_div = readl(LS1X_CLK_PLL_DIV);
		md_pllfreq = ((pll_freq >> 8) & 0xff) * APB_CLK / 4;
		if (clk_div & DIV_CPU_SEL) {
			if(clk_div & DIV_CPU_EN) {
				md_pipefreq = md_pllfreq / ((clk_div & DIV_CPU) >> DIV_CPU_SHIFT);
			} else {
				md_pipefreq = md_pllfreq / 2;
			}
		} else {
			md_pipefreq = APB_CLK;
		}
		md_cpufreq  = md_pipefreq / ((1 << ((pll_freq & 0x3) + 1)) % 5);
	}
#endif

	printf("Board: ls1b ");
	printf("(CPU Speed %d MHz/ Bus @ %d MHz)\n", md_pipefreq/1000000, md_cpufreq/1000000);

	set_io_port_base(0x0);

	return 0;
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
/*
int board_early_init_f(void)
{
	printf("in board_early_int_f\n");
#if defined(CONFIG_STATUS_LED) && defined(STATUS_LED_BOOT)
	status_led_set(STATUS_LED_BOOT, STATUS_LED_ON);
#endif
}*/
