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

/**\file
 * This file defines the function prototypes for the Synopsys GMAC device and the
 * Marvell 88E1011/88E1011S integrated 10/100/1000 Gigabit Ethernet Transceiver.
 * Since the phy register mapping are standardised, the phy register map and the
 * bit definitions remain the same for other phy as well.
 * This also defines some of the Ethernet related parmeters.
 * \internal
 *  -----------------------------REVISION HISTORY------------------------------------
 * Synopsys			   01/Aug/2007				Created
 */
 
#ifndef SYNOP_GMAC_DEV_H
#define SYNOP_GMAC_DEV_H 1

/*******************************************************************/
#define SYNOP_LOOPBACK_MODE 0
#define SYNOP_LOOPBACK_DEBUG 0
#define SYNOP_PHY_LOOPBACK 0

#define SYNOP_TOP_DEBUG 0
#define SYNOP_REG_DEBUG 0
#define SYNOP_RX_DEBUG 0
#define SYNOP_TX_DEBUG 0
/*******************************************************************/

#include "synopGMAC_plat.h"

#define MACBASE 0x0000			// The Mac Base address offset is 0x0000
#define DMABASE 0x1000			// Dma base address starts with an offset 0x1000

#define TRANSMIT_DESC_SIZE  32 	//Tx Descriptors needed in the Descriptor pool/queue
#define RECEIVE_DESC_SIZE   32 	//Rx Descriptors needed in the Descriptor pool/queue

#define ETHERNET_HEADER         14	//6 byte Dest addr, 6 byte Src addr, 2 byte length/type
#define ETHERNET_CRC             4	//Ethernet CRC
#define ETHERNET_EXTRA		      2  //Only God knows about this?????
#define ETHERNET_PACKET_COPY	 250  //Maximum length when received data is copied on to a new skb
#define ETHERNET_PACKET_EXTRA	  18  //Preallocated length for the rx packets is MTU + ETHERNET_PACKET_EXTRA
#define VLAN_TAG		            4  //optional 802.1q VLAN Tag
#define MIN_ETHERNET_PAYLOAD    46  //Minimum Ethernet payload size
#define MAX_ETHERNET_PAYLOAD  1500  //Maximum Ethernet payload size
#define JUMBO_FRAME_PAYLOAD   9000  //Jumbo frame payload size

#define TX_BUF_SIZE        ETHERNET_HEADER + ETHERNET_CRC + MAX_ETHERNET_PAYLOAD + VLAN_TAG
#define RX_BUF_SIZE        ETHERNET_HEADER + ETHERNET_CRC + MAX_ETHERNET_PAYLOAD + VLAN_TAG


/*
DMA Descriptor Structure
The structure is common for both receive and transmit descriptors
The descriptor is of 4 words, but our structrue contains 6 words where
last two words are to hold the virtual address of the network buffer pointers
for driver's use
From the GMAC core release 3.50a onwards, the Enhanced Descriptor structure got changed.
The descriptor (both transmit and receive) are of 8 words each rather the 4 words of normal
descriptor structure.
Whenever IEEE 1588 Timestamping is enabled TX/RX DESC6 provides the lower 32 bits of Timestamp value and
                                           TX/RX DESC7 provides the upper 32 bits of Timestamp value
In addition to this whenever extended status bit is set (RX DESC0 bit 0), RX DESC4 contains the extended status information
*/

#define MODULO_INTERRUPT   1	// if it is set to 1, interrupt is available for all the descriptors or else interrupt is available only for
			     						// descriptor whose index%MODULO_INTERRUPT is zero

typedef struct DmaDescStruct    
{                               
	u32   status;         /* Status 									*/
	u32   length;         /* Buffer 1  and Buffer 2 length 						*/
	u32   buffer1;        /* Network Buffer 1 pointer (Dma-able) 							*/
	u32   buffer2;        /* Network Buffer 2 pointer or next descriptor pointer (Dma-able)in chain structure 	*/
	/* This data below is used only by driver					*/
	u32   data1;          /* This holds virtual address of buffer1, not used by DMA  			*/
	u32   data2;          /* This holds virtual address of buffer2, not used by DMA  			*/

	u32	dummy1;			//for addr align
	u32	dummy2;
} DmaDesc;

enum DescMode
{
	RINGMODE  = 0x00000001,
	CHAINMODE = 0x00000002,
};

enum BufferMode
{
	SINGLEBUF = 0x00000001,
	DUALBUF   = 0x00000002,
};

/* synopGMAC device data */

typedef struct synopGMACDeviceStruct
{
  u32 MacBase; 		       /* base address of MAC registers         */
  u32 DmaBase;         		 /* base address of DMA registers         */
  u32 PhyBase;          	 /* PHY device address on MII interface   */
  u32 Version;	             /* Gmac Revision version	          */		
	

  dma_addr_t TxDescDma;		 /* Dma-able address of first tx descriptor either in ring or chain mode, this is used by the GMAC device*/
  dma_addr_t RxDescDma; 	 /* Dma-albe address of first rx descriptor either in ring or chain mode, this is used by the GMAC device*/
  DmaDesc *TxDesc;          /* start address of TX descriptors ring or chain, this is used by the driver  */
  DmaDesc *RxDesc;          /* start address of RX descriptors ring or chain, this is used by the driver  */

  u32 BusyTxDesc;	          /* Number of Tx Descriptors owned by DMA at any given time*/
  u32 BusyRxDesc;		       /* Number of Rx Descriptors owned by DMA at any given time*/

  u32  RxDescCount;         /* number of rx descriptors in the tx descriptor queue/pool */
  u32  TxDescCount;         /* number of tx descriptors in the rx descriptor queue/pool */

  u32  TxBusy;              /* index of the tx descriptor owned by DMA, is obtained by synopGMAC_get_tx_qptr()                */
  u32  TxNext;              /* index of the tx descriptor next available with driver, given to DMA by synopGMAC_set_tx_qptr() */
  u32  RxBusy;              /* index of the rx descriptor owned by DMA, obtained by synopGMAC_get_rx_qptr()                   */
  u32  RxNext;              /* index of the rx descriptor next available with driver, given to DMA by synopGMAC_set_rx_qptr() */

  DmaDesc * TxBusyDesc;     /* Tx Descriptor address corresponding to the index TxBusy */
  DmaDesc * TxNextDesc;     /* Tx Descriptor address corresponding to the index TxNext */
  DmaDesc * RxBusyDesc;     /* Rx Descriptor address corresponding to the index TxBusy */
  DmaDesc * RxNextDesc;     /* Rx Descriptor address corresponding to the index RxNext */


  /*Phy related stuff*/
  u32 ClockDivMdc;		    /* Clock divider value programmed in the hardware           */
  /* The status of the link */
  u32 LinkState;		       /* Link status as reported by the Marvel Phy                */
  u32 DuplexMode;           /* Duplex mode of the Phy				    */
  u32 Speed;			       /* Speed of the Phy					    */
  u32 LoopBackMode; 		    /* Loopback status of the Phy				    */

} synopGMACdevice;


