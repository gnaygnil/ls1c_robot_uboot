/* ===================================================================================
 * Copyright (c) <2009> Synopsys, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software annotated with this license and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * =================================================================================== */

/** \file
 * This file defines the synopsys GMAC device dependent functions.
 * Most of the operations on the GMAC device are available in this file.
 * Functions for initiliasing and accessing MAC/DMA/PHY registers and the DMA descriptors
 * are encapsulated in this file. The functions are platform/host/OS independent.
 * These functions in turn use the low level device dependent (HAL) functions to
 * access the register space.
 * \internal
 * ------------------------REVISION HISTORY---------------------------------
 * Synopsys                 01/Aug/2007                              Created
 */
#include "synopGMAC_Dev.h"

/**
 * Function to reset the GMAC core.
 * This reests the DMA and GMAC core. After reset all the registers holds their respective reset value
 * @param[in] pointer to synopGMACdevice.
 * \return 0 on success else return the error status.
 */
s32 synopGMAC_reset(synopGMACdevice *gmacdev)
{
	u32 data = 0;
	int cnt = 0;

	synopGMACWriteReg(gmacdev->DmaBase, DmaBusMode ,DmaResetOn);
	plat_delay(DEFAULT_LOOP_VARIABLE);
	while (1) {
		data = synopGMACReadReg(gmacdev->DmaBase, DmaBusMode);
		TR("DATA after Reset = %08x\n",data);
		if (data & DmaResetOn) {
			if(cnt > 20) {
				printf("  error: synopGMAC_reset DmaBusMode: 0x%08x\n", data);
				return -1;
			}
			udelay(1);
			cnt ++;
		} else
			break;
	}

	return 0;
}

/**
 * Disable all the interrupts.
 * Disables all DMA interrupts.
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 * \note This function disabled all the interrupts, if you want to disable a particular interrupt then
 *  use synopGMAC_disable_interrupt().
 */
void synopGMAC_disable_interrupt_all(synopGMACdevice *gmacdev)
{
	synopGMACWriteReg(gmacdev->DmaBase, DmaInterrupt, 0);
	return;
}

/**
 * Initialize the rx descriptors for ring or chain mode operation.
 * 	- Status field is initialized to 0.
 *	- EndOfRing set for the last descriptor.
 *	- buffer1 and buffer2 set to 0 for ring mode of operation. (note)
 *	- data1 and data2 set to 0. (note)
 * @param[in] pointer to DmaDesc structure.
 * @param[in] whether end of ring
 * \return void.
 * \note Initialization of the buffer1, buffer2, data1,data2 and status are not done here. This only initializes whether one wants to use this descriptor
 * in chain mode or ring mode. For chain mode of operation the buffer2 and data2 are programmed before calling this function.
 */
void synopGMAC_rx_desc_init_ring(DmaDesc *desc, int last_ring_desc)
{
	desc->status = 0;
	desc->length = last_ring_desc ? RxDescEndOfRing : 0;
	desc->buffer1 = 0;
	desc->buffer2 = 0;
	desc->data1 = 0;
	desc->data2 = 0;
	desc->dummy1 = 0;
	desc->dummy2 = 0;
	return;
}
/**
 * Initialize the tx descriptors for ring or chain mode operation.
 * 	- Status field is initialized to 0.
 *	- EndOfRing set for the last descriptor.
 *	- buffer1 and buffer2 set to 0 for ring mode of operation. (note)
 *	- data1 and data2 set to 0. (note)
 * @param[in] pointer to DmaDesc structure.
 * @param[in] whether end of ring
 * \return void.
 * \note Initialization of the buffer1, buffer2, data1,data2 and status are not done here. This only initializes whether one wants to use this descriptor
 * in chain mode or ring mode. For chain mode of operation the buffer2 and data2 are programmed before calling this function.
 */
