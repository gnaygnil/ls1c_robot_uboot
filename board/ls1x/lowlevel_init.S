/*
 *  Memory sub-system initialization code for Loongson 1.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include <asm/asm.h>

#include <config.h>
#include <asm/regdef.h>
#include <asm/arch/regs-clk.h>

#include "ns16550.h"

/* Delay macro */
#define DELAY(count)	\
	li		t0, count;	\
99:						\
	nop;				\
	subu	t0, 0x1;	\
	bnez	t0, 99b;	\
	nop

#ifdef CONFIG_CPU_UART
#define PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define PRINTSTR(x)
#endif

	.globl  lowlevel_init
lowlevel_init:
	move s0, ra

	/* initialize pll */
#if defined(CONFIG_CPU_LOONGSON1A)
	li  t1, COREPLL_CFG
	li  t2, 0xa
1:
	li  t0, 0xbfe78030
	sw  t1, 0x0(t0)
	nop
	sub  t2, 0x1
	bnez t2, 1b
	nop
#elif defined(CONFIG_CPU_LOONGSON1B)
	li	t0, 0xbfe78030
	li	t1, PLL_FREQ
	li	t2, PLL_DIV
	sw	t1, 0(t0)
	sw	t2, 4(t0)
#elif defined(CONFIG_CPU_LOONGSON1C)
	/* config pll div for cpu and sdram */

	li	t0, 0xbfe78030
	/* 设置PLL倍频 及SDRAM分频 */
	li	t2, PLL_FREQ
	/* 设置CPU分频 */
	li	t3, PLL_DIV
	/* 注意：首先需要把分频使能位清零 */
	li	t1, 0x2
	sw	t1, 0x4(t0)
	sw	t2, 0x0(t0)
	sw	t3, 0x4(t0)
	DELAY(4096)
#endif

	/* 芯片上电默认使用gpio(输入模式）但大多时候是使用模块的功能，如lcd i2c spi ac97等
	   所以这里把gpio都关闭，方便使用模块功能。如果上电后需要gpio输出一个确定电平，
	   如继电器、LDE等，可以修改这里的代码。*/
	/* disable all gpio */
	li a0,0xbfd00000
	sw zero,0x10c0(a0)	/* disable gpio 0-31 */
	sw zero,0x10c4(a0)	/* disable gpio 32-63 */
	sw zero,0x10c8(a0)	/* disable gpio 64-95 */
	sw zero,0x10cc(a0)

	li t0, 0xffffffff
	sw t0, 0x10d0(a0)
	sw t0, 0x10d4(a0)
	sw t0, 0x10d8(a0)
	sw t0, 0x10dc(a0)

	sw t0, 0x10f0(a0)
	sw t0, 0x10f4(a0)
	sw t0, 0x10f8(a0)
	sw t0, 0x10fc(a0)

#ifdef CONFIG_CPU_UART
	li	a0, 0
	bal	initserial
	nop
#endif

	PRINTSTR("U-BOOT Initializing...\r\n");

// use only 8wins
#define CPU_WIN_BASE 0xbfd00000
#define CPU_WIN_MASK 0xbfd00040
#define CPU_WIN_MMAP 0xbfd00080

#define set_cpu_window(id, base, mask, mmap) \
        li      t0, CPU_WIN_BASE          ;  \
        sw      $0, 0x80+id*8(t0)         ;  \
        li      t1, base                  ;  \
        sw      t1, 0x00+id*8(t0)         ;  \
        sw      $0, 0x04+id*8(t0)         ;  \
        li      t1, mask                  ;  \
        sw      t1, 0x40+id*8(t0)         ;  \
        sw      $0, 0x44+id*8(t0)         ;  \
        li      t1, mmap                  ;  \
        sw      t1, 0x80+id*8(t0)         ;  \
        sw      $0, 0x84+id*8(t0)

/* fixup cpu window */
cpu_win_fixup:
	//
	// hit         = (paddr & mask) == (mmap & mask)
	// mapped_addr =  paddr &~mask | mmap & mask
	//
	// mmap[7] -> enable
	// mmap[5] -> block trans enable
	// mmap[4] -> cachable
	// mmap[1:0] -> destination
	//
	// NOTE: the address windows has priority, win0 > win1 > ... > win7