/**********************************************************
 * GMAC registers Map
 * For Pci based system address is BARx + GmacRegisterBase
 * For any other system translation is done accordingly
 **********************************************************/
enum GmacRegisters
{
  GmacConfig     	     = 0x0000,    /* Mac config Register                       */
  GmacFrameFilter  	  = 0x0004,    /* Mac frame filtering controls              */
  GmacHashHigh     	  = 0x0008,    /* Multi-cast hash table high                */
  GmacHashLow      	  = 0x000C,    /* Multi-cast hash table low                 */
  GmacGmiiAddr     	  = 0x0010,    /* GMII address Register(ext. Phy)           */
  GmacGmiiData     	  = 0x0014,    /* GMII data Register(ext. Phy)              */
  GmacFlowControl  	  = 0x0018,    /* Flow control Register                     */
  GmacVlan         	  = 0x001C,    /* VLAN tag Register (IEEE 802.1Q)           */
  
  GmacVersion     	  = 0x0020,    /* GMAC Core Version Register                */ 
  GmacWakeupAddr  	  = 0x0028,    /* GMAC wake-up frame filter adrress reg     */ 
  GmacPmtCtrlStatus    = 0x002C,    /* PMT control and status register           */ 
  
  GmacInterruptStatus	= 0x0038,    /* Mac Interrupt ststus register	       */  
  GmacInterruptMask     = 0x003C,    /* Mac Interrupt Mask register	       */  
 
  GmacAddr0High    	  = 0x0040,    /* Mac address0 high Register                */
  GmacAddr0Low    	  = 0x0044,    /* Mac address0 low Register                 */
  GmacAddr1High    	  = 0x0048,    /* Mac address1 high Register                */
  GmacAddr1Low     	  = 0x004C,    /* Mac address1 low Register                 */
  GmacAddr2High   	  = 0x0050,    /* Mac address2 high Register                */
  GmacAddr2Low     	  = 0x0054,    /* Mac address2 low Register                 */
  GmacAddr3High    	  = 0x0058,    /* Mac address3 high Register                */
  GmacAddr3Low     	  = 0x005C,    /* Mac address3 low Register                 */
  GmacAddr4High    	  = 0x0060,    /* Mac address4 high Register                */
  GmacAddr4Low     	  = 0x0064,    /* Mac address4 low Register                 */
  GmacAddr5High    	  = 0x0068,    /* Mac address5 high Register                */
  GmacAddr5Low     	  = 0x006C,    /* Mac address5 low Register                 */
  GmacAddr6High    	  = 0x0070,    /* Mac address6 high Register                */
  GmacAddr6Low     	  = 0x0074,    /* Mac address6 low Register                 */
  GmacAddr7High    	  = 0x0078,    /* Mac address7 high Register                */
  GmacAddr7Low     	  = 0x007C,    /* Mac address7 low Register                 */
  GmacAddr8High    	  = 0x0080,    /* Mac address8 high Register                */
  GmacAddr8Low     	  = 0x0084,    /* Mac address8 low Register                 */
  GmacAddr9High    	  = 0x0088,    /* Mac address9 high Register                */
  GmacAddr9Low      	  = 0x008C,    /* Mac address9 low Register                 */
  GmacAddr10High       = 0x0090,    /* Mac address10 high Register               */
  GmacAddr10Low    	  = 0x0094,    /* Mac address10 low Register                */
  GmacAddr11High   	  = 0x0098,    /* Mac address11 high Register               */
  GmacAddr11Low    	  = 0x009C,    /* Mac address11 low Register                */
  GmacAddr12High   	  = 0x00A0,    /* Mac address12 high Register               */
  GmacAddr12Low     	  = 0x00A4,    /* Mac address12 low Register                */
  GmacAddr13High   	  = 0x00A8,    /* Mac address13 high Register               */
  GmacAddr13Low   	  = 0x00AC,    /* Mac address13 low Register                */
  GmacAddr14High   	  = 0x00B0,    /* Mac address14 high Register               */
  GmacAddr14Low        = 0x00B4,    /* Mac address14 low Register                */
  GmacAddr15High       = 0x00B8,    /* Mac address15 high Register               */
  GmacAddr15Low  	     = 0x00BC,    /* Mac address15 low Register                */

  /*Time Stamp Register Map*/
  GmacTSControl	     = 0x0700,  /* Controls the Timestamp update logic                         : only when IEEE 1588 time stamping is enabled in corekit            */

  GmacTSSubSecIncr     = 0x0704,  /* 8 bit value by which sub second register is incremented     : only when IEEE 1588 time stamping without external timestamp input */

  GmacTSHigh  	        = 0x0708,  /* 32 bit seconds(MS)                                          : only when IEEE 1588 time stamping without external timestamp input */
  GmacTSLow   	        = 0x070C,  /* 32 bit nano seconds(MS)                                     : only when IEEE 1588 time stamping without external timestamp input */

  GmacTSHighUpdate     = 0x0710,  /* 32 bit seconds(MS) to be written/added/subtracted           : only when IEEE 1588 time stamping without external timestamp input */
  GmacTSLowUpdate      = 0x0714,  /* 32 bit nano seconds(MS) to be writeen/added/subtracted      : only when IEEE 1588 time stamping without external timestamp input */

  GmacTSAddend         = 0x0718,  /* Used by Software to readjust the clock frequency linearly   : only when IEEE 1588 time stamping without external timestamp input */

  GmacTSTargetTimeHigh = 0x071C,  /* 32 bit seconds(MS) to be compared with system time          : only when IEEE 1588 time stamping without external timestamp input */
  GmacTSTargetTimeLow  = 0x0720,  /* 32 bit nano seconds(MS) to be compared with system time     : only when IEEE 1588 time stamping without external timestamp input */

  GmacTSHighWord       = 0x0724,  /* Time Stamp Higher Word Register (Version 2 only); only lower 16 bits are valid                                                   */
//GmacTSHighWordUpdate = 0x072C,  /* Time Stamp Higher Word Update Register (Version 2 only); only lower 16 bits are valid                                            */

  GmacTSStatus         = 0x0728,  /* Time Stamp Status Register                                                                                                       */
};

/**********************************************************
 * GMAC DMA registers
 * For Pci based system address is BARx + GmaDmaBase
 * For any other system translation is done accordingly
 **********************************************************/