void synopGMAC_tx_desc_init_ring(DmaDesc *desc, int last_ring_desc)
{
#ifdef ENH_DESC
	desc->status |= (last_ring_desc? TxDescEndOfRing : 0);
	desc->length = 0;
#else
	desc->length = last_ring_desc? TxDescEndOfRing : 0;
#endif
	desc->status = 0;	//thf
	desc->buffer1 = 0;
	desc->buffer2 = 0;
	desc->data1 = 0;
	desc->data2 = 0;
	desc->dummy1 = 0;
	desc->dummy2 = 0;
	return;
}

/**
 * Initialize the rx descriptors for chain mode of operation.
 * 	- Status field is initialized to 0.
 *	- EndOfRing set for the last descriptor.
 *	- buffer1 and buffer2 set to 0.
 *	- data1 and data2 set to 0.
 * @param[in] pointer to DmaDesc structure.
 * @param[in] whether end of ring
 * \return void.
 */

void synopGMAC_rx_desc_init_chain(DmaDesc * desc)
{
	desc->status = 0;
	desc->length = RxDescChain;
	desc->buffer1 = 0;
	desc->data1 = 0;
	return;
}
/**
 * Initialize the rx descriptors for chain mode of operation.
 * 	- Status field is initialized to 0.
 *	- EndOfRing set for the last descriptor.
 *	- buffer1 and buffer2 set to 0.
 *	- data1 and data2 set to 0.
 * @param[in] pointer to DmaDesc structure.
 * @param[in] whether end of ring
 * \return void.
 */
void synopGMAC_tx_desc_init_chain(DmaDesc * desc)
{
#ifdef ENH_DESC
	desc->status = TxDescChain;
	desc->length = 0;
#else
	desc->length = TxDescChain;
#endif
	desc->buffer1 = 0;
	desc->data1 = 0;
	return;
}


s32 synopGMAC_init_tx_rx_desc_queue(synopGMACdevice *gmacdev)
{
	s32 i;
	for(i =0; i < gmacdev -> TxDescCount; i++){
		synopGMAC_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount-1);
	}
	TR("At line %d\n",__LINE__);
	for(i =0; i < gmacdev -> RxDescCount; i++){
		synopGMAC_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount-1);
	}

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;

	return -ESYNOPGMACNOERR;
}

/**
 * Resumes the DMA Transmission.
 * the DmaTxPollDemand is written. (the data writeen could be anything).
 * This forces the DMA to resume transmission.
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 */
void synopGMAC_resume_dma_tx(synopGMACdevice * gmacdev)
{
	synopGMACWriteReg(gmacdev->DmaBase, DmaTxPollDemand, 1);
}

/**
 * Programs the DmaRxBaseAddress with the Rx descriptor base address.
 * Rx Descriptor's base address is available in the gmacdev structure. This function progrms the
 * Dma Rx Base address with the starting address of the descriptor ring or chain.
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 */
void synopGMAC_init_rx_desc_base(synopGMACdevice *gmacdev)
{
	synopGMACWriteReg(gmacdev->DmaBase, DmaRxBaseAddr, (u32)gmacdev->RxDescDma);
	return;
}

/**
 * Programs the DmaTxBaseAddress with the Tx descriptor base address.
 * Tx Descriptor's base address is available in the gmacdev structure. This function progrms the
 * Dma Tx Base address with the starting address of the descriptor ring or chain.
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 */
void synopGMAC_init_tx_desc_base(synopGMACdevice *gmacdev)
{
	synopGMACWriteReg(gmacdev->DmaBase, DmaTxBaseAddr, (u32)gmacdev->TxDescDma);
	return;
}

/**
 * Checks whether the descriptor is owned by DMA.
 * If descriptor is owned by DMA then the OWN bit is set to 1. This API is same for both ring and chain mode.
 * @param[in] pointer to DmaDesc structure.

 * \return returns true if Dma owns descriptor and false if not.
 */
int synopGMAC_is_desc_owned_by_dma(DmaDesc *desc)
{
	return ((desc->status & DescOwnByDma) == DescOwnByDma);
}

