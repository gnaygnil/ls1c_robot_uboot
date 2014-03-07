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

#include "ns16550.h"

/* Delay macro */
#define DELAY(count)	\
	li v0, count;		\
99:                     \
	bnez    v0, 99b;	\
	addiu   v0, -1

#ifdef CONFIG_CPU_UART
#define PRINTSTR(x) \
	.rdata;98: .asciz x; .text; la a0, 98b; bal stringserial; nop
#else
#define PRINTSTR(x)
#endif

	.globl  lowlevel_init
lowlevel_init:
	move s0, ra

/*spi speed*/
	li  t0, 0xbfe80000
	li  t1, 0x17  // div 4, fast_read + burst_en + memory_en
	sb  t1, 0x4(t0) // sfc_param

/*ddr2 configuration,stolen from pmon by zw*/
	li  a0, 0xbfe78030
    /*31:dc_div_en,30-26:dc_div,25:cpu_div_en,24-20:cpu_div,19:ddr_div_en,18-14:ddr_div*/
#if 0
    li v1,0x92298000
    li  a1, 0x39f0a
#else
	li v0,0xbfc701f0
	lw v1,4(v0)
	lw a1,(v0)

	li v0,0xfffc0000
	and v0,a1
	bnez v0,1f
	nop

	andi v0,v1,0x3f
	bnez v0,1f
	nop

	li v0,(1<<31)|(1<<25)|(1<<19)
	and a2,v0,v1
	bne a2,v0,1f
	nop

	beqz v1,1f
	nop
	nop
	b 2f
	nop
1:
    li v1, 0x8a290000//(1<<31)|(4<<26)|(1<<25)|(1<<20)|(1<<19)|(2<<14)|0x2a00
    li  a1, 0x10313

2:
#endif
	or v1, 0x2a00
	sw v1, 4(a0)
	sw a1, (a0);

1:
   lw v1,(a0)
   bne a1,v1,1b
   nop


	/*disable all gpio*/
	li	a0, 0xbfd00000
	sw	zero, 0x10c0(a0)
	sw	zero, 0x10c4(a0)
	sw	zero, 0x10c8(a0)

#ifdef BUZZER
	/* disable buzzer gpio40*/
	li	v0, 0xbfd010c4
	li	t0, 0x00000100
	sw	t0, 0(v0)
	li	v0, 0xbfd010d4
	li	t0, 0
	sw	t0, 0(v0)
#endif

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
#if defined(LS1ASOC)
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
#elif defined(LS1BSOC)
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
#endif
	// after this fixup, the kernel code should be compiled with
	// uncached instruction fetch patch

	/* 配置内存 */
#if defined(LS1ASOC) || defined(LS1BSOC)
	/*
	 * set *_ssel and *_tsel
	 * *_ssel参数用于配置DDR IO的驱动强度 01: 弱驱动 11: 强驱动
	 * *_tsel参数用于配置DDR IO的ODT输入匹配阻抗 00: disable 01: 75ohm 10: 150ohm 11: 50ohm
	 * pad_st不用于SSTL18模式，应保持为0
	 */
	li	t0, 0xbfd010c8
	li	t1, 0xfc000000
#ifdef LS1ASOC
	li	t1, 0x00000000
#endif
	sw	t1, (t0)
	li	t0, 0xbfd010f8
	li	t1, 0x14000000
	sw	t1, (t0)

	PRINTSTR("DDR2 config begin\r\n")
	bal     ddr2_config
	nop
	PRINTSTR("DDR2 config end\r\n")

	/* memory size defined in conf */
//	li msize, CONFIG_MEM_SIZE

#ifdef CONFIG_DDR16BIT
	/*16bit ddr and disable conf*/ 
	#ifdef LS1ASOC
		li a1, 0x3
	#elif LS1BSOC
		li a1, 0x110000
	#endif
#else
	/*disable conf*/
	#ifdef LS1ASOC
		li a1, 0x2
	#elif LS1BSOC
		li a1, 0x100000
	#endif
#endif //#ifdef CONFIG_DDR16BIT

#ifdef LS1ASOC
	li a0, 0xbfd00420
#elif LS1BSOC
	li a0, 0xbfd00424
#endif
	lw a2, 0x0(a0)
	or a2, a1
	sw a2, 0x0(a0)