#if defined(CONFIG_CPU_LOONGSON1A)
//	set_cpu_window(0, 0x1fc00000, 0xfff00000, 0x1fc000f3) // boot rom
	set_cpu_window(0, 0x10000000, 0xf8000000, 0x100000d1) // PCI mem0, mem1
	set_cpu_window(1, 0x18000000, 0xfc000000, 0x180000d1) // PCI mem2
	set_cpu_window(2, 0x1c000000, 0xffe00000, 0x1c0000d1) // PCI cfg/IO/header
	set_cpu_window(3, 0x1c200000, 0xffe00000, 0x1c2000d2) // gpu 1c2 /dc 1c3
	set_cpu_window(4, 0x1f000000, 0xff000000, 0x1f0000d3) // AXIMUX
//	set_cpu_window(5, 0x40000000, 0xc0000000, 0x000000f0) // DDR 1GB
	set_cpu_window(5, 0x00000000, 0x00000000, 0x000000f0) // everything else
	set_cpu_window(6, 0x00000000, 0x00000000, 0x000000f0) // everything else
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // everything else
#elif defined(CONFIG_CPU_LOONGSON1B)
	set_cpu_window(0, 0x1c300000, 0xfff00000, 0x1c3000d2) // dc       1M must cachable
	set_cpu_window(1, 0x1fe10000, 0xffffe000, 0x1fe100d3) // gmac0	8K
	set_cpu_window(2, 0x1fe20000, 0xffffe000, 0x1fe200d3) // gmac1	8K
	set_cpu_window(3, 0x1fe10000, 0xffff0000, 0x1fe100d0) // gmac0	64K
	set_cpu_window(4, 0x1fe20000, 0xffff0000, 0x1fe200d0) // gmac1	64K
	set_cpu_window(5, 0x1ff00000, 0xfff00000, 0x1ff000d0) // reserved 1M
	set_cpu_window(6, 0x1f000000, 0xff000000, 0x1f0000d3) // AXIMUX   16M
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	li	t0, 0xbfd000e0
	lw	t1, 0x0(t0)	//0xbfd000e0
	and t1, t1, 0xffffff00
	ori	t1, 0xd0
	sw	t1, 0x0(t0)
	lw	t1, 0x8(t0)	//0xbfd000e8
	and t1, t1, 0xffffff00
	ori	t1, 0xd0
	sw	t1, 0x8(t0)
#elif defined(CONFIG_CPU_LOONGSON1C)
	/*	set_cpu_window(0, 0x1c280000, 0xfff80000, 0x1c280083) // camera 512K
	set_cpu_window(1, 0x1c300000, 0xfff00000, 0x1c300081) // dc 1M
	set_cpu_window(2, 0x1fe10000, 0xffffe000, 0x1fe10082) // gmac0	8K
	set_cpu_window(3, 0x1fe10000, 0xffff0000, 0x1fe100d0) // gmac0	64K
	set_cpu_window(4, 0x1f000000, 0xff000000, 0x1f000082) // AXIMUX   16M
	set_cpu_window(5, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	set_cpu_window(6, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // ddr 0*/

/*	set_cpu_window(0, 0x1c280000, 0xfff80000, 0x1c2800d3) // camera
//	set_cpu_window(1, 0x1fc00000, 0xfff00000, 0x1fc000f2) //
	set_cpu_window(2, 0x1c300000, 0xfff00000, 0x1c3000d1) // dc 1M
//	set_cpu_window(3, 0x1f000000, 0xff000000, 0x1f0000d2) //
	set_cpu_window(4, 0x00000000, 0x00000000, 0x000000f0)
	set_cpu_window(5, 0x00000000, 0x00000000, 0x000000f0)
	set_cpu_window(6, 0x00000000, 0x00000000, 0x000000f0) // ddr 0
	set_cpu_window(7, 0x00000000, 0x00000000, 0x000000f0) // ddr 0*/
#endif
	// after this fixup, the kernel code should be compiled with
	// uncached instruction fetch patch

	/* 配置内存 */