enum DmaRegisters
{
  DmaBusMode        = 0x0000,    /* CSR0 - Bus Mode Register                          */
  DmaTxPollDemand   = 0x0004,    /* CSR1 - Transmit Poll Demand Register              */
  DmaRxPollDemand   = 0x0008,    /* CSR2 - Receive Poll Demand Register               */
  DmaRxBaseAddr     = 0x000C,    /* CSR3 - Receive Descriptor list base address       */
  DmaTxBaseAddr     = 0x0010,    /* CSR4 - Transmit Descriptor list base address      */
  DmaStatus         = 0x0014,    /* CSR5 - Dma status Register                        */
  DmaControl        = 0x0018,    /* CSR6 - Dma Operation Mode Register                */
  DmaInterrupt      = 0x001C,    /* CSR7 - Interrupt enable                           */
  DmaMissedFr       = 0x0020,    /* CSR8 - Missed Frame & Buffer overflow Counter     */
  DmaTxCurrDesc     = 0x0048,    /*      - Current host Tx Desc Register              */
  DmaRxCurrDesc     = 0x004C,    /*      - Current host Rx Desc Register              */
  DmaTxCurrAddr     = 0x0050,    /* CSR20 - Current host transmit buffer address      */
  DmaRxCurrAddr     = 0x0054,    /* CSR21 - Current host receive buffer address       */
};

/**********************************************************
 * DMA Engine registers Layout
 **********************************************************/

/*DmaBusMode               = 0x0000,    CSR0 - Bus Mode */
enum DmaBusModeReg         
{                                         /* Bit description                                Bits     R/W   Reset value */
  DmaFixedBurstEnable     = 0x00010000,   /* (FB)Fixed Burst SINGLE, INCR4, INCR8 or INCR16   16     RW                */
  DmaFixedBurstDisable    = 0x00000000,   /*             SINGLE, INCR                                          0       */

  DmaTxPriorityRatio11    = 0x00000000,   /* (PR)TX:RX DMA priority ratio 1:1                15:14   RW        00      */
  DmaTxPriorityRatio21    = 0x00004000,   /* (PR)TX:RX DMA priority ratio 2:1                                          */
  DmaTxPriorityRatio31    = 0x00008000,   /* (PR)TX:RX DMA priority ratio 3:1                                          */
  DmaTxPriorityRatio41    = 0x0000C000,   /* (PR)TX:RX DMA priority ratio 4:1                                          */

  DmaBurstLengthx8        = 0x01000000,   /* When set mutiplies the PBL by 8                  24      RW        0      */

  DmaBurstLength256       = 0x01002000,   /*(DmaBurstLengthx8 | DmaBurstLength32) = 256      [24]:13:8                 */
  DmaBurstLength128       = 0x01001000,   /*(DmaBurstLengthx8 | DmaBurstLength16) = 128      [24]:13:8                 */
  DmaBurstLength64        = 0x01000800,   /*(DmaBurstLengthx8 | DmaBurstLength8) = 64        [24]:13:8                 */
  DmaBurstLength32        = 0x00002000,   /* (PBL) programmable Dma burst length = 32        13:8    RW                */
  DmaBurstLength16        = 0x00001000,   /* Dma burst length = 16                                                     */
  DmaBurstLength8         = 0x00000800,   /* Dma burst length = 8                                                      */
  DmaBurstLength4         = 0x00000400,   /* Dma burst length = 4                                                      */
  DmaBurstLength2         = 0x00000200,   /* Dma burst length = 2                                                      */
  DmaBurstLength1         = 0x00000100,   /* Dma burst length = 1                                                      */
  DmaBurstLength0         = 0x00000000,   /* Dma burst length = 0                                               0x00   */

  DmaDescriptor8Words     = 0x00000080,   /* Enh Descriptor works  1=> 8 word descriptor      7                  0    */
  DmaDescriptor4Words     = 0x00000000,   /* Enh Descriptor works  0=> 4 word descriptor      7                  0    */

  DmaDescriptorSkip16     = 0x00000040,   /* (DSL)Descriptor skip length (no.of dwords)       6:2     RW               */
  DmaDescriptorSkip8      = 0x00000020,   /* between two unchained descriptors                                         */
  DmaDescriptorSkip4      = 0x00000010,   /*                                                                           */
  DmaDescriptorSkip2      = 0x00000008,   /*                                                                           */
  DmaDescriptorSkip1      = 0x00000004,   /*                                                                           */
  DmaDescriptorSkip0      = 0x00000000,   /*                                                                    0x00   */

  DmaArbitRr              = 0x00000000,   /* (DA) DMA RR arbitration                            1     RW         0     */
  DmaArbitPr              = 0x00000002,   /* Rx has priority over Tx                                                   */

  DmaResetOn              = 0x00000001,   /* (SWR)Software Reset DMA engine                     0     RW               */
  DmaResetOff             = 0x00000000,   /*                                                                      0    */
};


/*DmaStatus         = 0x0014,    CSR5 - Dma status Register                        */
enum DmaStatusReg         
{ 
  /*Bit 28 27 and 26 indicate whether the interrupt due to PMT GMACMMC or GMAC LINE Remaining bits are DMA interrupts*/                      
  GmacPmtIntr             = 0x10000000,   /* (GPI)Gmac subsystem interrupt                      28     RO       0       */ 
  GmacMmcIntr             = 0x08000000,   /* (GMI)Gmac MMC subsystem interrupt                  27     RO       0       */ 
  GmacLineIntfIntr        = 0x04000000,   /* Line interface interrupt                           26     RO       0       */

  DmaErrorBit2            = 0x02000000,   /* (EB)Error bits 0-data buffer, 1-desc. access       25     RO       0       */
  DmaErrorBit1            = 0x01000000,   /* (EB)Error bits 0-write trnsf, 1-read transfr       24     RO       0       */
  DmaErrorBit0            = 0x00800000,   /* (EB)Error bits 0-Rx DMA, 1-Tx DMA                  23     RO       0       */

  DmaTxState              = 0x00700000,   /* (TS)Transmit process state                         22:20  RO               */
  DmaTxStopped            = 0x00000000,   /* Stopped - Reset or Stop Tx Command issued                         000      */
  DmaTxFetching           = 0x00100000,   /* Running - fetching the Tx descriptor                                       */
  DmaTxWaiting            = 0x00200000,   /* Running - waiting for status                                               */
  DmaTxReading            = 0x00300000,   /* Running - reading the data from host memory                                */
  DmaTxSuspended          = 0x00600000,   /* Suspended - Tx Descriptor unavailabe                                       */
  DmaTxClosing            = 0x00700000,   /* Running - closing Rx descriptor                                            */

