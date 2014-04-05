#ifndef __LS1X_H
#define __LS1X_H

#include <asm/addrspace.h>
#include <asm/cacheops.h>

#define uncached(x) KSEG1ADDR(x)
#define tobus(x)    (((unsigned long)(x)&0x1fffffff) )
#define CACHED_MEMORY_ADDR      0x80000000
#define UNCACHED_MEMORY_ADDR    0xa0000000

#define CACHED_TO_PHYS(x)       ((unsigned)(x) & 0x1fffffff)
#define PHYS_TO_CACHED(x)       ((unsigned)(x) | CACHED_MEMORY_ADDR)
#define UNCACHED_TO_PHYS(x)     ((unsigned)(x) & 0x1fffffff)
#define PHYS_TO_UNCACHED(x)     ((unsigned)(x) | UNCACHED_MEMORY_ADDR)
#define VA_TO_CINDEX(x)         ((unsigned)(x) & 0xffffff | CACHED_MEMORY_ADDR)
#define CACHED_TO_UNCACHED(x)   (PHYS_TO_UNCACHED(CACHED_TO_PHYS(x)))
#define UNCACHED_TO_CACHED(x)  (PHYS_TO_CACHED(UNCACHED_TO_PHYS(x)))

/* PLL Controller */
#define LS1X_CLK_PLL_FREQ	0xbfe78030
#define LS1X_CLK_PLL_DIV	0xbfe78034

/* GPIO Controller */
#if defined(LS1ASOC)
#define LS1X_GPIO_COUNT	64
#elif defined(LS1BSOC)
#define LS1X_GPIO_COUNT 96
#elif defined(LS1CSOC)
#define LS1X_GPIO_COUNT 128
#endif

/* GPIO 0-31 group 0 */
#define LS1X_GPIO_CFG0		0xbfd010c0	/* 配置寄存器 */
#define LS1X_GPIO_OE0		0xbfd010d0	/* 输入使能寄存器 */
#define LS1X_GPIO_IN0		0xbfd010e0	/* 输入寄存器 */
#define LS1X_GPIO_OUT0		0xbfd010f0	/* 输出寄存器 */

/* GPIO 32-63 group 1 */
#define LS1X_GPIO_CFG1		0xbfd010c4
#define LS1X_GPIO_OE1		0xbfd010d4
#define LS1X_GPIO_IN1		0xbfd010e4
#define LS1X_GPIO_OUT1		0xbfd010f4

#ifdef LS1ASOC
/* GPIO 64-87 group 2 */
#define LS1X_GPIO_CFG2		0xbfd010c8
#define LS1X_GPIO_OE2		0xbfd010d8
#define LS1X_GPIO_IN2		0xbfd010e8
#define LS1X_GPIO_OUT2		0xbfd010f8
#endif

#ifdef LS1CSOC
/* GPIO 64-95 group 2 */
#define LS1X_GPIO_CFG2		0xbfd010c8
#define LS1X_GPIO_OE2		0xbfd010d8
#define LS1X_GPIO_IN2		0xbfd010e8
#define LS1X_GPIO_OUT2		0xbfd010f8
/* GPIO 96-127 group 3 */
#define LS1X_GPIO_CFG3		0xbfd010cc
#define LS1X_GPIO_OE3		0xbfd010dc
#define LS1X_GPIO_IN3		0xbfd010ec
#define LS1X_GPIO_OUT3		0xbfd010fc
#endif

/* SPI Controller */
#define LS1X_SPI0_BASE	0xbfe80000
#define LS1X_SPI1_BASE	0xbfec0000

/* watchdog */
#if	defined(CONFIG_CPU_LOONGSON1A)
#define LS1X_WDT_BASE				0xbfe7c060
#elif	defined(CONFIG_CPU_LOONGSON1B) || defined(CONFIG_CPU_LOONGSON1C)
#define LS1X_WDT_BASE				0xbfe5c060
#endif
#define WDT_EN						0x00
#define WDT_TIMER					0x04
#define WDT_SET						0x08

/* RTC */
#define LS1X_RTC_BASE		0xbfe64000

/* NAND FLASH */
#define LS1X_NAND_BASE		0xbfe78000

/* ETH GMAC */
#define LS1X_GMAC0_BASE		0xbfe10000
#define LS1X_GMAC1_BASE		0xbfe20000

#endif