/**
 * returns the byte length of received frame including CRC.
 * This returns the no of bytes received in the received ethernet frame including CRC(FCS).
 * @param[in] pointer to DmaDesc structure.
 * \return returns the length of received frame lengths in bytes.
 */
u32 synopGMAC_get_rx_desc_frame_length(u32 status)
{
	return ((status & DescFrameLengthMask) >> DescFrameLengthShift);
}

/**
 * Checks whether the descriptor is valid
 * if no errors such as CRC/Receive Error/Watchdog Timeout/Late collision/Giant Frame/Overflow/Descriptor
 * error the descritpor is said to be a valid descriptor.
 * @param[in] pointer to DmaDesc structure.
 * \return True if desc valid. false if error.
 */
int synopGMAC_is_desc_valid(u32 status)
{
	return ((status & DescError) == 0);
}

/**
 * Checks whether the descriptor is empty.
 * If the buffer1 and buffer2 lengths are zero in ring mode descriptor is empty.
 * In chain mode buffer2 length is 0 but buffer2 itself contains the next descriptor address.
 * @param[in] pointer to DmaDesc structure.
 * \return returns true if descriptor is empty, false if not empty.
 */
int synopGMAC_is_desc_empty(DmaDesc *desc)
{
	//if both the buffer1 length and buffer2 length are zero desc is empty
	return (((desc->length  & DescSize1Mask) == 0) && ((desc->length  & DescSize2Mask) == 0));
}


/**
 * Checks whether the rx descriptor is valid.
 * if rx descripor is not in error and complete frame is available in the same descriptor
 * @param[in] pointer to DmaDesc structure.
 * \return returns true if no error and first and last desc bits are set, otherwise it returns false.
 */
int synopGMAC_is_rx_desc_valid(u32 status)
{
	return ((status & DescError) == 0) && ((status & DescRxFirst) == DescRxFirst) && ((status & DescRxLast) == DescRxLast);
}

/**
 * Checks whether this rx descriptor is last rx descriptor.
 * This returns true if it is last descriptor either in ring mode or in chain mode.
 * @param[in] pointer to devic structure.
 * @param[in] pointer to DmaDesc structure.
 * \return returns true if it is last descriptor, false if not.
 * \note This function should not be called before initializing the descriptor using synopGMAC_desc_init().
 */
int synopGMAC_is_last_rx_desc(synopGMACdevice * gmacdev,DmaDesc *desc)
{
	return (((desc->length & RxDescEndOfRing) == RxDescEndOfRing) || ((u32)gmacdev->RxDesc == desc->data2));
}

/**
 * Checks whether this tx descriptor is last tx descriptor.
 * This returns true if it is last descriptor either in ring mode or in chain mode.
 * @param[in] pointer to devic structure.
 * @param[in] pointer to DmaDesc structure.
 * \return returns true if it is last descriptor, false if not.
 * \note This function should not be called before initializing the descriptor using synopGMAC_desc_init().
 */
int synopGMAC_is_last_tx_desc(synopGMACdevice * gmacdev,DmaDesc *desc)
{
#ifdef ENH_DESC
	return (((desc->status & TxDescEndOfRing) == TxDescEndOfRing) || ((u32)gmacdev->TxDesc == desc->data2));
#else
	return (((desc->length & TxDescEndOfRing) == TxDescEndOfRing) || ((u32)gmacdev->TxDesc == desc->data2));
#endif
}

/**
 * Checks whether this rx descriptor is in chain mode.
 * This returns true if it is this descriptor is in chain mode.
 * @param[in] pointer to DmaDesc structure.
 * \return returns true if chain mode is set, false if not.
 */
int synopGMAC_is_rx_desc_chained(DmaDesc * desc)
{
	return((desc->length & RxDescChain) == RxDescChain);
}

/**
 * Checks whether this tx descriptor is in chain mode.
 * This returns true if it is this descriptor is in chain mode.
 * @param[in] pointer to DmaDesc structure.
 * \return returns true if chain mode is set, false if not.
 */
