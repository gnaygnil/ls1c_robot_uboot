/*
 * (C) Copyright 2003
 * Masami Komiya <mkomiya@sonare.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Config header file for TANBAC TB0229 board using an VR4131 CPU module
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define DEBUG  1

#define CONFIG_MIPS32		1	/* MIPS 4Kc CPU core	*/
#define CONFIG_CPU_LOONGSON1
#define CONFIG_CPU_LOONGSON1B
#define APB_CLK 33000000
#define CPU_MULT 7
#define DDR_MULT 4
#define LS1BSOC 1

#define BUZZER 1

#ifndef CPU_CLOCK_RATE
#define CPU_CLOCK_RATE	APB_CLK*CPU_MULT	/* 800 MHz clock for the MIPS core */
#endif
#define CPU_TCLOCK_RATE CPU_CLOCK_RATE 
#define CONFIG_SYS_MIPS_TIMER_FREQ	(CPU_TCLOCK_RATE/4)
#define CONFIG_SYS_HZ			1000


#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_MONITOR_LEN		(192 << 10)
//#define CONFIG_STACKSIZE		(256 << 10)
#define CONFIG_SYS_MALLOC_LEN		(128 << 10)
#define CONFIG_SYS_BOOTPARAMS_LEN	(128 << 10)
#define CONFIG_SYS_INIT_SP_OFFSET	0x600000

/* Cache Configuration */
#define CONFIG_SYS_DCACHE_SIZE		8*1024
#define CONFIG_SYS_ICACHE_SIZE		8*1024
#define CONFIG_SYS_CACHELINE_SIZE	32

/* SDRAM */
#define CONFIG_SYS_SDRAM_BASE		0x80000000
//#define CONFIG_SYS_MBYTES_SDRAM		128
#define CONFIG_SYS_MEMTEST_START	0x80200000
#define CONFIG_SYS_MEMTEST_END		0x80400000
#define CONFIG_SYS_LOAD_ADDR		0x81000000	/* default load address */
#define CONFIG_DDR16BIT 1
#define CONFIG_MEM_SIZE 0x04000000

/* misc settings */
#define CONFIG_BOARD_EARLY_INIT_F 1	/* call board_early_init_f() */

/* GPIO */
#define CONFIG_LS1X_GPIO

/* LED configuration */
#define CONFIG_GPIO_LED
#define CONFIG_STATUS_LED
#define CONFIG_BOARD_SPECIFIC_LED

/* The LED PINs */
/* LED */
#define STATUS_LED_BIT			38
#define STATUS_LED_STATE		STATUS_LED_ON
#define STATUS_LED_PERIOD		(CONFIG_SYS_HZ / 100)

/* LED 1 */
#define STATUS_LED_BIT1			39
#define STATUS_LED_STATE1		STATUS_LED_OFF
#define STATUS_LED_PERIOD1		(CONFIG_SYS_HZ / 100)

/* LED 2 */
#define STATUS_LED_BIT2			40
#define STATUS_LED_STATE2		STATUS_LED_OFF
#define STATUS_LED_PERIOD2		(CONFIG_SYS_HZ / 1000)

/* Boot status LED */
#define STATUS_LED_BOOT			1 /* LED 1 */

/* UART */
#define CONFIG_CPU_UART
#define CONFIG_BAUDRATE			115200
#define CONFIG_CONS_INDEX	1

#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	 1
#define CONFIG_SYS_NS16550_CLK	0
#define CONFIG_SYS_NS16550_COM1	 0xbfe48000

/* SPI Settings */
#define CONFIG_LS1X_SPI
#define CONFIG_SPI_CS
#define CONFIG_SF_DEFAULT_SPEED	30000000
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI

/* Flash Settings */
#define CONFIG_SYS_NO_FLASH	1

/* Env Storage Settings */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SPI_CS	0
#define CONFIG_ENV_SPI_MAX_HZ	30000000
#define CONFIG_ENV_OFFSET	0x7e000	/* 512KB - 8KB */
#define CONFIG_ENV_SIZE		0x2000	/* 8KB */
#define CONFIG_ENV_SECT_SIZE	256	/* 4KB */


/* SPI_MMC Settings */
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC_SPI
#define CONFIG_MMC_SPI_BUS 0
#define CONFIG_MMC_SPI_CS 2
#define CONFIG_CMD_MMC
#define CONFIG_CMD_MMC_SPI

/* RTC configuration */
#define CONFIG_RTC_LS1X
#define CONFIG_CMD_DATE

/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_MTD_PARTITIONS

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME


/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP				/* undef to save memory	     */
#define CONFIG_SYS_PROMPT		"uboot$ "		/* Monitor Command Prompt    */
#define CONFIG_SYS_CBSIZE		256		/* Console I/O Buffer Size   */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)  /* Print Buffer Size */
#define CONFIG_SYS_MAXARGS		16		/* max number of command args*/
#define CONFIG_TIMESTAMP		/* Print image info with timestamp */
#define CONFIG_CMDLINE_EDITING			/* add command line history	*/


#define CONFIG_NET_MULTI
#define CONFIG_CONSOLE_MUX
#define CONFIG_BOOTDELAY	5	/* autoboot after 5 seconds	*/
#define CONFIG_SHOW_BOOT_PROGRESS 
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#endif	/* __CONFIG_H */