  DmaRxState              = 0x000E0000,   /* (RS)Receive process state                         19:17  RO                */
  DmaRxStopped            = 0x00000000,   /* Stopped - Reset or Stop Rx Command issued                         000      */
  DmaRxFetching           = 0x00020000,   /* Running - fetching the Rx descriptor                                       */
  DmaRxWaiting            = 0x00060000,   /* Running - waiting for packet                                               */
  DmaRxSuspended          = 0x00080000,   /* Suspended - Rx Descriptor unavailable                                      */
  DmaRxClosing            = 0x000A0000,   /* Running - closing descriptor                                               */
  DmaRxQueuing            = 0x000E0000,   /* Running - queuing the recieve frame into host memory                       */

  DmaIntNormal            = 0x00010000,   /* (NIS)Normal interrupt summary                     16     RW        0       */
  DmaIntAbnormal          = 0x00008000,   /* (AIS)Abnormal interrupt summary                   15     RW        0       */

  DmaIntEarlyRx           = 0x00004000,   /* Early receive interrupt (Normal)       RW        0       */
  DmaIntBusError          = 0x00002000,   /* Fatal bus error (Abnormal)             RW        0       */
  DmaIntEarlyTx           = 0x00000400,   /* Early transmit interrupt (Abnormal)    RW        0       */
  DmaIntRxWdogTO          = 0x00000200,   /* Receive Watchdog Timeout (Abnormal)    RW        0       */
  DmaIntRxStopped         = 0x00000100,   /* Receive process stopped (Abnormal)     RW        0       */
  DmaIntRxNoBuffer        = 0x00000080,   /* Receive buffer unavailable (Abnormal)  RW        0       */
  DmaIntRxCompleted       = 0x00000040,   /* Completion of frame reception (Normal) RW        0       */
  DmaIntTxUnderflow       = 0x00000020,   /* Transmit underflow (Abnormal)          RW        0       */
  DmaIntRcvOverflow       = 0x00000010,   /* Receive Buffer overflow interrupt      RW        0       */
  DmaIntTxJabberTO        = 0x00000008,   /* Transmit Jabber Timeout (Abnormal)     RW        0       */
  DmaIntTxNoBuffer        = 0x00000004,   /* Transmit buffer unavailable (Normal)   RW        0       */
  DmaIntTxStopped         = 0x00000002,   /* Transmit process stopped (Abnormal)    RW        0       */
  DmaIntTxCompleted       = 0x00000001,   /* Transmit completed (Normal)            RW        0       */
};

/*DmaControl        = 0x0018,     CSR6 - Dma Operation Mode Register                */
enum DmaControlReg
{
  DmaDisableDropTcpCs	  = 0x04000000,   /* (DT) Dis. drop. of tcp/ip CS error frames        26      RW        0       */
                                        
  DmaStoreAndForward      = 0x00200000,   /* (SF)Store and forward                            21      RW        0       */
  DmaFlushTxFifo          = 0x00100000,   /* (FTF)Tx FIFO controller is reset to default      20      RW        0       */

  DmaTxThreshCtrl         = 0x0001C000,   /* (TTC)Controls thre Threh of MTL tx Fifo          16:14   RW                */
  DmaTxThreshCtrl16       = 0x0001C000,   /* (TTC)Controls thre Threh of MTL tx Fifo 16       16:14   RW                */
  DmaTxThreshCtrl24       = 0x00018000,   /* (TTC)Controls thre Threh of MTL tx Fifo 24       16:14   RW                */
  DmaTxThreshCtrl32       = 0x00014000,   /* (TTC)Controls thre Threh of MTL tx Fifo 32       16:14   RW                */
  DmaTxThreshCtrl40       = 0x00010000,   /* (TTC)Controls thre Threh of MTL tx Fifo 40       16:14   RW                */
  DmaTxThreshCtrl256      = 0x0000c000,   /* (TTC)Controls thre Threh of MTL tx Fifo 256      16:14   RW                */
  DmaTxThreshCtrl192      = 0x00008000,   /* (TTC)Controls thre Threh of MTL tx Fifo 192      16:14   RW                */
  DmaTxThreshCtrl128      = 0x00004000,   /* (TTC)Controls thre Threh of MTL tx Fifo 128      16:14   RW                */
  DmaTxThreshCtrl64       = 0x00000000,   /* (TTC)Controls thre Threh of MTL tx Fifo 64       16:14   RW        000     */
  
  DmaTxStart              = 0x00002000,   /* (ST)Start/Stop transmission                      13      RW        0       */

  DmaRxFlowCtrlDeact      = 0x00401800,   /* (RFD)Rx flow control deact. threhold             [22]:12:11   RW                 */
  DmaRxFlowCtrlDeact1K    = 0x00000000,   /* (RFD)Rx flow control deact. threhold (1kbytes)   [22]:12:11   RW        00       */
  DmaRxFlowCtrlDeact2K    = 0x00000800,   /* (RFD)Rx flow control deact. threhold (2kbytes)   [22]:12:11   RW                 */
  DmaRxFlowCtrlDeact3K    = 0x00001000,   /* (RFD)Rx flow control deact. threhold (3kbytes)   [22]:12:11   RW                 */
  DmaRxFlowCtrlDeact4K    = 0x00001800,   /* (RFD)Rx flow control deact. threhold (4kbytes)   [22]:12:11   RW                 */
  DmaRxFlowCtrlDeact5K    = 0x00400000,   /* (RFD)Rx flow control deact. threhold (4kbytes)   [22]:12:11   RW                 */
  DmaRxFlowCtrlDeact6K    = 0x00400800,   /* (RFD)Rx flow control deact. threhold (4kbytes)   [22]:12:11   RW                 */
  DmaRxFlowCtrlDeact7K    = 0x00401000,   /* (RFD)Rx flow control deact. threhold (4kbytes)   [22]:12:11   RW                 */

  DmaRxFlowCtrlAct        = 0x00800600,   /* (RFA)Rx flow control Act. threhold              [23]:10:09   RW                 */
  DmaRxFlowCtrlAct1K      = 0x00000000,   /* (RFA)Rx flow control Act. threhold (1kbytes)    [23]:10:09   RW        00       */
  DmaRxFlowCtrlAct2K      = 0x00000200,   /* (RFA)Rx flow control Act. threhold (2kbytes)    [23]:10:09   RW                 */
  DmaRxFlowCtrlAct3K      = 0x00000400,   /* (RFA)Rx flow control Act. threhold (3kbytes)    [23]:10:09   RW                 */
  DmaRxFlowCtrlAct4K      = 0x00000300,   /* (RFA)Rx flow control Act. threhold (4kbytes)    [23]:10:09   RW                 */
  DmaRxFlowCtrlAct5K      = 0x00800000,   /* (RFA)Rx flow control Act. threhold (5kbytes)    [23]:10:09   RW                 */
  DmaRxFlowCtrlAct6K      = 0x00800200,   /* (RFA)Rx flow control Act. threhold (6kbytes)    [23]:10:09   RW                 */
  DmaRxFlowCtrlAct7K      = 0x00800400,   /* (RFA)Rx flow control Act. threhold (7kbytes)    [23]:10:09   RW                 */