#if defined(CONFIG_CPU_LOONGSON1A) || defined(CONFIG_CPU_LOONGSON1B)
	/*
	 * set *_ssel and *_tsel
	 * *_ssel参数用于配置DDR IO的驱动强度 01: 弱驱动 11: 强驱动
	 * *_tsel参数用于配置DDR IO的ODT输入匹配阻抗 00: disable 01: 75ohm 10: 150ohm 11: 50ohm
	 * pad_st不用于SSTL18模式，应保持为0
	 */
	li	t0, 0xbfd010c8
	li	t1, 0xfc000000
#ifdef CONFIG_CPU_LOONGSON1A
	li	t1, 0x00000000
#endif
	sw	t1, (t0)
	li	t0, 0xbfd010f8
	li	t1, 0x14000000
	sw	t1, (t0)

	/* DDR2 config begin */
	bal	ddr2_config
	nop

#ifdef CONFIG_DDR16BIT
	/*16bit ddr and disable conf*/
	#if defined(CONFIG_CPU_LOONGSON1A)
		li a1, 0x3
	#elif defined(CONFIG_CPU_LOONGSON1B)
		li a1, 0x110000
	#endif
#else
	/*disable conf*/
	#if defined(CONFIG_CPU_LOONGSON1A)
		li a1, 0x2
	#elif defined(CONFIG_CPU_LOONGSON1B)
		li a1, 0x100000
	#endif
#endif //#ifdef CONFIG_DDR16BIT

#if defined(CONFIG_CPU_LOONGSON1A)
	li a0, 0xbfd00420
#elif defined(CONFIG_CPU_LOONGSON1B)
	li a0, 0xbfd00424
#endif
	lw a2, 0x0(a0)
	or a2, a1
	sw a2, 0x0(a0)

#elif defined(CONFIG_CPU_LOONGSON1C)
#if !defined(CONFIG_NAND_BOOT_EN)
#include "sdram_cfg.S"

	li  	t1, 0xbfd00410
	li		a1, SD_PARA0
	sw		a1, 0x0(t1)
	li		a1, SD_PARA1
	sw		a1, 0x4(t1)
	li		a1, SD_PARA0
	sw		a1, 0x0(t1)
	li		a1, SD_PARA1
	sw		a1, 0x4(t1)
	li		a1, SD_PARA0
	sw		a1, 0x0(t1)
	li		a1, SD_PARA1_EN
	sw		a1, 0x4(t1)
#endif
#endif

	PRINTSTR("OK...\r\n");

	move	ra, s0
	jr		ra
	nop

	/* serial port configuration */
LEAF(initserial)
	.set	noat
	move	AT, ra

	la		v0, UART_BASE_ADDR
#ifdef	HAVE_MUT_COM
	bal		1f
	nop

	li		a0, 0
	la		v0, COM3_BASE_ADDR
	bal		1f
	nop

	jr		AT
	nop
#endif
1:
	li		v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb		v1, NSREG(NS16550_FIFO)(v0)
	li		v1, CFCR_DLAB
	sb		v1, NSREG(NS16550_CFCR)(v0)
#if defined(CONFIG_CPU_LOONGSON1A)
	li		a0, COREPLL_CFG
//	move	a0, a1
	and	a0, 0x700
	srl	a0, 8
	addiu	a0, 3
	li		v1, OSC_CLK
	multu	a0, v1
	mflo	v1
	li		a0, 2*16*CONFIG_BAUDRATE
	divu	v1, a0
	mflo	v1
#elif defined(CONFIG_CPU_LOONGSON1B)
	move	v1, a0
	bnez	v1, 2f
	nop
	li		v1, OSC_CLK
	li		a0, 0xbfe78030
	lw		a1, 4(a0)
	li		a2, 0xc00
	and		a1, a2
	beq		a1, a2,2f
	nop
	lw		a1, (a0)
	andi	a2, a1, 0x3f
	addiu	a2, 12
	sll		a2, 10
	srl		a1, 8
	andi	a1, 0x3ff
	addu	a1, a2
	li		a2, (OSC_CLK>>11)
	multu	a1, a2
	mflo	v1
	lw		a1, 4(a0)
	srl		a1, 14
	andi	a2, a1, 0x20
	beqz	a2, 1f
	nop
	andi	a1, 0x1f
	divu	v1, a1
	mflo	v1
	b 2f
	nop
1:
	srl		v1, 1
2:
	li		a1, 2*16*CONFIG_BAUDRATE
	divu	v1, v1, a1
