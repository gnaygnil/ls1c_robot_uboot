/*
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * This file contains the configuration parameters for the ls1a board.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

//#define DEBUG  1

#define CONFIG_MIPS32		1
#define CONFIG_CPU_LOONGSON1
#define CONFIG_CPU_LOONGSON1A
#define CONFIG_BOARD_NAME	"ls1a core board ver:1.0"
#define LS1ASOC 1

#define LS1X_SPI_SFC_PARAM	0x0b	/* 设置SPI flash控制器的模式，加快启动
									div 2, double I/O + burst_en + memory_en 模式
									部分SPI flash可能不支持 导致启动不了 根据使用的spi flash型号修改 */
									
#define OSC_CLK		33000000 /* Hz */
#define CPU_MULT		8			// CPU倍频
#define DDR_MULT		4			// DDR倍频
#define COREPLL_CFG	(0x8888 | (CPU_MULT-4) | ((DDR_MULT-3)<<8))
#define APB_CLOCK_RATE	((((COREPLL_CFG >> 8) & 7) + 3) * OSC_CLK)

#ifndef CPU_CLOCK_RATE
#define CPU_CLOCK_RATE	(((COREPLL_CFG & 7) + 4) * OSC_CLK)	/* MHz clock for the MIPS core */
#endif
#define CPU_TCLOCK_RATE CPU_CLOCK_RATE 
#define CONFIG_SYS_MIPS_TIMER_FREQ	(CPU_TCLOCK_RATE / 2)
#define CONFIG_SYS_HZ			1000

/* Cache Configuration */
#define CONFIG_SYS_DCACHE_SIZE		(16*1024)
#define CONFIG_SYS_ICACHE_SIZE		(16*1024)
#define CONFIG_SYS_CACHELINE_SIZE	32

/* Miscellaneous configurable options */
#define CONFIG_SYS_MAXARGS 16	/* max number of command args */
#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_PROMPT "uboot# "
#define CONFIG_SYS_CBSIZE 256 /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)	/* Print Buffer Size */

#define CONFIG_SYS_MONITOR_BASE	CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_MALLOC_LEN		(16 * 1024 * 1024)
#define CONFIG_SYS_BOOTPARAMS_LEN	(16 * 1024 * 1024)

/* memory */
#define CONFIG_SYS_SDRAM_BASE		0x80000000	/* Cached addr */
#define CONFIG_SYS_INIT_SP_OFFSET	0x00040000
#define CONFIG_SYS_LOAD_ADDR		0x82000000
#define CONFIG_SYS_MEMTEST_START	0x80100000
#define CONFIG_SYS_MEMTEST_END		0x80800000
//#define CONFIG_DDR16BIT 1
#define EIGHT_BANK_MODE 1
#define CONFIG_MEM_SIZE 0x10000000

#define CONFIG_SYS_MIPS_CACHE_MODE CONF_CM_CACHABLE_NONCOHERENT

/* misc settings */
#define CONFIG_BOARD_EARLY_INIT_F 1	/* call board_early_init_f() */

/* GPIO */
#define CONFIG_LS1X_GPIO

/* UART */
#define CONFIG_CPU_UART
#define CONFIG_BAUDRATE			115200
#define CONFIG_CONS_INDEX	2

#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	 1
#define CONFIG_SYS_NS16550_CLK	0
#define CONFIG_SYS_NS16550_COM2	 0xbfe48000
#define UART_BASE_ADDR	CONFIG_SYS_NS16550_COM2

/* SPI Settings */
#define CONFIG_LS1X_SPI
//#define CONFIG_LS1X_SPI1_ENABLE	/* 使用spi1控制器(复用设置), spi1控制器复用can，如果不使用spi1，最好把该选项关闭 */
#define CONFIG_SPI_CS
//#define CONFIG_SPI_CS_USED_GPIO
#define CONFIG_SF_DEFAULT_SPEED	30000000
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SPI_FLASH_GIGADEVICE
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI

/* Flash Settings */
#define CONFIG_SYS_NO_FLASH	1

/* Env Storage Settings */
#if 1
/*
#define CONFIG_ENV_IS_IN_NVRAM	1
#define CONFIG_ENV_ADDR		0xBFC40000
#define CONFIG_ENV_SIZE		0x20000
*/
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SPI_CS	0
#define CONFIG_ENV_SPI_MAX_HZ	30000000
#define CONFIG_ENV_OFFSET	0x68000	/* 保留0x18000 */
#define CONFIG_ENV_SIZE		0x2000	/* 8KB */
#define CONFIG_ENV_SECT_SIZE	256
#else
#define CONFIG_ENV_IS_IN_NAND
#define CONFIG_ENV_OFFSET	0x40000
#define CONFIG_ENV_SIZE		0x20000
#endif