  DmaRxThreshCtrl         = 0x00000018,   /* (RTC)Controls thre Threh of MTL rx Fifo          4:3   RW                */
  DmaRxThreshCtrl64       = 0x00000000,   /* (RTC)Controls thre Threh of MTL tx Fifo 64       4:3   RW                */
  DmaRxThreshCtrl32       = 0x00000008,   /* (RTC)Controls thre Threh of MTL tx Fifo 32       4:3   RW                */
  DmaRxThreshCtrl96       = 0x00000010,   /* (RTC)Controls thre Threh of MTL tx Fifo 96       4:3   RW                */
  DmaRxThreshCtrl128      = 0x00000018,   /* (RTC)Controls thre Threh of MTL tx Fifo 128      4:3   RW                */

  DmaEnHwFlowCtrl         = 0x00000100,   /* (EFC)Enable HW flow control                      8       RW                 */
  DmaDisHwFlowCtrl        = 0x00000000,   /* Disable HW flow control                                            0        */

  DmaFwdErrorFrames       = 0x00000080,   /* (FEF)Forward error frames                        7       RW        0       */
  DmaFwdUnderSzFrames     = 0x00000040,   /* (FUF)Forward undersize frames                    6       RW        0       */
  DmaTxSecondFrame        = 0x00000004,   /* (OSF)Operate on second frame                     4       RW        0       */
  DmaRxStart              = 0x00000002,   /* (SR)Start/Stop reception                         1       RW        0       */
};


/*DmaInterrupt      = 0x001C,    CSR7 - Interrupt enable Register Layout     */
enum  DmaInterruptReg
{
  DmaIeNormal            = DmaIntNormal     ,   /* Normal interrupt enable                 RW        0       */
  DmaIeAbnormal          = DmaIntAbnormal   ,   /* Abnormal interrupt enable               RW        0       */

  DmaIeEarlyRx           = DmaIntEarlyRx    ,   /* Early receive interrupt enable          RW        0       */
  DmaIeBusError          = DmaIntBusError   ,   /* Fatal bus error enable                  RW        0       */
  DmaIeEarlyTx           = DmaIntEarlyTx    ,   /* Early transmit interrupt enable         RW        0       */
  DmaIeRxWdogTO          = DmaIntRxWdogTO   ,   /* Receive Watchdog Timeout enable         RW        0       */
  DmaIeRxStopped         = DmaIntRxStopped  ,   /* Receive process stopped enable          RW        0       */
  DmaIeRxNoBuffer        = DmaIntRxNoBuffer ,   /* Receive buffer unavailable enable       RW        0       */
  DmaIeRxCompleted       = DmaIntRxCompleted,   /* Completion of frame reception enable    RW        0       */
  DmaIeTxUnderflow       = DmaIntTxUnderflow,   /* Transmit underflow enable               RW        0       */

  DmaIeRxOverflow        = DmaIntRcvOverflow,   /* Receive Buffer overflow interrupt       RW        0       */
  DmaIeTxJabberTO        = DmaIntTxJabberTO ,   /* Transmit Jabber Timeout enable          RW        0       */
  DmaIeTxNoBuffer        = DmaIntTxNoBuffer ,   /* Transmit buffer unavailable enable      RW        0       */
  DmaIeTxStopped         = DmaIntTxStopped  ,   /* Transmit process stopped enable         RW        0       */
  DmaIeTxCompleted       = DmaIntTxCompleted,   /* Transmit completed enable               RW        0       */
};



/**********************************************************
 * DMA Engine descriptors
 **********************************************************/
#ifdef ENH_DESC
/*
**********Enhanced Descritpor structure to support 8K buffer per buffer ****************************

DmaRxBaseAddr     = 0x000C,   CSR3 - Receive Descriptor list base address
DmaRxBaseAddr is the pointer to the first Rx Descriptors. the Descriptor format in Little endian with a
32 bit Data bus is as shown below

Similarly
DmaTxBaseAddr     = 0x0010,  CSR4 - Transmit Descriptor list base address
DmaTxBaseAddr is the pointer to the first Rx Descriptors. the Descriptor format in Little endian with a
32 bit Data bus is as shown below
                  --------------------------------------------------------------------------
    RDES0	  |OWN (31)| Status                                                        |
 		  --------------------------------------------------------------------------
    RDES1	  | Ctrl | Res | Byte Count Buffer 2 | Ctrl | Res | Byte Count Buffer 1    |
		  --------------------------------------------------------------------------
    RDES2	  |  Buffer 1 Address                                                      |
		  --------------------------------------------------------------------------
    RDES3	  |  Buffer 2 Address / Next Descriptor Address                            |
		  --------------------------------------------------------------------------

                  --------------------------------------------------------------------------
    TDES0	  |OWN (31)| Ctrl | Res | Ctrl | Res | Status                              |
 		  --------------------------------------------------------------------------
    TDES1	  | Res | Byte Count Buffer 2 | Res |         Byte Count Buffer 1          |
		  --------------------------------------------------------------------------
    TDES2	  |  Buffer 1 Address                                                      |
		  --------------------------------------------------------------------------
    TDES3         |  Buffer 2 Address / Next Descriptor Address                            |
		  --------------------------------------------------------------------------

*/

enum DmaDescriptorStatus    /* status word of DMA descriptor */
{

  DescOwnByDma          = 0x80000000,   /* (OWN)Descriptor is owned by DMA engine              31      RW                  */

  DescDAFilterFail      = 0x40000000,   /* (AFM)Rx - DA Filter Fail for the rx frame           30                          */

  DescFrameLengthMask   = 0x3FFF0000,   /* (FL)Receive descriptor frame length                 29:16                       */
  DescFrameLengthShift  = 16,