#elif defined(LS1CSOC)
	li msize, MEM_SIZE
#include "sdram_cfg.S"

	li  t1, 0xbfd00410
	li  a1, SD_PARA0
	sw  a1, 0x0(t1)
	li  a1, SD_PARA1
	sw  a1, 0x4(t1)

    PRINTSTR("sdram init done \r\n")
#endif

	move	ra, s0
	jr		ra
	nop

/* serial port configuration */
LEAF(initserial)
	.set noat
	move AT,ra
	
	la	v0, CONFIG_SYS_NS16550_COM1
#ifdef	HAVE_MUT_COM
	bal	1f
	nop

	li	a0, 0
	la	v0, COM3_BASE_ADDR
	bal	1f
	nop

	jr	AT
	nop
#endif
1:
	li	v1, FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	v1, NSREG(NS16550_FIFO)(v0)
	li	v1, CFCR_DLAB
	sb	v1, NSREG(NS16550_CFCR)(v0)
#if defined(LS1ASOC)
//	li	v1, ((APB_CLK*DDR_MULT)/(2*16*CONFIG_BAUDRATE))     // 8MHz crystal,   M[7:3]=6     1fboard
//	li	a0, 0xbfe78030
//	lw	a0, (a0)
	move	a0, a1
	and		a0, 0x700
	srl		a0, 8
	addiu	a0, 3
	li		v1, APB_CLK
	multu	a0, v1
	mflo	v1
	li		a0, 2*16*CONFIG_BAUDRATE
	divu	v1, a0
	mflo	v1
#elif defined(LS1BSOC)
	move	v1, a0
	bnez	v1, 2f
	nop
	li		v1, APB_CLK
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
	li		a2, (APB_CLK>>11)
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
#elif defined(LS1CSOC)
	/* #uart3 config mux */
	li	t1, 0xbfd011c4
	li	t2, 0x0
	sw	t2, 0x0(t1)

	li	t2, 0x6
	sw	t2, 0x10(t1)

	li	t2, 0x0
	sw	t2, 0x20(t1)

	li	t2, 0x0
	sw	t2, 0x30(t1)

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
	li		a2, APB_CLK
	srl		a2, 2			//APB_CLK/4
	multu	a1, a2
	mflo	v1				//APB_CLK/4 * PLL_MULT
	divu	v1, t1
	mflo	v1				//APB_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5

	lw		a1, 4(a0)
	andi	a2, a1, DIV_CPU_SEL
	bnez	a2, 1f
	nop
	li		v1, APB_CLK
	b		3f
	nop
1:
	andi	a2, a1, DIV_CPU_EN
	bnez	a2, 2f
	nop
	srl		v1, 1			//APB_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5 / 2
	b		3f
	nop
2:
	andi	a1, DIV_CPU
	srl		a1, DIV_CPU_SHIFT
	divu	v1, a1
	mflo	v1				//APB_CLK/4 * PLL_MULT / (1<<(sdram_div+1))%5 / CPU_DIV
3:
//	li	v1, ((APB_CLK / 4) * (PLL_MULT / CPU_DIV)) / SDRAM_PARAM_DIV_NUM / (16*CONFIG_BAUDRATE)
	li		a1, 16*CONFIG_BAUDRATE
	divu	v1, v1, a1
#endif
	sb	v1, NSREG(NS16550_DATA)(v0)
	srl	v1, 8
	sb	v1, NSREG(NS16550_IER)(v0)
	li	v1, CFCR_8BITS
	sb	v1, NSREG(NS16550_CFCR)(v0)
	li	v1, MCR_DTR|MCR_RTS
	sb	v1, NSREG(NS16550_MCR)(v0)
	li	v1, 0x0
	sb	v1, NSREG(NS16550_IER)(v0)
        
	#disable all interrupt
	li  v1, 0x0
	sb  v1, NSREG(NS16550_IER)(v0)
	j   ra
	nop
	.set at
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
	la		v0, CONFIG_SYS_NS16550_COM1
1:
	lbu		v1, NSREG(NS16550_LSR)(v0)
	and		v1, LSR_TXRDY
	beqz	v1, 1b
	nop

	sb		a0, NSREG(NS16550_DATA)(v0)
	move	v1, v0
	la		v0, CONFIG_SYS_NS16550_COM1
	bne		v0, v1, 1b
	nop
	j		ra
	nop