#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"mtdids=" MTDIDS_DEFAULT "\0"					\
	"mtdparts=" MTDPARTS_DEFAULT "\0"				\
	"serverip=192.168.1.3\0" \
	"ipaddr=192.168.1.2\0" \
	"ethaddr=10:84:7F:B5:9D:Fc\0" \
	"panel=" "vesa800x600@75" "\0" \

#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC
/* SPI_MMC Settings */
#define CONFIG_MMC_SPI
#define CONFIG_MMC_SPI_BUS 1
#define CONFIG_MMC_SPI_CS 0
#define CONFIG_CMD_MMC_SPI

/* RTC configuration */
#define CONFIG_RTC_LS1X
//#define CONFIG_RTC_TOY_LS1X
#define CONFIG_CMD_DATE

/* NAND settings */
#define CONFIG_SYS_NAND_SELF_INIT
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE	0xbfe78000
#define CONFIG_CMD_NAND
#define CONFIG_NAND_LS1X
#define CONFIG_NAND_USE_LPC_PWM01	//for LS1A
/* support for yaffs */
#define CONFIG_CMD_NAND_YAFFS

#define CONFIG_MTD_DEVICE	/* needed for mtdparts commands */
#define CONFIG_MTD_PARTITIONS	/* needed for UBI */
#define MTDIDS_DEFAULT          "nand0=ls1x_nand"
#define MTDPARTS_DEFAULT	"mtdparts=ls1x_nand:"	\
/*						"128k(env),"	*/\
						"14M(kernel),"	\
						"100M(root),"	\
						"-(user)"
#define CONFIG_CMD_MTDPARTS

/* OHCI USB */
#define CONFIG_CMD_USB
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_OHCI
#define CONFIG_USB_OHCI_LS1X
//#define CONFIG_SYS_OHCI_SWAP_REG_ACCESS
//#define CONFIG_SYS_OHCI_USE_NPS		/* force NoPowerSwitching mode */
//#define CONFIG_SYS_USB_OHCI_CPU_INIT
#define CONFIG_SYS_USB_OHCI_BOARD_INIT
#define CONFIG_USB_HUB_MIN_POWER_ON_DELAY	500
#define CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS	1
#define CONFIG_SYS_USB_OHCI_REGS_BASE		0xbfe08000
#define CONFIG_SYS_USB_OHCI_SLOT_NAME		"ls1x-ohci"
#define CONFIG_USB_STORAGE
#endif

/* File System Support */
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT
#define CONFIG_DOS_PARTITION
#define CONFIG_SUPPORT_VFAT

/* Ethernet driver configuration */
#define CONFIG_GMII
#define CONFIG_PHYLIB
#define CONFIG_PHY_ADDR				0
#define CONFIG_LS1X_GMAC
#define CONFIG_DW_GMAC_DEFAULT_DMA_PBL	32
//#define CONFIG_LS1X_GMAC0_100M
#define CONFIG_PHY_REALTEK
#define CONFIG_NET_MULTI

/* Framebuffer and LCD */
//#define CONFIG_PREBOOT
#define CONFIG_VIDEO
#define VIDEO_FB_16BPP_WORD_SWAP
#define CONFIG_VIDEO_LS1X
#define CONFIG_VIDEO_LS1X_VGA_MODEM
#define CONFIG_CFB_CONSOLE
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_SPLASH_SCREEN
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_LOGO
/* lcd背光或供电控制 /board/ls1x/ls1x_video.c文件中lcd控制器初始化后使能背光 */
//#define CONFIG_BACKLIGHT_GPIO 28	//gpio号

/*
 * Command line configuration.
 */
#include <config_cmd_default.h>
#define CONFIG_CMDLINE_EDITING			/* add command line history	*/
#define CONFIG_AUTO_COMPLETE

#define CONFIG_CMD_ELF
#define CONFIG_CMD_NET
#define CONFIG_CMD_MII
#define CONFIG_CMD_PING

/* Other helpful shell-like commands */
#define CONFIG_MD5
#define CONFIG_CMD_MD5SUM

#define CONFIG_BOOTDELAY	2		/* Autoboot after 5 seconds	*/
#define CONFIG_BOOTCOMMAND	"tftp a2000000 uImage\;bootm 82000000"	/* Autoboot command	*/
//#define CONFIG_BOOTCOMMAND	"nboot kernel\;bootm 82000000" /* 注意： nboot默认加载地址为CONFIG_SYS_LOAD_ADDR，CONFIG_SYS_LOAD_ADDR要与bootm的地址一致 */
#define CONFIG_BOOTARGS		"console=ttyS2,115200 root=/dev/mtdblock1 noinitrd init=/linuxrc rootfstype=cramfs video=ls1xfb:480x272-16@60"


#endif	/* CONFIG_H */