  DescError             = 0x00008000,   /* (ES)Error summary bit  - OR of the follo. bits:     15                          */
					/*  DE || OE || IPC || LC || RWT || RE || CE */
  DescRxTruncated       = 0x00004000,   /* (DE)Rx - no more descriptors for receive frame      14                          */
  DescSAFilterFail      = 0x00002000,   /* (SAF)Rx - SA Filter Fail for the received frame     13                          */
  DescRxLengthError	    = 0x00001000,   /* (LE)Rx - frm size not matching with len field       12                          */
  DescRxDamaged         = 0x00000800,   /* (OE)Rx - frm was damaged due to buffer overflow     11                          */
  DescRxVLANTag         = 0x00000400,   /* (VLAN)Rx - received frame is a VLAN frame           10                          */
  DescRxFirst           = 0x00000200,   /* (FS)Rx - first descriptor of the frame              9                          */
  DescRxLast            = 0x00000100,   /* (LS)Rx - last descriptor of the frame               8                          */
  DescRxLongFrame       = 0x00000080,   /* (Giant Frame)Rx - frame is longer than 1518/1522    7                          */
  DescRxCollision       = 0x00000040,   /* (LC)Rx - late collision occurred during reception   6                          */
  DescRxFrameEther      = 0x00000020,   /* (FT)Rx - Frame type - Ethernet, otherwise 802.3     5                          */
  DescRxWatchdog        = 0x00000010,   /* (RWT)Rx - watchdog timer expired during reception   4                          */
  DescRxMiiError        = 0x00000008,   /* (RE)Rx - error reported by MII interface            3                          */
  DescRxDribbling       = 0x00000004,   /* (DE)Rx - frame contains non int multiple of 8 bits  2                          */
  DescRxCrc             = 0x00000002,   /* (CE)Rx - CRC error                                  1                          */
//  DescRxMacMatch        = 0x00000001,   /* (RX MAC Address) Rx mac address reg(1 to 15)match   0                          */

  DescRxEXTsts          = 0x00000001,   /* Extended Status Available (RDES4)                           0                          */

  DescTxIntEnable       = 0x40000000,   /* (IC)Tx - interrupt on completion                    30                       */
  DescTxLast            = 0x20000000,   /* (LS)Tx - Last segment of the frame                  29                       */
  DescTxFirst           = 0x10000000,   /* (FS)Tx - First segment of the frame                 28                       */
  DescTxDisableCrc      = 0x08000000,   /* (DC)Tx - Add CRC disabled (first segment only)      27                       */
  DescTxDisablePadd   	= 0x04000000,   /* (DP)disable padding, added by - reyaz               26                       */

  DescTxCisMask     	= 0x00c00000,   /* Tx checksum offloading control mask		       23:22			*/
  DescTxCisBypass   	= 0x00000000,   /* Checksum bypass								*/
  DescTxCisIpv4HdrCs	= 0x00400000,	/* IPv4 header checksum								*/
  DescTxCisTcpOnlyCs    = 0x00800000,	/* TCP/UDP/ICMP checksum. Pseudo header checksum is assumed to be present	*/
  DescTxCisTcpPseudoCs  = 0x00c00000,	/* TCP/UDP/ICMP checksum fully in hardware including pseudo header		*/

  TxDescEndOfRing       = 0x00200000,   /* (TER)End of descriptors ring                        21                       */
  TxDescChain           = 0x00100000,   /* (TCH)Second buffer address is chain address         20                       */

  DescRxChkBit0		    = 0x00000001,   /*()  Rx - Rx Payload Checksum Error                   0                          */
  DescRxChkBit7	    	= 0x00000080,   /* (IPC CS ERROR)Rx - Ipv4 header checksum error       7                          */
  DescRxChkBit5 		= 0x00000020,   /* (FT)Rx - Frame type - Ethernet, otherwise 802.3     5                          */

  DescRxTSavail         = 0x00000080,   /* Time stamp available                                7                          */
  DescRxFrameType   	= 0x00000020,   /* (FT)Rx - Frame type - Ethernet, otherwise 802.3     5                          */

  DescTxIpv4ChkError    = 0x00010000,   /* (IHE) Tx Ip header error                            16                         */
  DescTxTimeout         = 0x00004000,   /* (JT)Tx - Transmit jabber timeout                    14                         */
  DescTxFrameFlushed    = 0x00002000,   /* (FF)Tx - DMA/MTL flushed the frame due to SW flush  13                         */
  DescTxPayChkError     = 0x00001000,   /* (PCE) Tx Payload checksum Error                     12                         */
  DescTxLostCarrier     = 0x00000800,   /* (LC)Tx - carrier lost during tramsmission           11                         */
  DescTxNoCarrier       = 0x00000400,   /* (NC)Tx - no carrier signal from the tranceiver      10                         */
  DescTxLateCollision   = 0x00000200,   /* (LC)Tx - transmission aborted due to collision      9                         */
  DescTxExcCollisions   = 0x00000100,   /* (EC)Tx - transmission aborted after 16 collisions   8                         */
  DescTxVLANFrame       = 0x00000080,   /* (VF)Tx - VLAN-type frame                            7                         */

  DescTxCollMask        = 0x00000078,   /* (CC)Tx - Collision count                            6:3                        */
  DescTxCollShift       = 3,

  DescTxExcDeferral     = 0x00000004,   /* (ED)Tx - excessive deferral                         2                        */
  DescTxUnderflow       = 0x00000002,   /* (UF)Tx - late data arrival from the memory          1                        */
  DescTxDeferred        = 0x00000001,   /* (DB)Tx - frame transmision deferred                 0                        */

	/*
	This explains the RDES1/TDES1 bits layout
			  --------------------------------------------------------------------
	    RDES1/TDES1  | Control Bits | Byte Count Buffer 2 | Byte Count Buffer 1          |
			  --------------------------------------------------------------------

	*/
// DmaDescriptorLength     length word of DMA descriptor


  RxDisIntCompl		= 0x80000000,	/* (Disable Rx int on completion) 			31			*/
  RxDescEndOfRing       = 0x00008000,   /* (TER)End of descriptors ring                         15                       */
  RxDescChain           = 0x00004000,   /* (TCH)Second buffer address is chain address          14                       */


  DescSize2Mask         = 0x1FFF0000,   /* (TBS2) Buffer 2 size                                28:16                    */
  DescSize2Shift        = 16,
  DescSize1Mask         = 0x00001FFF,   /* (TBS1) Buffer 1 size                                12:0                     */
  DescSize1Shift        = 0,