int synopGMAC_is_tx_desc_chained(DmaDesc * desc)
{
#ifdef ENH_DESC
	return((desc->status & TxDescChain) == TxDescChain);
#else
	return((desc->length & TxDescChain) == TxDescChain);
#endif
}

/**
 * Get the index and address of Tx desc.
 * This api is same for both ring mode and chain mode.
 * This function tracks the tx descriptor the DMA just closed after the transmission of data from this descriptor is
 * over. This returns the descriptor fields to the caller.
 * @param[in] pointer to synopGMACdevice.
 * @param[out] status field of the descriptor.
 * @param[out] Dma-able buffer1 pointer.
 * @param[out] length of buffer1 (Max is 2048).
 * @param[out] virtual pointer for buffer1.
 * @param[out] Dma-able buffer2 pointer.
 * @param[out] length of buffer2 (Max is 2048).
 * @param[out] virtual pointer for buffer2.
 * @param[out] u32 data indicating whether the descriptor is in ring mode or chain mode.
 * \return returns present tx descriptor index on success. Negative value if error.
 */
s32 synopGMAC_get_tx_qptr(synopGMACdevice * gmacdev, u32 * Status, u32 * Buffer1, u32 * Length1, u32 * Data1, u32 * Buffer2, u32 * Length2, u32 * Data2 )
{
	u32 txover      = gmacdev->TxBusy;
	DmaDesc *txdesc = gmacdev->TxBusyDesc;

	if(synopGMAC_is_desc_owned_by_dma(txdesc))
		return -1;
	if(synopGMAC_is_desc_empty(txdesc))
		return -1;

	(gmacdev->BusyTxDesc)--; //busy tx descriptor is reduced by one as it will be handed over to Processor now

	if(Status != 0)
		*Status = txdesc->status;

	if(Buffer1 != 0)
		*Buffer1 = txdesc->buffer1;
	if(Length1 != 0)
		*Length1 = (txdesc->length & DescSize1Mask) >> DescSize1Shift;
	if(Data1 != 0)
		*Data1 = txdesc->data1;

	if(Buffer2 != 0)
		*Buffer2 = txdesc->buffer2;
	if(Length2 != 0)
		*Length2 = (txdesc->length & DescSize2Mask) >> DescSize2Shift;
	if(Data1 != 0)
		*Data2 = txdesc->data2;

	gmacdev->TxBusy = synopGMAC_is_last_tx_desc(gmacdev,txdesc) ? 0 : txover + 1;

	if(synopGMAC_is_tx_desc_chained(txdesc)){
	   gmacdev->TxBusyDesc = (DmaDesc *)txdesc->data2;
		synopGMAC_tx_desc_init_chain(txdesc);
	}
	else{
		gmacdev->TxBusyDesc = synopGMAC_is_last_tx_desc(gmacdev,txdesc) ? gmacdev->TxDesc : (txdesc + 1);
		synopGMAC_tx_desc_init_ring(txdesc, synopGMAC_is_last_tx_desc(gmacdev,txdesc));
	}
	TR("(get)%02d %08x %08x %08x %08x %08x %08x %08x\n",txover,(u32)txdesc,txdesc->status,txdesc->length,txdesc->buffer1,txdesc->buffer2,txdesc->data1,txdesc->data2);

	return txover;
}

/**
  * Populate the tx desc structure with the buffer address.
  * Once the driver has a packet ready to be transmitted, this function is called with the
  * valid dma-able buffer addresses and their lengths. This function populates the descriptor
  * and make the DMA the owner for the descriptor. This function also controls whetther Checksum
  * offloading to be done in hardware or not.
  * This api is same for both ring mode and chain mode.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] Dma-able buffer1 pointer.
  * @param[in] length of buffer1 (Max is 2048).
  * @param[in] virtual pointer for buffer1.
  * @param[in] Dma-able buffer2 pointer.
  * @param[in] length of buffer2 (Max is 2048).
  * @param[in] virtual pointer for buffer2.
  * @param[in] u32 data indicating whether the descriptor is in ring mode or chain mode.
  * @param[in] u32 indicating whether the checksum offloading in HW/SW.
  * \return returns present tx descriptor index on success. Negative value if error.
  */
