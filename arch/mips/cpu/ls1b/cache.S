/*
 *  Cache-handling routined for MIPS CPUs
 *
 *  Copyright (c) 2013 Tang Haifeng <tanghaifeng-gz@loongson.cn>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/cacheops.h>

#ifndef CONFIG_SYS_MIPS_CACHE_MODE
#define CONFIG_SYS_MIPS_CACHE_MODE CONF_CM_CACHABLE_NONCOHERENT
#endif

#define RA		t9

/*
 * 16kB is the maximum size of instruction and data caches on MIPS 4K,
 * 64kB is on 4KE, 24K, 5K, etc. Set bigger size for convenience.
 *
 * Note that the above size is the maximum size of primary cache. U-Boot
 * doesn't have L2 cache support for now.
 */
#define MIPS_MAX_CACHE_SIZE	0x10000

#define INDEX_BASE	CKSEG0

	.macro	cache_op op addr
	.set	push
	.set	noreorder
	.set	mips3
	cache	\op, 0(\addr)
	.set	pop
	.endm

	.macro	f_fill64 dst, offset, val
	LONG_S	\val, (\offset +  0 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  1 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  2 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  3 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  4 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  5 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  6 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  7 * LONGSIZE)(\dst)
#if LONGSIZE == 4
	LONG_S	\val, (\offset +  8 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset +  9 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 10 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 11 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 12 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 13 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 14 * LONGSIZE)(\dst)
	LONG_S	\val, (\offset + 15 * LONGSIZE)(\dst)
#endif
	.endm

/*
 * mips_init_icache(uint PRId, ulong icache_size, unchar icache_linesz)
 */
LEAF(mips_init_icache)
	blez		a1, 9f
	mtc0		zero, CP0_TAGLO
	/* clear tag to invalidate */
	PTR_LI		t0, INDEX_BASE
	PTR_ADDU	t1, t0, a1
1:	cache_op	INDEX_STORE_TAG_I t0
	PTR_ADDU	t0, a2
	bne		t0, t1, 1b
	/* fill once, so data field parity is correct */
	PTR_LI		t0, INDEX_BASE
2:	cache_op	FILL t0
	PTR_ADDU	t0, a2
	bne		t0, t1, 2b
	/* invalidate again - prudent but not strictly neccessary */
	PTR_LI		t0, INDEX_BASE
1:	cache_op	INDEX_STORE_TAG_I t0
	PTR_ADDU	t0, a2
	bne		t0, t1, 1b
9:	jr		ra
	END(mips_init_icache)

/*
 * mips_init_dcache(uint PRId, ulong dcache_size, unchar dcache_linesz)
 */
LEAF(mips_init_dcache)
	blez		a1, 9f
	mtc0		zero, CP0_TAGLO
	/* clear all tags */
	PTR_LI		t0, INDEX_BASE
	PTR_ADDU	t1, t0, a1
1:	cache_op	INDEX_STORE_TAG_D t0
	PTR_ADDU	t0, a2
	bne		t0, t1, 1b
	/* load from each line (in cached space) */
	PTR_LI		t0, INDEX_BASE
2:	LONG_L		zero, 0(t0)
	PTR_ADDU	t0, a2
	bne		t0, t1, 2b
	/* clear all tags */
	PTR_LI		t0, INDEX_BASE
1:	cache_op	INDEX_STORE_TAG_D t0
	PTR_ADDU	t0, a2
	bne		t0, t1, 1b
9:	jr		ra
	END(mips_init_dcache)

/*
 * mips_cache_reset - low level initialisation of the primary caches
 *
 * This routine initialises the primary caches to ensure that they have good
 * parity.  It must be called by the ROM before any cached locations are used
 * to prevent the possibility of data with bad parity being written to memory.
 *
 * To initialise the instruction cache it is essential that a source of data
 * with good parity is available. This routine will initialise an area of
 * memory starting at location zero to be used as a source of parity.
 *
 * RETURNS: N/A
 *
 */
//NESTED(mips_cache_reset, 0, ra)
LEAF(mips_cache_reset)
	.set noreorder
	move t1, ra
####part 2####
cache_detect_4way:
	.set mips32
	mfc0    t4, CP0_CONFIG, 1
	lui     v0, 0x7
	and     v0, t4, v0
	srl     t3, v0, 16    #ic

	li      t5, 0x800 #32*64
	srl     v1, t4, 22
	andi    v1, 7
	sll     t5, v1   #InstCacheSetSize
	sll     t5, t3   #t6 InstCacheSize


	andi    v0, t4, 0x0380
	srl     t7, v0, 7     #dc

	li      t6, 0x800 #32*64
	srl     v1, t4, 13
	andi    v1, 7
	sll     t6, v1  #DataCacheSetSize
	sll     t6, t7  #t5 DataCacheSize
####part 3####
	lui     a0, 0x8000
	addu    a1, $0, t5
	addu    a2, $0, t6
cache_init_d2way:
	mtc0    $0, CP0_TAGHI
	addu    v0, $0, a0
	addu    v1, a0, a2
1:	slt     a3, v0, v1
	beq     a3, $0, 1f
	nop
	mtc0    $0, CP0_TAGLO
	cache   INDEX_STORE_TAG_D, 0x0(v0)      # 1 way
	beq     $0, $0, 1b
	addiu   v0, v0, 0x20
1:
cache_flush_i2way:
	addu    v0, $0, a0
	addu    v1, a0, a1
1:	slt     a3, v0, v1
	beq     a3, $0, 1f
	nop
	cache   INDEX_INVALIDATE_I, 0x0(v0)     # 1 way
	beq     $0, $0, 1b
	addiu   v0, v0, 0x20
1:
cache_flush_d2way:
	addu    v0, $0, a0
	addu    v1, a0, a2
1:	slt     a3, v0, v1
	beq     a3, $0, 1f
	nop
	cache   INDEX_WRITEBACK_INV_D, 0x0(v0)  #1 way
	beq     $0, $0, 1b
	addiu   v0, v0, 0x20
	.set    mips0

1:
cache_init_finish:
	jr  t1
	nop
	.set reorder
	END(mips_cache_reset)

/*
 * dcache_status - get cache status
 *
 * RETURNS: 0 - cache disabled; 1 - cache enabled
 *
 */
LEAF(dcache_status)
	mfc0	t0, CP0_CONFIG
	li	t1, CONF_CM_UNCACHED
	andi	t0, t0, CONF_CM_CMASK
	move	v0, zero
	beq	t0, t1, 2f
	li	v0, 1
2:	jr	ra
	END(dcache_status)

/*
 * dcache_disable - disable cache
 *
 * RETURNS: N/A
 *
 */
LEAF(dcache_disable)
	mfc0	t0, CP0_CONFIG
	li	t1, -8
	and	t0, t0, t1
	ori	t0, t0, CONF_CM_UNCACHED
	mtc0	t0, CP0_CONFIG
	jr	ra
	END(dcache_disable)

/*
 * dcache_enable - enable cache
 *
 * RETURNS: N/A
 *
 */
LEAF(dcache_enable)
	mfc0	t0, CP0_CONFIG
	ori	t0, CONF_CM_CMASK
	xori	t0, CONF_CM_CMASK
	ori	t0, CONFIG_SYS_MIPS_CACHE_MODE
	mtc0	t0, CP0_CONFIG
	jr	ra
	END(dcache_enable)