	/*
	This explains the RDES4 Extended Status bits layout
			   --------------------------------------------------------------------
	  RDES4   |                             Extended Status                        |
			   --------------------------------------------------------------------
	*/
  DescRxPtpAvail        = 0x00004000,    /* PTP snapshot available                              14                        */
  DescRxPtpVer          = 0x00002000,    /* When set indicates IEEE1584 Version 2 (else Ver1)   13                        */
  DescRxPtpFrameType    = 0x00001000,    /* PTP frame type Indicates PTP sent over ethernet     12                        */
  DescRxPtpMessageType  = 0x00000F00,    /* Message Type                                        11:8                      */
  DescRxPtpNo           = 0x00000000,    /* 0000 => No PTP message received                                               */
  DescRxPtpSync         = 0x00000100,    /* 0001 => Sync (all clock types) received                                       */
  DescRxPtpFollowUp     = 0x00000200,    /* 0010 => Follow_Up (all clock types) received                                  */
  DescRxPtpDelayReq     = 0x00000300,    /* 0011 => Delay_Req (all clock types) received                                  */
  DescRxPtpDelayResp    = 0x00000400,    /* 0100 => Delay_Resp (all clock types) received                                 */
  DescRxPtpPdelayReq    = 0x00000500,    /* 0101 => Pdelay_Req (in P to P tras clk)  or Announce in Ord and Bound clk     */
  DescRxPtpPdelayResp   = 0x00000600,    /* 0110 => Pdealy_Resp(in P to P trans clk) or Management in Ord and Bound clk   */
  DescRxPtpPdelayRespFP = 0x00000700,    /* 0111 => Pdealy_Resp_Follow_Up (in P to P trans clk) or Signaling in Ord and Bound clk   */
  DescRxPtpIPV6         = 0x00000080,    /* Received Packet is  in IPV6 Packet                  7                         */
  DescRxPtpIPV4         = 0x00000040,    /* Received Packet is  in IPV4 Packet                  6                         */

  DescRxChkSumBypass    = 0x00000020,    /* When set indicates checksum offload engine          5
                                            is bypassed                                                                   */
  DescRxIpPayloadError  = 0x00000010,    /* When set indicates 16bit IP payload CS is in error  4                         */
  DescRxIpHeaderError   = 0x00000008,    /* When set indicates 16bit IPV4 header CS is in       3
                                            error or IP datagram version is not consistent
                                            with Ethernet type value                                                      */
  DescRxIpPayloadType   = 0x00000007,     /* Indicate the type of payload encapsulated          2:0
                                             in IPdatagram processed by COE (Rx)                                          */
  DescRxIpPayloadUnknown= 0x00000000,     /* Unknown or didnot process IP payload                                         */
  DescRxIpPayloadUDP    = 0x00000001,     /* UDP                                                                          */
  DescRxIpPayloadTCP    = 0x00000002,     /* TCP                                                                          */
  DescRxIpPayloadICMP   = 0x00000003,     /* ICMP                                                                         */

};

#else
/*

********** Default Descritpor structure  ****************************
DmaRxBaseAddr     = 0x000C,   CSR3 - Receive Descriptor list base address
DmaRxBaseAddr is the pointer to the first Rx Descriptors. the Descriptor format in Little endian with a
32 bit Data bus is as shown below

Similarly
DmaTxBaseAddr     = 0x0010,  CSR4 - Transmit Descriptor list base address
DmaTxBaseAddr is the pointer to the first Rx Descriptors. the Descriptor format in Little endian with a
32 bit Data bus is as shown below
                  --------------------------------------------------------------------
    RDES0/TDES0  |OWN (31)| Status                                                   |
 		  --------------------------------------------------------------------
    RDES1/TDES1  | Control Bits | Byte Count Buffer 2 | Byte Count Buffer 1          |
		  --------------------------------------------------------------------
    RDES2/TDES2  |  Buffer 1 Address                                                 |
		  --------------------------------------------------------------------
    RDES3/TDES3  |  Buffer 2 Address / Next Descriptor Address                       |
		  --------------------------------------------------------------------
*/
enum DmaDescriptorStatus    /* status word of DMA descriptor */
{
  DescOwnByDma          = 0x80000000,   /* (OWN)Descriptor is owned by DMA engine            31      RW                  */

  DescDAFilterFail      = 0x40000000,   /* (AFM)Rx - DA Filter Fail for the rx frame         30                          */

  DescFrameLengthMask   = 0x3FFF0000,   /* (FL)Receive descriptor frame length               29:16                       */
  DescFrameLengthShift  = 16,

  DescError             = 0x00008000,   /* (ES)Error summary bit  - OR of the follo. bits:   15                          */
					/*  DE || OE || IPC || LC || RWT || RE || CE */
  DescRxTruncated       = 0x00004000,   /* (DE)Rx - no more descriptors for receive frame    14                          */
  DescSAFilterFail      = 0x00002000,   /* (SAF)Rx - SA Filter Fail for the received frame   13                          */
  DescRxLengthError	= 0x00001000,   /* (LE)Rx - frm size not matching with len field     12                          */
  DescRxDamaged         = 0x00000800,   /* (OE)Rx - frm was damaged due to buffer overflow   11                          */
  DescRxVLANTag         = 0x00000400,   /* (VLAN)Rx - received frame is a VLAN frame         10                          */
  DescRxFirst           = 0x00000200,   /* (FS)Rx - first descriptor of the frame             9                          */
  DescRxLast            = 0x00000100,   /* (LS)Rx - last descriptor of the frame              8                          */
  DescRxLongFrame       = 0x00000080,   /* (Giant Frame)Rx - frame is longer than 1518/1522   7                          */
  DescRxCollision       = 0x00000040,   /* (LC)Rx - late collision occurred during reception  6                          */
  DescRxFrameEther      = 0x00000020,   /* (FT)Rx - Frame type - Ethernet, otherwise 802.3    5                          */
  DescRxWatchdog        = 0x00000010,   /* (RWT)Rx - watchdog timer expired during reception  4                          */
  DescRxMiiError        = 0x00000008,   /* (RE)Rx - error reported by MII interface           3                          */
  DescRxDribbling       = 0x00000004,   /* (DE)Rx - frame contains non int multiple of 8 bits 2                          */
  DescRxCrc             = 0x00000002,   /* (CE)Rx - CRC error                                 1                          */
  DescRxMacMatch        = 0x00000001,   /* (RX MAC Address) Rx mac address reg(1 to 15)match  0                          */

  //Rx Descriptor Checksum Offload engine (type 2) encoding
  //DescRxPayChkError     = 0x00000001,   /* ()  Rx - Rx Payload Checksum Error                 0                          */
  //DescRxIpv4ChkError    = 0x00000080,   /* (IPC CS ERROR)Rx - Ipv4 header checksum error      7                          */

  DescRxChkBit0		= 0x00000001,   /*()  Rx - Rx Payload Checksum Error                  0                          */
  DescRxChkBit7		= 0x00000080,   /* (IPC CS ERROR)Rx - Ipv4 header checksum error      7                          */
  DescRxChkBit5		= 0x00000020,   /* (FT)Rx - Frame type - Ethernet, otherwise 802.3    5                          */

