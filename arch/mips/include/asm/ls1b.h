#ifndef __LS1B_H
#define __LS1B_H

#include <asm/addrspace.h>

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
#define UNCACHED_TO_CACHED(x)   (PHYS_TO_CACHED(UNCACHED_TO_PHYS(x)))

/* PLL Controller */
#define LS1X_CLK_PLL_FREQ	0xbfe78030
#define LS1X_CLK_PLL_DIV	0xbfe78034

#endif