#elif defined(CONFIG_CPU_LOONGSON1C)
	/* uart3 config mux 默认第一复用 */
#if (UART_BASE_ADDR == 0xbfe4c000)
#if defined(CONFIG_LS1C_UART_USE_EJTAG)
	li		a0, 0xbfd011f0
	lw		a1, 0x00(a0)
	ori		a1, 0x03
	sw		a1, 0x00(a0)
#else
	li		a0, 0xbfd011c4
//	lw		a1, 0x00(a0)
//	and		a1, 0xfffffff9
//	sw		a1, 0x00(a0)
	lw		a1, 0x10(a0)
	ori		a1, 0x06
	sw		a1, 0x10(a0)
//	lw		a1, 0x20(a0)
//	and		a1, 0xfffffff9
//	sw		a1, 0x20(a0)
//	lw		a1, 0x30(a0)
//	and		a1, 0xfffffff9
//	sw		a1, 0x30(a0)
#endif
#elif (UART_BASE_ADDR == 0xbfe48000)
	/* UART2 */
	li		a0, 0xbfd011c4
	lw		a1, 0x10(a0)
	ori		a1, 0x30
	sw		a1, 0x10(a0)
#elif (UART_BASE_ADDR == 0xbfe44000)
	/* UART1 */
	li		a0, 0xbfd011f0
	lw		a1, 0x00(a0)
	ori		a1, 0x0c
	sw		a1, 0x00(a0)
#endif

	li		a0, 0xbfe78030
	lw		a1, 0(a0)
	andi	a2, a1, 0x3
	addiu	a2, 1			//sdram_div+1
	li		t1, 1
	sll		t1, a2			//1<<(sdram_div+1)
	li		t2, 5
	remu	t1, t2			//(1<<(sdram_div+1))%5
	srl		a1, 8
	andi	a1, 0xff
	li		a2, OSC_CLK
	srl		a2, 2			//OSC_CLK/4
	multu	a1, a2
	mflo	v1				//OSC_CLK/4 * PLL_MULT
	divu	v1, t1
	mflo	v1				//OSC_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5

	lw		a1, 4(a0)
	andi	a2, a1, DIV_CPU_SEL
	bnez	a2, 1f
	nop
	li		v1, OSC_CLK
	b		3f
	nop
1:
	andi	a2, a1, DIV_CPU_EN
	bnez	a2, 2f
	nop
	srl		v1, 1			//OSC_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5 / 2
	b		3f
	nop
2:
	andi	a1, DIV_CPU
	srl		a1, DIV_CPU_SHIFT
	divu	v1, a1
	mflo	v1				//OSC_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5 / CPU_DIV
3:
//	li		v1, ((OSC_CLK / 4) * (PLL_MULT / CPU_DIV)) / SDRAM_PARAM_DIV_NUM / (16*CONFIG_BAUDRATE)
	li		a1, 16*CONFIG_BAUDRATE
	divu	v1, v1, a1
#endif
	sb		v1, NSREG(NS16550_DATA)(v0)
	srl		v1, 8
	sb		v1, NSREG(NS16550_IER)(v0)
	li		v1, CFCR_8BITS
	sb		v1, NSREG(NS16550_CFCR)(v0)
	li		v1, MCR_DTR|MCR_RTS
	sb		v1, NSREG(NS16550_MCR)(v0)
	li		v1, 0x0
	sb		v1, NSREG(NS16550_IER)(v0)

	#disable all interrupt
	li  	v1, 0x0
	sb  	v1, NSREG(NS16550_IER)(v0)
	j   	ra
	nop

	.set 	at
END(initserial)

stringserial:
	.set	noreorder
	move	a2, ra
	move	a1, a0
	lbu		a0, 0(a1)
1:
	beqz	a0, 2f
	nop
	bal		tgt_putchar
	addiu	a1, 1
	b		1b
	lbu		a0, 0(a1)

2:
	j	a2
	nop
	.set reorder


tgt_putchar:
	la		v0, UART_BASE_ADDR
1:
	lbu		v1, NSREG(NS16550_LSR)(v0)
	and		v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb		a0, NSREG(NS16550_DATA)(v0)
	move	v1, v0
	la		v0, UART_BASE_ADDR
	bne		v0, v1, 1b
	nop
	j		ra
	nop