  DescTxIpv4ChkError    = 0x00010000,   /* (IHE) Tx Ip header error                           16                         */
  DescTxTimeout         = 0x00004000,   /* (JT)Tx - Transmit jabber timeout                   14                         */
  DescTxFrameFlushed    = 0x00002000,   /* (FF)Tx - DMA/MTL flushed the frame due to SW flush 13                         */
  DescTxPayChkError     = 0x00001000,   /* (PCE) Tx Payload checksum Error                    12                         */
  DescTxLostCarrier     = 0x00000800,   /* (LC)Tx - carrier lost during tramsmission          11                         */
  DescTxNoCarrier       = 0x00000400,   /* (NC)Tx - no carrier signal from the tranceiver     10                         */
  DescTxLateCollision   = 0x00000200,   /* (LC)Tx - transmission aborted due to collision      9                         */
  DescTxExcCollisions   = 0x00000100,   /* (EC)Tx - transmission aborted after 16 collisions   8                         */
  DescTxVLANFrame       = 0x00000080,   /* (VF)Tx - VLAN-type frame                            7                         */

  DescTxCollMask        = 0x00000078,   /* (CC)Tx - Collision count                           6:3                        */
  DescTxCollShift       = 3,

  DescTxExcDeferral     = 0x00000004,   /* (ED)Tx - excessive deferral                          2                        */
  DescTxUnderflow       = 0x00000002,   /* (UF)Tx - late data arrival from the memory           1                        */
  DescTxDeferred        = 0x00000001,   /* (DB)Tx - frame transmision deferred                  0                        */

	/*
	This explains the RDES1/TDES1 bits layout
			  --------------------------------------------------------------------
	    RDES1/TDES1  | Control Bits | Byte Count Buffer 2 | Byte Count Buffer 1          |
			  --------------------------------------------------------------------

	*/
//DmaDescriptorLength     length word of DMA descriptor

  DescTxIntEnable       = 0x80000000,   /* (IC)Tx - interrupt on completion                    31                       */
  DescTxLast            = 0x40000000,   /* (LS)Tx - Last segment of the frame                  30                       */
  DescTxFirst           = 0x20000000,   /* (FS)Tx - First segment of the frame                 29                       */
  DescTxDisableCrc      = 0x04000000,   /* (DC)Tx - Add CRC disabled (first segment only)      26                       */

  RxDisIntCompl		= 0x80000000,	/* (Disable Rx int on completion) 			31			*/
  RxDescEndOfRing       = 0x02000000,   /* (TER)End of descriptors ring                                                 */
  RxDescChain           = 0x01000000,   /* (TCH)Second buffer address is chain address         24                       */

  DescTxDisablePadd	= 0x00800000,   /* (DP)disable padding, added by - reyaz               23                       */

  TxDescEndOfRing       = 0x02000000,   /* (TER)End of descriptors ring                                                 */
  TxDescChain           = 0x01000000,   /* (TCH)Second buffer address is chain address         24                       */

  DescSize2Mask         = 0x003FF800,   /* (TBS2) Buffer 2 size                                21:11                    */
  DescSize2Shift        = 11,
  DescSize1Mask         = 0x000007FF,   /* (TBS1) Buffer 1 size                                10:0                     */
  DescSize1Shift        = 0,


  DescTxCisMask  	= 0x18000000,   /* Tx checksum offloading control mask			28:27			*/
  DescTxCisBypass	= 0x00000000,   /* Checksum bypass								*/
  DescTxCisIpv4HdrCs	= 0x08000000,	/* IPv4 header checksum								*/
  DescTxCisTcpOnlyCs    = 0x10000000,	/* TCP/UDP/ICMP checksum. Pseudo header checksum is assumed to be present	*/
  DescTxCisTcpPseudoCs  = 0x18000000,	/* TCP/UDP/ICMP checksum fully in hardware including pseudo header		*/
};
#endif


s32 synopGMAC_reset(synopGMACdevice *gmacdev);
void synopGMAC_disable_interrupt_all(synopGMACdevice *gmacdev);
void synopGMAC_rx_desc_init_ring(DmaDesc *desc, int last_ring_desc);
void synopGMAC_tx_desc_init_ring(DmaDesc *desc, int last_ring_desc);
void synopGMAC_rx_desc_init_chain(DmaDesc * desc);
void synopGMAC_tx_desc_init_chain(DmaDesc * desc);
s32 synopGMAC_init_tx_rx_desc_queue(synopGMACdevice *gmacdev);
void synopGMAC_resume_dma_tx(synopGMACdevice * gmacdev);
void synopGMAC_init_rx_desc_base(synopGMACdevice *gmacdev);
void synopGMAC_init_tx_desc_base(synopGMACdevice *gmacdev);
int synopGMAC_is_desc_owned_by_dma(DmaDesc *desc);
u32 synopGMAC_get_rx_desc_frame_length(u32 status);
int synopGMAC_is_desc_valid(u32 status);
int synopGMAC_is_desc_empty(DmaDesc *desc);
int synopGMAC_is_rx_desc_valid(u32 status);
int synopGMAC_is_last_rx_desc(synopGMACdevice * gmacdev,DmaDesc *desc);
int synopGMAC_is_last_tx_desc(synopGMACdevice * gmacdev,DmaDesc *desc);
int synopGMAC_is_rx_desc_chained(DmaDesc * desc);
int synopGMAC_is_tx_desc_chained(DmaDesc * desc);
s32 synopGMAC_get_tx_qptr(synopGMACdevice * gmacdev, u32 * Status, u32 * Buffer1, u32 * Length1, u32 * Data1, u32 * Buffer2, u32 * Length2, u32 * Data2 );
s32 synopGMAC_set_tx_qptr(synopGMACdevice * gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2,u32 offload_needed);
s32 synopGMAC_set_rx_qptr(synopGMACdevice * gmacdev, u32 Buffer1, u32 Length1, u32 Data1, u32 Buffer2, u32 Length2, u32 Data2);
s32 synopGMAC_get_rx_qptr(synopGMACdevice *gmacdev, u32 *Status, u32 *Buffer1, u32 *Length1, u32 *Data1, u32 *Buffer2, u32 *Length2, u32 *Data2);
void synopGMAC_tx_checksum_offload_ipv4hdr(synopGMACdevice *gmacdev, DmaDesc *desc);
void synopGMAC_tx_checksum_offload_tcponly(synopGMACdevice *gmacdev, DmaDesc *desc);


#endif /* End of file */