s32 synopGMAC_set_tx_qptr(synopGMACdevice * gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2,u32 offload_needed)
{
	u32 txnext      = gmacdev->TxNext;
	DmaDesc *txdesc = gmacdev->TxNextDesc;

	if(!synopGMAC_is_desc_empty(txdesc))
		return -1;

	(gmacdev->BusyTxDesc)++; //busy tx descriptor is reduced by one as it will be handed over to Processor now

	if(synopGMAC_is_tx_desc_chained(txdesc)){
		txdesc->length |= ((Length1 <<DescSize1Shift) & DescSize1Mask);
#ifdef ENH_DESC
		txdesc->status |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //ENH_DESC
#else
		txdesc->length |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //Its always assumed that complete data will fit in to one descriptor
#endif

	 	txdesc->buffer1 = Buffer1;
		txdesc->data1 = Data1;

		if(offload_needed){
			/*
			  Make sure that the OS you are running supports the IP and TCP checkusm offloaidng,
			  before calling any of the functions given below.
			*/
			synopGMAC_tx_checksum_offload_ipv4hdr(gmacdev, txdesc);
			synopGMAC_tx_checksum_offload_tcponly(gmacdev, txdesc);
	//		synopGMAC_tx_checksum_offload_tcp_pseudo(gmacdev, txdesc);
		}
#ifdef ENH_DESC
		txdesc->status |= DescOwnByDma;//ENH_DESC
#else
		txdesc->status = DescOwnByDma;
#endif

		gmacdev->TxNext = synopGMAC_is_last_tx_desc(gmacdev,txdesc) ? 0 : txnext + 1;
	   gmacdev->TxNextDesc = (DmaDesc *)txdesc->data2;
	}
	else{
		txdesc->length |= (((Length1 <<DescSize1Shift) & DescSize1Mask) | ((Length2 <<DescSize2Shift) & DescSize2Mask));
#ifdef ENH_DESC
		txdesc->status |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //ENH_DESC
#else
		txdesc->length |=  (DescTxFirst | DescTxLast | DescTxIntEnable); //Its always assumed that complete data will fit in to one descriptor
#endif

	 	txdesc->buffer1 = Buffer1;
		txdesc->data1 = Data1;

	 	txdesc->buffer2 = Buffer2;
		txdesc->data2 = Data2;

		if(offload_needed){
			/*
			  Make sure that the OS you are running supports the IP and TCP checkusm offloaidng,
			  before calling any of the functions given below.
			*/
			synopGMAC_tx_checksum_offload_ipv4hdr(gmacdev, txdesc);
			synopGMAC_tx_checksum_offload_tcponly(gmacdev, txdesc);
	//		synopGMAC_tx_checksum_offload_tcp_pseudo(gmacdev, txdesc);
		}
#ifdef ENH_DESC
		txdesc->status |= DescOwnByDma;//ENH_DESC
#else
		txdesc->status = DescOwnByDma;
#endif

		gmacdev->TxNext = synopGMAC_is_last_tx_desc(gmacdev,txdesc) ? 0 : txnext + 1;
		gmacdev->TxNextDesc = synopGMAC_is_last_tx_desc(gmacdev,txdesc) ? gmacdev->TxDesc : (txdesc + 1);
	}

#if SYNOP_TX_DEBUG
	printf("%02d %08x %08x %08x %08x %08x %08x %08x\n",txnext,(u32)txdesc,txdesc->status,txdesc->length,txdesc->buffer1,txdesc->buffer2,txdesc->data1,txdesc->data2);
#endif

	return txnext;
}

