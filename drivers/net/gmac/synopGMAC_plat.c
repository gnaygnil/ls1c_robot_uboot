/**\file
 *  This file defines the wrapper for the platform/OS related functions
 *  The function definitions needs to be modified according to the platform 
 *  and the Operating system used.
 *  This file should be handled with greatest care while porting the driver
 *  to a different platform running different operating system other than
 *  Linux 2.6.xx.
 * \internal
 * ----------------------------REVISION HISTORY-----------------------------
 * Synopsys			01/Aug/2007			Created
 */
 
#include "synopGMAC_plat.h"
#include "synopGMAC_Dev.h"

/**
  * This is a wrapper function for Memory allocation routine. In linux Kernel 
  * it it kmalloc function
  * @param[in] bytes in bytes to allocate
  */

void *plat_alloc_memory(u32 bytes) 
{
	void *buf = (void*)memalign(ARCH_DMA_MINALIGN, bytes);

	flush_cache((unsigned int)buf, bytes);
	return buf;
}

/**
  * This is a wrapper function for Memory free routine. In linux Kernel 
  * it it kfree function
  * @param[in] buffer pointer to be freed
  */
void plat_free_memory(void *buffer)
{
	free(buffer);
}

dma_addr_t plat_dma_map_single(unsigned long addr, size_t size)
{
	flush_cache(addr, size);

	return virt_to_phys((void *)addr);
}

/**
  * This is a wrapper function for consistent dma-able Memory allocation routine. 
  * In linux Kernel, it depends on pci dev structure
  * @param[in] bytes in bytes to allocate
  */
void *plat_alloc_consistent_dmaable_memory(u32 size, u32 *addr)
{
	void *buf;

	buf = (void *)memalign(16, size+16);
	flush_cache((unsigned int)buf, size+16);
	*addr = virt_to_phys(buf);
	buf = (void *)(((unsigned int)buf & 0x0fffffff) | 0xa0000000);
	return buf;
}

/**
  * This is a wrapper function for freeing consistent dma-able Memory.
  * In linux Kernel, it depends on pci dev structure
  * @param[in] bytes in bytes to allocate
  */
void plat_free_consistent_dmaable_memory(u32 size, void * addr, u32 dma_addr) 
{
	free(PHYS_TO_CACHED(UNCACHED_TO_PHYS(addr)));
}

/**
  * This is a wrapper function for platform dependent delay 
  * Take care while passing the argument to this function 
  * @param[in] buffer pointer to be freed
  */
void plat_delay(u32 delay)
{
	while (delay--);
	return;
}


/**
 * The Low level function to read register contents from Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * \return  Returns the register contents 
 */
u32 synopGMACReadReg(u32 RegBase, u32 RegOffset)
{
	return __raw_readl(RegBase + RegOffset);
}

/**
 * The Low level function to write to a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Data to be written 
 * \return  void 
 */
void synopGMACWriteReg(u32 RegBase, u32 RegOffset, u32 RegData)
{
	__raw_writel(RegData, RegBase + RegOffset);
}

/**
 * The Low level function to set bits of a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1 
 * \return  void 
 */
void synopGMACSetBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 data;
	data = synopGMACReadReg(RegBase, RegOffset);
	data |= BitPos; 
	synopGMACWriteReg(RegBase, RegOffset, data);
}


/**
 * The Low level function to clear bits of a register in Hardware.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to clear bits to logical 0 
 * \return  void 
 */
void  synopGMACClearBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 data;
	data = synopGMACReadReg(RegBase, RegOffset);
	data &= (~BitPos); 
	synopGMACWriteReg(RegBase, RegOffset, data);
}

/**
 * The Low level function to Check the setting of the bits.
 * 
 * @param[in] pointer to the base of register map  
 * @param[in] Offset from the base
 * @param[in] Bit mask to set bits to logical 1 
 * \return  returns TRUE if set to '1' returns FALSE if set to '0'. Result undefined there are no bit set in the BitPos argument.
 * 
 */
int synopGMACCheckBits(u32 RegBase, u32 RegOffset, u32 BitPos)
{
	u32 data;
	data = synopGMACReadReg(RegBase, RegOffset);
	data &= BitPos; 
	if(data)
		return 1;
	else
		return 0;
}
