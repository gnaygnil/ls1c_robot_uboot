/**\file
 *  This file serves as the wrapper for the platform/OS dependent functions
 *  It is needed to modify these functions accordingly based on the platform and the
 *  OS. Whenever the synopsys GMAC driver ported on to different platform, this file
 *  should be handled at most care.
 *  The corresponding function definitions for non-inline functions are available in 
 *  synopGMAC_plat.c file.
 * \internal
 * -------------------------------------REVISION HISTORY---------------------------
 * Synopsys 				01/Aug/2007		 	   Created
 */


#ifndef SYNOP_GMAC_PLAT_H
#define SYNOP_GMAC_PLAT_H 1

#include <common.h>
#include <net.h>
#include <linux/stddef.h>
#include <linux/err.h>

#include <malloc.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/ls1x.h>

#ifdef GMAC_DEBUG
#define TR0(fmt, args...)	printf(fmt, ##args) 
#define TR(fmt, args...)	printf(fmt, ##args)
#else
#define TR0(fmt, args...)
#define TR(fmt, args...)
#endif

#define DEFAULT_DELAY_VARIABLE  10
#define DEFAULT_LOOP_VARIABLE   10000

/* Error Codes */
#define ESYNOPGMACNOERR   0
#define ESYNOPGMACNOMEM   1
#define ESYNOPGMACPHYERR  2
#define ESYNOPGMACBUSY    3

/**
  * These are the wrapper function prototypes for OS/platform related routines
  */
void *plat_alloc_memory(u32);
void plat_free_memory(void *);
dma_addr_t plat_dma_map_single(unsigned long addr, size_t size);
void *plat_alloc_consistent_dmaable_memory(u32 size, u32 *addr);
void plat_delay(u32);

u32 synopGMACReadReg(u32 RegBase, u32 RegOffset);
void synopGMACWriteReg(u32 RegBase, u32 RegOffset, u32 RegData );
void synopGMACSetBits(u32 RegBase, u32 RegOffset, u32 BitPos);
void synopGMACClearBits(u32 RegBase, u32 RegOffset, u32 BitPos);
int synopGMACCheckBits(u32 RegBase, u32 RegOffset, u32 BitPos);

#endif