/**
 * Prepares the descriptor to receive packets.
 * The descriptor is allocated with the valid buffer addresses (sk_buff address) and the length fields
 * and handed over to DMA by setting the ownership. After successful return from this function the
 * descriptor is added to the receive descriptor pool/queue.
 * This api is same for both ring mode and chain mode.
 * @param[in] pointer to synopGMACdevice.
 * @param[in] Dma-able buffer1 pointer.
 * @param[in] length of buffer1 (Max is 2048).
 * @param[in] Dma-able buffer2 pointer.
 * @param[in] length of buffer2 (Max is 2048).
 * @param[in] u32 data indicating whether the descriptor is in ring mode or chain mode.
 * \return returns present rx descriptor index on success. Negative value if error.
 */
s32 synopGMAC_set_rx_qptr(synopGMACdevice * gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2)
{
	u32 rxnext      = gmacdev->RxNext;
	DmaDesc *rxdesc = gmacdev->RxNextDesc;

	if(!synopGMAC_is_desc_empty(rxdesc))
		return -1;

	if(synopGMAC_is_rx_desc_chained(rxdesc)){
		rxdesc->length |= ((Length1 <<DescSize1Shift) & DescSize1Mask);

		rxdesc->buffer1 = Buffer1;
		rxdesc->data1 = Data1;

		if((rxnext % MODULO_INTERRUPT) !=0)
			rxdesc->length |= RxDisIntCompl;

		rxdesc->status = DescOwnByDma;

		gmacdev->RxNext     = synopGMAC_is_last_rx_desc(gmacdev,rxdesc) ? 0 : rxnext + 1;
	   gmacdev->RxNextDesc = (DmaDesc *)rxdesc->data2;
	}
	else{
		rxdesc->length |= (((Length1 <<DescSize1Shift) & DescSize1Mask) | ((Length2 << DescSize2Shift) & DescSize2Mask));

		rxdesc->buffer1 = Buffer1;
		rxdesc->data1 = Data1;

		rxdesc->buffer2 = Buffer2;
		rxdesc->data2 = Data2;

		if((rxnext % MODULO_INTERRUPT) !=0)
			rxdesc->length |= RxDisIntCompl;

		rxdesc->status = DescOwnByDma;

		gmacdev->RxNext     = synopGMAC_is_last_rx_desc(gmacdev,rxdesc) ? 0 : rxnext + 1;
		gmacdev->RxNextDesc = synopGMAC_is_last_rx_desc(gmacdev,rxdesc) ? gmacdev->RxDesc : (rxdesc + 1);
	}
#if SYNOP_RX_DEBUG
	TR("%02d %08x %08x %08x %08x %08x %08x %08x\n",rxnext,(u32)rxdesc,rxdesc->status,rxdesc->length,rxdesc->buffer1,rxdesc->buffer2,rxdesc->data1,rxdesc->data2);
#endif
	(gmacdev->BusyRxDesc)++; //One descriptor will be given to Hardware. So busy count incremented by one
	return rxnext;
}

/**
 * Get back the descriptor from DMA after data has been received.
 * When the DMA indicates that the data is received (interrupt is generated), this function should be
 * called to get the descriptor and hence the data buffers received. With successful return from this
 * function caller gets the descriptor fields for processing. check the parameters to understand the
 * fields returned.`
 * @param[in] pointer to synopGMACdevice.
 * @param[out] pointer to hold the status of DMA.
 * @param[out] Dma-able buffer1 pointer.
 * @param[out] pointer to hold length of buffer1 (Max is 2048).
 * @param[out] virtual pointer for buffer1.
 * @param[out] Dma-able buffer2 pointer.
 * @param[out] pointer to hold length of buffer2 (Max is 2048).
 * @param[out] virtual pointer for buffer2.
 * \return returns present rx descriptor index on success. Negative value if error.
 */
