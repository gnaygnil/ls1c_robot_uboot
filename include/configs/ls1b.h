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

/*
 * UART
 */
#define CONFIG_CPU_UART
#define CONFIG_BAUDRATE			115200
#define CONFIG_CONS_INDEX	1

#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	 1
#define CONFIG_SYS_NS16550_CLK	 3686400 
#define CONFIG_SYS_NS16550_COM1	 0xbfe48000


/*
 * SDRAM
 */
#define CONFIG_SYS_SDRAM_BASE		0x80000000
//#define CONFIG_SYS_MBYTES_SDRAM		128
#define CONFIG_SYS_MEMTEST_START	0x80200000
#define CONFIG_SYS_MEMTEST_END		0x80400000
#define CONFIG_SYS_LOAD_ADDR		0x81000000	/* default load address */
#define CONFIG_DDR16BIT 1
#define CONFIG_MEM_SIZE 0x04000000

/*
 * Command line configuration.
 */
//#include <config_cmd_default.h>


/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
//#define CONFIG_BOOTP_SUBNETMASK


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


/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CONFIG_SYS_DCACHE_SIZE		8*1024
#define CONFIG_SYS_ICACHE_SIZE		8*1024
#define CONFIG_SYS_CACHELINE_SIZE	32


#define CONFIG_NET_MULTI
#define CONFIG_CONSOLE_MUX
#define CONFIG_BOOTDELAY	5	/* autoboot after 5 seconds	*/
#define CONFIG_SHOW_BOOT_PROGRESS 
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CONFIG_SYS_NO_FLASH	1

//#define CONFIG_SYS_MAX_FLASH_BANKS	1	/* max number of memory banks */
//#define CONFIG_SYS_MAX_FLASH_SECT	(128)	/* max number of sectors on one chip */

#define PHYS_FLASH_1		0xbfc00000 /* Flash Bank #1 */

/* The following #defines are needed to get flash environment right */
#define CONFIG_SYS_FLASH_BASE		PHYS_FLASH_1

/* timeout values are in ticks */
//#define CONFIG_SYS_FLASH_ERASE_TOUT	(20 * CONFIG_SYS_HZ) /* Timeout for Flash Erase */
//#define CONFIG_SYS_FLASH_WRITE_TOUT	(2 * CONFIG_SYS_HZ) /* Timeout for Flash Write */

//#define CONFIG_ENV_IS_IN_EEPROM	1
//#define CONFIG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_IS_IN_NVRAM	1

/* Address and size of Primary Environment Sector	*/
#define CONFIG_ENV_ADDR		0xBFC40000
#define CONFIG_ENV_SIZE		0x20000

/* Address and size of Primary Environment Sector	*/
//#define CONFIG_SPI 1
//#define CONFIG_LOONGSON1B_SPI 1
//#define CONFIG_ENV_OFFSET		0x80000
//#define CONFIG_ENV_SIZE		0x10000
//#define CONFIG_CMD_ENV
//#define CONFIG_CMD_EEPROM


/*USB*/
//#define CONFIG_CMD_USB
//#define CONFIG_DOS_PARTITION
//#define CONFIG_USB_STORAGE
//#define CONFIG_SUPPORT_VFAT
//#define CONFIG_CMD_ELF
//#define CONFIG_CMD_EXT2
//#define CONFIG_BOOTM_LINUX
//#define CONFIG_SYS_USB_EVENT_POLL 
//#define CONFIG_USB_KEYBOARD 
//#define CONFIG_CMD_FAT

/*USB/EHCI*/
//#define CONFIG_USB_EHCI
//#define CONFIG_USB_EHCI_LS1AB
//#define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS 15
//#define CONFIG_EHCI_DCACHE
/*USB/OHCI*/
//#define CONFIG_USB_OHCI_NEW
//#define CONFIG_SYS_USB_OHCI_BOARD_INIT
//#define CONFIG_SYS_USB_OHCI_CPU_INIT
//#define CONFIG_SYS_USB_OHCI_REGS_BASE 0xbfe08000
//#define  CONFIG_SYS_USB_OHCI_SLOT_NAME "ls1b_ohci"
//#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	15

//#define CONFIG_CMD_NAND
//#define CONFIG_LS1G_NAND
//#define CONFIG_SYS_MAX_NAND_DEVICE 1
//#define CONFIG_SYS_NAND_BASE 0x00000000 //not real address
//#define CONFIG_YAFFS2
//#define CONFIG_YAFFS_YAFFS2
//#define CONFIG_KEYBOARD 1
#define CONFIG_GMAC 1
#define CONFIG_NET_RETRY_COUNT          5

//#define CONFIG_CMD_NAND_YAFFS
//#define CONFIG_CMD_MTDPARTS
//#define CONFIG_MTD_PARTITIONS
//#define CONFIG_MTD_DEVICE
//#define CONFIG_CMD_UBI
#define CONFIG_RBTREE
//#define MTDIDS_DEFAULT "nand0=ls1g-nand"
//#define MTDPARTS_DEFAULT "mtdparts=ls1g-nand:21m(yaffs2)," \
                     "-(rest)"

//#include <config_cmd_default.h>
#define CONFIG_CMD_PING
#define CONFIG_CMD_ELF
//#define CONFIG_CMD_DHCP
#define CONFIG_CMD_NET

#define CONFIG_VIDEO_LOGO
#define CONFIG_VIDEO_BMP_LOGO


#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootcmd=nboot 84200000 0x0 0x0\0"\
	"autostart=yes\0"\
        "serverip=172.17.101.9\0" \
        "ipaddr=172.17.101.198\0" \
        "ethaddr=10:84:7F:B5:9D:Fc\0" \
        "bootargs=cpuclock=133000000 rdinit=/sbin/init console=ttyS0,115200\0" \
        "stdin=vga,serial\0"    \
        "stdout=vga,serial\0"   \
        "stderr=vga,serial\0\0"   


#endif	/* __CONFIG_H */