s32 synopGMAC_get_rx_qptr(synopGMACdevice *gmacdev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 *Length2, u32 *Data2)
{
	u32 rxnext      = gmacdev->RxBusy;	// index of descriptor the DMA just completed. May be useful when data is spread over multiple buffers/descriptors
	DmaDesc *rxdesc = gmacdev->RxBusyDesc;

	if(synopGMAC_is_desc_owned_by_dma(rxdesc))
		return -1;
	if(synopGMAC_is_desc_empty(rxdesc))
		return -1;

	if(Status != 0)
		*Status = rxdesc->status;// send the status of this descriptor

	if(Length1 != 0)
		*Length1 = (rxdesc->length & DescSize1Mask) >> DescSize1Shift;
	if(Buffer1 != 0)
		*Buffer1 = rxdesc->buffer1;
	if(Data1 != 0)
		*Data1 = rxdesc->data1;

	if(Length2 != 0)
		*Length2 = (rxdesc->length & DescSize2Mask) >> DescSize2Shift;
	if(Buffer2 != 0)
		*Buffer2 = rxdesc->buffer2;
	if(Data1 != 0)
		*Data2 = rxdesc->data2;

	gmacdev->RxBusy = synopGMAC_is_last_rx_desc(gmacdev,rxdesc) ? 0 : rxnext + 1;

	if(synopGMAC_is_rx_desc_chained(rxdesc)){
	   gmacdev->RxBusyDesc = (DmaDesc *)rxdesc->data2;
		synopGMAC_rx_desc_init_chain(rxdesc);
//		synopGMAC_desc_init_chain(rxdesc, synopGMAC_is_last_rx_desc(gmacdev,rxdesc),0,0);
	}
	else{
		gmacdev->RxBusyDesc = synopGMAC_is_last_rx_desc(gmacdev,rxdesc) ? gmacdev->RxDesc : (rxdesc + 1);
		synopGMAC_rx_desc_init_ring(rxdesc, synopGMAC_is_last_rx_desc(gmacdev,rxdesc));
//		synopGMAC_rx_desc_recycle(rxdesc, synopGMAC_is_last_rx_desc(gmacdev,rxdesc));
	}
	TR("%02d %08x %08x %08x %08x %08x %08x %08x\n",rxnext,(u32)rxdesc,rxdesc->status,rxdesc->length,rxdesc->buffer1,rxdesc->buffer2,rxdesc->data1,rxdesc->data2);
	(gmacdev->BusyRxDesc)--; //This returns one descriptor to processor. So busy count will be decremented by one

	return(rxnext);
}

/**
 * The check summ offload engine is enabled to do only IPV4 header checksum.
 * IPV4 header Checksum is computed in the Hardware.
 * @param[in] pointer to synopGMACdevice.
 * @param[in] Pointer to tx descriptor for which  ointer to synopGMACdevice.
 * \return returns void.
 */
void synopGMAC_tx_checksum_offload_ipv4hdr(synopGMACdevice *gmacdev, DmaDesc *desc)
{
#ifdef ENH_DESC
	desc->status = ((desc->status & (~DescTxCisMask)) | DescTxCisIpv4HdrCs);//ENH_DESC
#else
	desc->length = ((desc->length & (~DescTxCisMask)) | DescTxCisIpv4HdrCs);
#endif

}

/**
 * The check summ offload engine is enabled to do TCPIP checsum assuming Pseudo header is available.
 * Hardware computes the tcp ip checksum assuming pseudo header checksum is computed in software.
 * Ipv4 header checksum is also inserted.
 * @param[in] pointer to synopGMACdevice.
 * @param[in] Pointer to tx descriptor for which  ointer to synopGMACdevice.
 * \return returns void.
 */
void synopGMAC_tx_checksum_offload_tcponly(synopGMACdevice *gmacdev, DmaDesc *desc)
{
#ifdef ENH_DESC
	desc->status = ((desc->status & (~DescTxCisMask)) | DescTxCisTcpOnlyCs);//ENH_DESC
#else
	desc->length = ((desc->length & (~DescTxCisMask)) | DescTxCisTcpOnlyCs);
#endif

}
