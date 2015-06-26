/** \file
 * This is the network dependent layer to handle network related functionality.
 * This file is tightly coupled to neworking frame work of linux 2.6.xx kernel.
 * The functionality carried out in this file should be treated as an example only
 * if the underlying operating system is not Linux. 
 * 
 * \note Many of the functions other than the device specific functions
 *  changes for operating system other than Linux 2.6.xx
 * \internal 
 *-----------------------------REVISION HISTORY-----------------------------------
 * Synopsys			01/Aug/2007				Created
 */

#include "synopGMAC_Host.h"
#include "synopGMAC_plat.h"
#include "synopGMAC_network_interface.h"
#include "synopGMAC_Dev.h"

static struct synopGMACNetworkAdapter *gmac_adapter;
static unsigned int rx_tx_ok = 0;
static u32 GMAC_Power_down; // This global variable is used to indicate the ISR whether the interrupts occured in the process of powering down the mac or not


static int rtl88e1111_config_init(synopGMACdevice *gmacdev)
{
	int err;
	u16 data;

	synopGMAC_read_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,&data);
	data = data | 0x82;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,data);
	synopGMAC_read_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,&data);
	data = data | 0x8000;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,data);
#if SYNOP_PHY_LOOPBACK
	synopGMAC_read_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,&data);
	data = data | 0x70;
	data = data & 0xffdf;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x14,data);
	data = 0x8000;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,data);
	data = 0x5140;
	err = synopGMAC_write_phy_reg(gmacdev->MacBase,gmacdev->PhyBase,0x00,data);
#endif
	if (err < 0)
		return err;
	return 0;
}

s32 synopGMAC_check_phy_init (synopGMACPciNetworkAdapter *adapter);

/**

 * Function used to detect the cable plugging and unplugging.
 * This function gets scheduled once in every second and polls
 * the PHY register for network cable plug/unplug. Once the 
 * connection is back the GMAC device is configured as per
 * new Duplex mode and Speed of the connection.

 * @param[in] u32 type but is not used currently. 
 * \return returns void.
 * \note This function is tightly coupled with Linux 2.6.xx.
 * \callgraph
 */
static void synopGMAC_linux_cable_unplug_function(synopGMACPciNetworkAdapter *adapter)
{
	s32 data;
	synopGMACdevice *gmacdev = adapter->synopGMACdev;

/*	if (!mii_link_ok(&adapter->mii)) {
		if(gmacdev->LinkState)
			TR("No Link\n");
		gmacdev->DuplexMode = 0;
		gmacdev->Speed = 0;
		gmacdev->LoopBackMode = 0;
		gmacdev->LinkState = 0;
	} else {
		data = synopGMAC_check_phy_init(adapter);

		if(gmacdev->LinkState != data) {
			gmacdev->LinkState = data;
			synopGMAC_mac_init(gmacdev);
			TR("Link UP data=%08x\n",data);
			TR("Link is up in %s mode\n",(gmacdev->DuplexMode == FULLDUPLEX) ? "FULL DUPLEX": "HALF DUPLEX");
			if(gmacdev->Speed == SPEED1000)	
				TR("Link is with 1000M Speed \n");
			if(gmacdev->Speed == SPEED100)	
				TR("Link is with 100M Speed \n");
			if(gmacdev->Speed == SPEED10)	
				TR("Link is with 10M Speed \n");
		}
	}*/

	data = synopGMAC_check_phy_init(adapter);

	if(gmacdev->LinkState != data) {
		gmacdev->LinkState = data;
		synopGMAC_mac_init(gmacdev);
		TR("Link UP data=%08x\n",data);
		TR("Link is up in %s mode\n",(gmacdev->DuplexMode == FULLDUPLEX) ? "FULL DUPLEX": "HALF DUPLEX");
		if(gmacdev->Speed == SPEED1000)	
			TR("Link is with 1000M Speed \n");
		if(gmacdev->Speed == SPEED100)	
			TR("Link is with 100M Speed \n");
		if(gmacdev->Speed == SPEED10)	
			TR("Link is with 10M Speed \n");
	}
}

s32 synopGMAC_check_phy_init (synopGMACPciNetworkAdapter *adapter)
{
	synopGMACdevice *gmacdev = adapter->synopGMACdev;
/*	struct ethtool_cmd cmd;

	if(!mii_link_ok(&adapter->mii)) {
		gmacdev->DuplexMode = FULLDUPLEX;
		gmacdev->Speed      = SPEED100;
		return 0;
	} else {
		mii_ethtool_gset(&adapter->mii, &cmd);

		gmacdev->DuplexMode = (cmd.duplex == DUPLEX_FULL)  ? FULLDUPLEX: HALFDUPLEX;
		if(cmd.speed == SPEED_1000)
			gmacdev->Speed = SPEED1000;
		else if(cmd.speed == SPEED_100)
			gmacdev->Speed = SPEED100;
		else
			gmacdev->Speed = SPEED10;
	}*/

	gmacdev->DuplexMode = FULLDUPLEX;
	gmacdev->Speed      = SPEED100;

	return gmacdev->Speed|(gmacdev->DuplexMode<<4);
}

static void synopGMAC_linux_powerup_mac(synopGMACdevice *gmacdev)
{
	GMAC_Power_down = 0;	// Let ISR know that MAC is out of power down now
	if( synopGMAC_is_magic_packet_received(gmacdev))
		TR("GMAC wokeup due to Magic Pkt Received\n");
	if(synopGMAC_is_wakeup_frame_received(gmacdev))
		TR("GMAC wokeup due to Wakeup Frame Received\n");
	//Disable the assertion of PMT interrupt
	synopGMAC_pmt_int_disable(gmacdev);
	//Enable the mac and Dma rx and tx paths
	synopGMAC_rx_enable(gmacdev);
	synopGMAC_enable_dma_rx(gmacdev);

	synopGMAC_tx_enable(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);
}


/**
  * This sets up the transmit Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures for ring mode or chain mode.
  * This function depends on the pcidev structure for allocation consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in tx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->TxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements  
  */
static s32 synopGMAC_setup_tx_desc_queue(synopGMACdevice *gmacdev, void *dev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->TxDescCount = 0;




	TR("Total size of memory required for Tx Descriptors in Ring Mode = 0x%08x\n",
		((sizeof(DmaDesc) * no_of_desc)));
	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc) * no_of_desc, &dma_addr);
	if(first_desc == NULL){
		TR("Error in Tx Descriptors memory allocation\n");
		return -ESYNOPGMACNOMEM;
	}

	gmacdev->TxDescCount = no_of_desc;
	gmacdev->TxDesc      = first_desc;
	gmacdev->TxDescDma   = dma_addr;
	TR("Tx Descriptors in Ring Mode: No. of descriptors = %d base = 0x%08x dma = 0x%08x\n",
		no_of_desc, (u32)first_desc, dma_addr);

	for(i =0; i < gmacdev -> TxDescCount; i++){
		synopGMAC_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount-1);
		TR("%02d %08x \n",i, (unsigned int)(gmacdev->TxDesc + i));
	}

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->TxNextDesc = gmacdev->TxDesc;
	gmacdev->TxBusyDesc = gmacdev->TxDesc;
	gmacdev->BusyTxDesc  = 0; 

	return -ESYNOPGMACNOERR;
}


/**
  * This sets up the receive Descriptor queue in ring or chain mode.
  * This function is tightly coupled to the platform and operating system
  * Device is interested only after the descriptors are setup. Therefore this function
  * is not included in the device driver API. This function should be treated as an
  * example code to design the descriptor structures in ring mode or chain mode.
  * This function depends on the pcidev structure for allocation of consistent dma-able memory in case of linux.
  * This limitation is due to the fact that linux uses pci structure to allocate a dmable memory
  *	- Allocates the memory for the descriptors.
  *	- Initialize the Busy and Next descriptors indices to 0(Indicating first descriptor).
  *	- Initialize the Busy and Next descriptors to first descriptor address.
  * 	- Initialize the last descriptor with the endof ring in case of ring mode.
  *	- Initialize the descriptors in chain mode.
  * @param[in] pointer to synopGMACdevice.
  * @param[in] pointer to pci_device structure.
  * @param[in] number of descriptor expected in rx descriptor queue.
  * @param[in] whether descriptors to be created in RING mode or CHAIN mode.
  * \return 0 upon success. Error code upon failure.
  * \note This function fails if allocation fails for required number of descriptors in Ring mode, but in chain mode
  *  function returns -ESYNOPGMACNOMEM in the process of descriptor chain creation. once returned from this function
  *  user should for gmacdev->RxDescCount to see how many descriptors are there in the chain. Should continue further
  *  only if the number of descriptors in the chain meets the requirements  
  */
static s32 synopGMAC_setup_rx_desc_queue(synopGMACdevice *gmacdev, void *dev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->RxDescCount = 0;




	TR("total size of memory required for Rx Descriptors in Ring Mode = 0x%08x\n",
		((sizeof(DmaDesc) * no_of_desc)));
	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc) * no_of_desc, &dma_addr);
	if(first_desc == NULL){
		TR("Error in Rx Descriptor Memory allocation in Ring mode\n");
		return -ESYNOPGMACNOMEM;
	}

	gmacdev->RxDescCount = no_of_desc;
	gmacdev->RxDesc      = first_desc;
	gmacdev->RxDescDma   = dma_addr;
	TR("Rx Descriptors in Ring Mode: No. of descriptors = %d base = 0x%08x dma = 0x%08x\n",
		no_of_desc, (u32)first_desc, dma_addr);

	for(i =0; i < gmacdev -> RxDescCount; i++){
		synopGMAC_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount-1);
		TR("%02d %08x \n",i, (unsigned int)(gmacdev->RxDesc + i));
	}

	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;
	gmacdev->RxNextDesc = gmacdev->RxDesc;
	gmacdev->RxBusyDesc = gmacdev->RxDesc;
	gmacdev->BusyRxDesc   = 0; 

	return -ESYNOPGMACNOERR;
}


/**
 * Function to handle housekeeping after a packet is transmitted over the wire.
 * After the transmission of a packet DMA generates corresponding interrupt 
 * (if it is enabled). It takes care of returning the sk_buff to the linux
 * kernel, updating the networking statistics and tracking the descriptors.
 * @param[in] pointer to net_device structure. 
 * \return void.
 * \note This function runs in interrupt context
 */
void synop_handle_transmit_over(struct synopGMACNetworkAdapter *tp)
{
	synopGMACdevice * gmacdev;
	s32 desc_index;
	u32 data1, data2;
	u32 status;
	u32 length1, length2;
	u32 dma_addr1, dma_addr2;
#ifdef ENH_DESC_8W
	u32 ext_status;
	u16 time_stamp_higher;
	u32 time_stamp_high;
	u32 time_stamp_low;
#endif
	
	gmacdev = tp->synopGMACdev;

	/*Handle the transmit Descriptors*/
	do {
#ifdef ENH_DESC_8W
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2,&ext_status,&time_stamp_high,&time_stamp_low);
        synopGMAC_TS_read_timestamp_higher_val(gmacdev, &time_stamp_higher);
#else
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
#endif
//		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr, &length, &data1);
		if(desc_index >= 0 && data1 != 0){
		#ifdef	IPC_OFFLOAD
			if(synopGMAC_is_tx_ipv4header_checksum_error(gmacdev, status)){
				TR("Harware Failed to Insert IPV4 Header Checksum\n");
			}
			if(synopGMAC_is_tx_payload_checksum_error(gmacdev, status)){
				TR("Harware Failed to Insert Payload Checksum\n");
			}
		#endif
		
//			plat_free_memory((void *)(data1));	//sw:	data1 = buffer1
			plat_free_memory((void *)((data1 & 0x0fffffff) | 0x80000000));
			
			if(synopGMAC_is_desc_valid(status)){
				tp->synopGMACNetStats.tx_bytes += length1;
				tp->synopGMACNetStats.tx_packets++;
			}
			else {
				TR("Error in Status %08x\n",status);
				tp->synopGMACNetStats.tx_errors++;
				tp->synopGMACNetStats.tx_aborted_errors += synopGMAC_is_tx_aborted(status);
				tp->synopGMACNetStats.tx_carrier_errors += synopGMAC_is_tx_carrier_error(status);
			}
		}
		tp->synopGMACNetStats.collisions += synopGMAC_get_tx_collision_count(status);
	} while(desc_index >= 0);
}




/**
 * Function to Receive a packet from the interface.
 * After Receiving a packet, DMA transfers the received packet to the system memory
 * and generates corresponding interrupt (if it is enabled). This function prepares
 * the sk_buff for received packet after removing the ethernet CRC, and hands it over
 * to linux networking stack.
 * 	- Updataes the networking interface statistics
 *	- Keeps track of the rx descriptors
 * @param[in] pointer to net_device structure. 
 * \return void.
 * \note This function runs in interrupt context.
 */

int synop_handle_received_data(struct synopGMACNetworkAdapter* tp)
{
	synopGMACdevice *gmacdev;
	s32 desc_index;
	u32 data1;
	u32 data2;
	u32 len = 0;
	u32 status;
	u32 dma_addr1;
	u32 dma_addr2;
//	unsigned char *skb;

	gmacdev = tp->synopGMACdev;

	/*Handle the Receive Descriptors*/
//	do {
		flush_cache((unsigned long)gmacdev, sizeof(synopGMACdevice));
		desc_index = synopGMAC_get_rx_qptr(gmacdev, &status, &dma_addr1, NULL, &data1, &dma_addr2, NULL, &data2);
//		flush_cache((unsigned int)data1, RX_BUF_SIZE);
		if (desc_index >= 0 && data1 != 0) {
			if (synopGMAC_is_rx_desc_valid(status)) {
//				skb = (unsigned char *)plat_alloc_memory(RX_BUF_SIZE);

				dma_addr1 = plat_dma_map_single(data1, RX_BUF_SIZE);
//				len = synopGMAC_get_rx_desc_frame_length(status) - 4; //Not interested in Ethernet CRC bytes
				len = synopGMAC_get_rx_desc_frame_length(status);
//				memcpy((void *)skb, (void *)data1, len);

//				NetReceive(skb, len);
				NetReceive((unsigned char *)data1, len);
				tp->synopGMACNetStats.rx_packets++;
				tp->synopGMACNetStats.rx_bytes += len;
//				plat_free_memory((void *)skb);
			}
			else {
				printf("s: %08x\n",status);
				tp->synopGMACNetStats.rx_errors++;
				tp->synopGMACNetStats.collisions       += synopGMAC_is_rx_frame_collision(status);
				tp->synopGMACNetStats.rx_crc_errors    += synopGMAC_is_rx_crc(status);
				tp->synopGMACNetStats.rx_frame_errors  += synopGMAC_is_frame_dribbling_errors(status);
				tp->synopGMACNetStats.rx_length_errors += synopGMAC_is_rx_frame_length_errors(status);
			}
//			flush_cache((unsigned int)data1, RX_BUF_SIZE);
			desc_index = synopGMAC_set_rx_qptr(gmacdev,dma_addr1, RX_BUF_SIZE, (u32)data1,0,0,0);

			if (desc_index < 0) {
				plat_free_memory((void *)data1);
			}
		}
//	} while(desc_index >= 0);
	return len;
}


/**
 * Interrupt service routing.
 * This is the function registered as ISR for device interrupts.
 * @param[in] interrupt number. 
 * @param[in] void pointer to device unique structure (Required for shared interrupts in Linux).
 * @param[in] pointer to pt_regs (not used).
 * \return Returns IRQ_NONE if not device interrupts IRQ_HANDLED for device interrupts.
 * \note This function runs in interrupt context
 *
 */
int synopGMAC_intr_handler(struct synopGMACNetworkAdapter * tp)
{
	synopGMACdevice *gmacdev;
	u32 dma_status_reg;

	s32 status;
	u32 dma_addr;

	gmacdev = tp->synopGMACdev;

	rx_tx_ok = 0;

	/*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
	dma_status_reg = synopGMACReadReg(gmacdev->DmaBase, DmaStatus);
//	synopGMAC_disable_interrupt_all(gmacdev);

	/* TX/RX NORMAL interrupts正常中断 */
//	if (dma_status_reg & DmaIntNormal) {
		/* 接收中断或发送中断 */
		if ((dma_status_reg & DmaIntRxCompleted) ||
			 (dma_status_reg & (DmaIntTxCompleted))) {
				if(dma_status_reg & DmaIntRxCompleted){
//					synop_handle_received_data(tp);
//					rx_tx_ok = 1;
				}
				if(dma_status_reg & DmaIntTxCompleted){
					synop_handle_transmit_over(tp);	//Do whatever you want after the transmission is over
					rx_tx_ok = 1;
				}
		}
//	}
	
	/* ABNORMAL interrupts异常中断 */
	if (dma_status_reg & DmaIntAbnormal) {
		/*总线错误(异常)*/
		if(dma_status_reg & DmaIntBusError){
			u8 mac_addr[6] = DEFAULT_MAC_ADDRESS;//after soft reset, configure the MAC address to default value
			TR("%s::Fatal Bus Error Inetrrupt Seen\n",__FUNCTION__);
			printf("====DMA error!!!\n");

			synopGMAC_disable_dma_tx(gmacdev);
			synopGMAC_disable_dma_rx(gmacdev);

			synopGMAC_take_desc_ownership_tx(gmacdev);
			synopGMAC_take_desc_ownership_rx(gmacdev);

			synopGMAC_init_tx_rx_desc_queue(gmacdev);

			synopGMAC_reset(gmacdev);//reset the DMA engine and the GMAC ip

			synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, mac_addr); 
			synopGMAC_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip2 );
			synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward);	
			synopGMAC_init_rx_desc_base(gmacdev);
			synopGMAC_init_tx_desc_base(gmacdev);
			synopGMAC_mac_init(gmacdev);
			synopGMAC_enable_dma_rx(gmacdev);
			synopGMAC_enable_dma_tx(gmacdev);
		}

		/*接收缓存不可用(异常)*/
		if(dma_status_reg & DmaIntRxNoBuffer){
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				tp->synopGMACNetStats.rx_over_errors++;
				/*Now Descriptors have been created in synop_handle_received_data(). Just issue a poll demand to resume DMA operation*/
				synopGMACWriteReg(gmacdev->DmaBase, DmaStatus ,0x80); 	//sw: clear the rxb ua bit
				synopGMAC_resume_dma_rx(gmacdev);//To handle GBPS with 12 descriptors
			}
		}

		/*接收过程停止(异常)*/
		if(dma_status_reg & DmaIntRxStopped){
			TR("%s::Receiver stopped seeing Rx interrupts\n",__FUNCTION__); //Receiver gone in to stopped state
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				tp->synopGMACNetStats.rx_over_errors++;
				/*
				do{
					struct sk_buff *skb = alloc_skb(netdev->mtu + ETHERNET_HEADER + ETHERNET_CRC, GFP_ATOMIC);
					if(skb == NULL){
						TR("%s::ERROR in skb buffer allocation Better Luck Next time\n",__FUNCTION__);
						break;
						//return -ESYNOPGMACNOMEM;
					}
					dma_addr = pci_map_single(pcidev,skb->data,skb_tailroom(skb),PCI_DMA_FROMDEVICE);
					status = synopGMAC_set_rx_qptr(gmacdev,dma_addr, skb_tailroom(skb), (u32)skb,0,0,0);
					TR("%s::Set Rx Descriptor no %08x for skb %08x \n",__FUNCTION__,status,(u32)skb);
					if(status < 0)
					dev_kfree_skb_irq(skb);//changed from dev_free_skb. If problem check this again--manju
				}while(status >= 0);
				*/
				do {
					u32 skb = (u32)plat_alloc_memory(RX_BUF_SIZE);		//should skb aligned here?
					skb = (u32)(((unsigned int)skb & 0x0fffffff) | 0xa0000000);
					if (skb == 0) {
						printf("ERROR in skb buffer allocation\n");
						break;
						//return -ESYNOPGMACNOMEM;
					}

					dma_addr = plat_dma_map_single(skb, RX_BUF_SIZE);
					status = synopGMAC_set_rx_qptr(gmacdev,dma_addr,RX_BUF_SIZE, (u32)skb, 0, 0, 0);
					if (status < 0) {
						printf("==%s:no free\n",__FUNCTION__);
						plat_free_memory((void *)skb);
					}
				} while(status >= 0);

				synopGMAC_enable_dma_rx(gmacdev);
			}
		}

		/*传输缓存下溢(异常)*/
		if(dma_status_reg & DmaIntTxUnderflow){
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				synop_handle_transmit_over(tp);
			}
		}

		/*传输过程停止(异常)*/
		if(dma_status_reg & DmaIntTxStopped){
			TR("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
			if(GMAC_Power_down == 0){	// If Mac is not in powerdown
				synopGMAC_disable_dma_tx(gmacdev);
				synopGMAC_take_desc_ownership_tx(gmacdev);
				synopGMAC_enable_dma_tx(gmacdev);
				//netif_wake_queue(netdev);
				TR("%s::Transmission Resumed\n",__FUNCTION__);
			}
		}
	}
	
	/* Optional hardware blocks, interrupts should be disabled */
	if (dma_status_reg &
		     (GmacPmtIntr | GmacMmcIntr | GmacLineIntfIntr)) {
		TR("%s: unexpected status %08x\n", __func__, intr_status);
		if(dma_status_reg & GmacPmtIntr){
			synopGMAC_linux_powerup_mac(gmacdev);
		}
	
		if(dma_status_reg & GmacMmcIntr){
			TR("%s:: Interrupt due to MMC module\n",__FUNCTION__);
			TR("%s:: synopGMAC_rx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_rx_int_status(gmacdev));
			TR("%s:: synopGMAC_tx_int_status = %08x\n",__FUNCTION__,synopGMAC_read_mmc_tx_int_status(gmacdev));
		}

		/* thf PMON中执行synopGMAC_linux_cable_unplug_function()函数来判断网络是否有连接,
		   并获取连接信息,由于PMON使用轮询的方式来执行,所以占用很多资源，导致网络延时比较长,
		   目前暂时屏蔽该函数的执行来提高网络速度。屏蔽该函数没有发现会影响使用.
		*/
		if(dma_status_reg & GmacLineIntfIntr){
			/* 配置成千兆模式时GmacLineIntfIntr会被置1, 
			导致synopGMAC_linux_cable_unplug_function不断被执行,影响系统性能 
			GmacLineIntfIntr会被置1 还不清楚原因，这里暂时屏蔽该函数 */
			#ifdef CONFIG_PHY100M
			synopGMAC_linux_cable_unplug_function(tp);
			#endif
		}
	}

	/* Enable the interrrupt before returning from ISR*/
//	synopGMAC_clear_interrupt(gmacdev);
//	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
	return 0;
}



/**
 * Function used when the interface is opened for use.
 * We register synopGMAC_linux_open function to linux open(). Basically this 
 * function prepares the the device for operation . This function is called whenever ifconfig (in Linux)
 * activates the device (for example "ifconfig eth0 up"). This function registers
 * system resources needed 
 * 	- Attaches device to device specific structure
 * 	- Programs the MDC clock for PHY configuration
 * 	- Check and initialize the PHY interface 
 *	- ISR registration
 * 	- Setup and initialize Tx and Rx descriptors
 *	- Initialize MAC and DMA
 *	- Allocate Memory for RX descriptors (The should be DMAable)
 * 	- Initialize one second timer to detect cable plug/unplug
 *	- Configure and Enable Interrupts
 *	- Enable Tx and Rx
 *	- start the Linux network queue interface
 * @param[in] pointer to net_device structure. 
 * \return Returns 0 on success and error status upon failure.
 * \callgraph
 */
static s32 synopGMAC_linux_open(struct eth_device *dev)
{
	s32 status = 0;
	s32 retval = 0;

	u32 dma_addr;
	u32 skb;	//sw	we just use the name skb in pomn

	struct synopGMACNetworkAdapter *adapter = dev->priv;
	synopGMACdevice *gmacdev;

	gmacdev = (synopGMACdevice *)adapter->synopGMACdev;

	/*Now platform dependent initialization.*/
//	synopGMAC_disable_interrupt_all(gmacdev);

	/*Lets reset the IP*/
	synopGMAC_reset(gmacdev);

	/* we do not process interrupts */
	synopGMAC_disable_interrupt_all(gmacdev);

	/*Attach the device to MAC struct This will configure all the required base addresses
	  such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
	synopGMAC_set_mac_addr(gmacdev,GmacAddr0High,GmacAddr0Low, dev->enetaddr);

	/*Lets read the version of ip in to device structure*/
	synopGMAC_read_version(gmacdev);

	synopGMAC_get_mac_addr(adapter->synopGMACdev, GmacAddr0High, GmacAddr0Low, dev->enetaddr);
	
	/*Check for Phy initialization*/
	synopGMAC_set_mdc_clk_div(gmacdev, GmiiCsrClk2);	//thf
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

	/*Set up the tx and rx descriptor queue/ring*/
	synopGMAC_setup_tx_desc_queue(gmacdev, NULL, TRANSMIT_DESC_SIZE, RINGMODE);
	synopGMAC_init_tx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr
	
	synopGMAC_setup_rx_desc_queue(gmacdev, NULL, RECEIVE_DESC_SIZE, RINGMODE);
	synopGMAC_init_rx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

#ifdef ENH_DESC_8W
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip2 | DmaDescriptor8Words); //pbl32 incr with rxthreshold 128 and Desc is 8 Words
#else
#if defined(LS1CSOC)
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength4 | DmaDescriptorSkip2);
#else
	synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength4 | DmaDescriptorSkip1);	//pbl4 incr with rxthreshold 128
#endif
#endif
	synopGMAC_dma_control_init(gmacdev, DmaStoreAndForward|DmaTxSecondFrame|DmaRxThreshCtrl128);

	/*Initialize the mac interface*/
	synopGMAC_check_phy_init(adapter);
	synopGMAC_mac_init(gmacdev);
	synopGMAC_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

	do {
		skb = (u32)plat_alloc_memory(RX_BUF_SIZE);		//should skb aligned here?
		skb = (u32)(((unsigned int)skb & 0x0fffffff) | 0xa0000000);
		if (skb == 0) {
			printf("ERROR in skb buffer allocation\n");
			break;
//			return -ESYNOPGMACNOMEM;
		}

		dma_addr = plat_dma_map_single(skb, RX_BUF_SIZE);

		status = synopGMAC_set_rx_qptr(gmacdev, dma_addr, RX_BUF_SIZE, (u32)skb, 0, 0, 0);
		if (status < 0) {
			plat_free_memory((void *)skb);
		}
	} while(status >= 0 && status < RECEIVE_DESC_SIZE-1);

	synopGMAC_clear_interrupt(gmacdev);
	/*
	Disable the interrupts generated by MMC and IPC counters.
	If these are not disabled ISR should be modified accordingly to handle these interrupts.
	*/	
	synopGMAC_disable_mmc_tx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_rx_interrupt(gmacdev, 0xFFFFFFFF);
	synopGMAC_disable_mmc_ipc_rx_interrupt(gmacdev, 0xFFFFFFFF);

	/* no interrupts in pmon */
//	synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);

	synopGMAC_enable_dma_rx(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);

#if defined(LS1ASOC) || defined(LS1BSOC) || defined(LS1CSOC)
	synopGMAC_mac_init(gmacdev);
#endif

//	PInetdev->sc_ih = pci_intr_establish(0, 0, IPL_NET, synopGMAC_intr_handler, adapter, 0);
	TR("register poll interrupt: gmac 0\n");

	return retval;
}

/**
 * Function to transmit a given packet on the wire.
 * Whenever Linux Kernel has a packet ready to be transmitted, this function is called.
 * The function prepares a packet and prepares the descriptor and 
 * enables/resumes the transmission.
 * @param[in] pointer to sk_buff structure. 
 * @param[in] pointer to net_device structure.
 * \return Returns 0 on success and Error code on failure. 
 * \note structure sk_buff is used to hold packet in Linux networking stacks.
 */
s32 synopGMAC_linux_xmit_frames(struct eth_device *dev, void *packet, int length)
{
	s32 status = 0;
	u32 dma_addr;
	u32 offload_needed = 0;
	u32 skb;
	int len;
	struct synopGMACNetworkAdapter *adapter;
	synopGMACdevice *gmacdev;

	adapter = (struct synopGMACNetworkAdapter *)dev->priv;

	gmacdev = (synopGMACdevice *)adapter->synopGMACdev;

//	while (ifp->if_snd.ifq_head != NULL) {
		if (!synopGMAC_is_desc_owned_by_dma(gmacdev->TxNextDesc)) {

			skb = (u32)plat_alloc_memory(TX_BUF_SIZE);
			skb = (u32)(((unsigned int)skb & 0x0fffffff) | 0xa0000000);
			if(skb == 0)
				return -1;

			len = length;

			memcpy((void *)skb, packet, len);
			dma_addr = plat_dma_map_single(skb, len);

			status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, len, skb, 0, 0, 0, offload_needed);

			if(status < 0){
				TR("%s No More Free Tx Descriptors\n",__FUNCTION__);
				return -1;
			}
		}
#if SYNOP_TX_DEBUG
		else
			printf("===%x: next txDesc belongs to DMA don't set it\n",gmacdev->TxNextDesc);
#endif
//	}
	
	/*Now force the DMA to start transmission*/
	synopGMAC_resume_dma_tx(gmacdev);
	return 0;
}

static int init_phy(synopGMACdevice *gmacdev)
{
	u16 data, data1;

	synopGMAC_read_phy_reg(gmacdev->MacBase, gmacdev->PhyBase, 2, &data);
	synopGMAC_read_phy_reg(gmacdev->MacBase, gmacdev->PhyBase, 3, &data1);
//	printf("PHY ID %x %x\n", data, data1);
#ifdef RMII
	/* RTL8201EL */
	if ((data == 0x001c) && (data1 == 0xc815)) {
		/*  设置寄存器25，使能RMII模式 */
		synopGMAC_write_phy_reg(gmacdev->MacBase, gmacdev->PhyBase, 25, 0x400);
	}
#endif
	/*set 88e1111 clock phase delay*/
	if (data == 0x141)
		rtl88e1111_config_init(gmacdev);
	return 0;
}

static void gmac_halt(struct eth_device *dev)
{
}

static int gmac_init(struct eth_device *dev, bd_t * bd)
{
	return 0;
}

static int gmac_recv(struct eth_device *dev)
{
	struct synopGMACNetworkAdapter *adapter = gmac_adapter;

/*	while (1) {
		synopGMAC_intr_handler(adapter);
		if (rx_tx_ok) {
			break;
		}
	}
	return 0;*/
	return synop_handle_received_data(adapter);
}

static int gmac_send(struct eth_device *dev, void *packet, int length)
{
	struct synopGMACNetworkAdapter *adapter = gmac_adapter;

	synopGMAC_linux_xmit_frames(dev, packet, length);

	while (1) {
		synopGMAC_intr_handler(adapter);
		if (rx_tx_ok) {
			break;
		}
	}
	return 0;
}

static int gethex(u8 *vp, char *p, int n)
{
	u8 v;
	int digit;

	for (v = 0; n > 0; n--) {
		if (*p == 0)
			return (0);
		if (*p >= '0' && *p <= '9')
			digit = *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			digit = *p - 'a' + 10;
		else if (*p >= 'A' && *p <= 'F')
			digit = *p - 'A' + 10;
		else
			return (0);

		v <<= 4;
		v |= digit;
		p++;
	}
	*vp = v;
	return (1);
}

/**
 * Function to initialize the Linux network interface.
 * 
 * Linux dependent Network interface is setup here. This provides 
 * an example to handle the network dependent functionality.
 *
 * \return Returns 0 on success and Error code on failure.
 */
s32  synopGMAC_init_network_interface(char *xname, unsigned int synopGMACMappedAddr)
{
	static u8 mac_addr0[6] = DEFAULT_MAC_ADDRESS;
	static int inited = 0;
	int i, ret;
	struct synopGMACNetworkAdapter *synopGMACadapter;

	if (!inited) {
		u8 v;
		char *s = getenv("ethaddr");
		if (s) {
			int allz, allf;
			u8 macaddr[6];

			for (i=0, allz=1, allf=1; i<6; i++) {
				gethex(&v, s, 2);
				macaddr[i] = (u8)v;
				s += 3;         /* Don't get to fancy here :-) */
				if(v != 0) allz = 0;
				if(v != 0xff) allf = 0;
			}
			if (!allz && !allf)
				memcpy(mac_addr0, macaddr, 6);
		}
		inited = 1;
	}
#if defined(LS1ASOC)
	*((volatile unsigned int*)0xbfd00420) &= ~0x00800000;	/* 使能GMAC0 */
	#ifdef CONFIG_GMAC0_100M
	*((volatile unsigned int*)0xbfd00420) |= 0x500;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00420) &= ~0x500;		/* 否则配置成千兆模式 */
	#endif
	if (synopGMACMappedAddr == 0xbfe20000) {
		*((volatile unsigned int*)0xbfd00420) &= ~0x01000000;	/* 使能GMAC1 */
		#ifdef CONFIG_GMAC1_100M
		*((volatile unsigned int*)0xbfd00420) |= 0xa00;		/* 配置成百兆模式 */
		#else
		*((volatile unsigned int*)0xbfd00420) &= ~0xa00;		/* 否则配置成千兆模式 */
		#endif
		#ifdef GMAC1_USE_UART01
		*((volatile unsigned int*)0xbfd00420) |= 0xc0;
		#else
		*((volatile unsigned int*)0xbfd00420) &= ~0xc0;
		#endif
	}
#elif defined(LS1BSOC)
	/* 寄存器0xbfd00424有GMAC的使能开关 */
	*((volatile unsigned int*)0xbfd00424) &= ~0x1000;	/* 使能GMAC0 */
	#ifdef CONFIG_GMAC0_100M
	*((volatile unsigned int*)0xbfd00424) |= 0x5;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00424) &= ~0x5;	/* 否则配置成千兆模式 */
	#endif
	/* GMAC1初始化 使能GMAC1 和UART0复用，导致UART0不能使用 */
	if (synopGMACMappedAddr == 0xbfe20000) {
		*((volatile unsigned int*)0xbfd00420) |= 0x18;
		*((volatile unsigned int*)0xbfd00424) &= ~0x2000;	/* 使能GMAC1 */
		#ifdef CONFIG_GMAC1_100M
		*((volatile unsigned int*)0xbfd00424) |= 0xa;		/* 配置成百兆模式 */
		#else
		*((volatile unsigned int*)0xbfd00424) &= ~0xa;	/* 否则配置成千兆模式 */
		#endif
	}
#elif defined(LS1CSOC)
	*((volatile unsigned int *)0xbfd00424) &= ~(7 << 28);
#ifdef RMII
    *((volatile unsigned int *)0xbfd00424) |= (1 << 30); //wl rmii
#endif
/*    *((volatile unsigned int *)0xbfd011c0) &= 0x000fffff; //gpio[37:21] used as mac
    *((volatile unsigned int *)0xbfd011c4) &= 0xffffffc0;
    *((volatile unsigned int *)0xbfd011d0) &= 0x000fffff;
    *((volatile unsigned int *)0xbfd011d4) &= 0xffffffc6;
    *((volatile unsigned int *)0xbfd011e0) &= 0x000fffff;
    *((volatile unsigned int *)0xbfd011e4) &= 0xffffffc0;
    *((volatile unsigned int *)0xbfd011f0) &= 0x000fffff;
    *((volatile unsigned int *)0xbfd011f4) &= 0xffffffc0;*/
#endif
	
	TR("Now Going to Call register_netdev to register the network interface for GMAC core\n");

	synopGMACadapter = (struct synopGMACNetworkAdapter * )plat_alloc_memory(sizeof(struct synopGMACNetworkAdapter)); 
	memset((char *)synopGMACadapter, 0, sizeof(struct synopGMACNetworkAdapter));
	gmac_adapter = synopGMACadapter;
	synopGMACadapter->synopGMACdev = NULL;

	/*Allocate Memory for the the GMACip structure*/
	synopGMACadapter->synopGMACdev = (synopGMACdevice *)plat_alloc_memory(sizeof(synopGMACdevice));
	memset((char *)synopGMACadapter->synopGMACdev, 0, sizeof(synopGMACdevice));
	if(!synopGMACadapter->synopGMACdev) {
		TR0("Error in Memory Allocataion \n");
	}

	ret = synopGMAC_attach(synopGMACadapter->synopGMACdev, (u32)synopGMACMappedAddr + MACBASE, (u32)synopGMACMappedAddr + DMABASE, DEFAULT_PHY_BASE, mac_addr0);
	if (ret) {
		return -1;
	}
	init_phy(synopGMACadapter->synopGMACdev);

	{
	struct eth_device *dev;
	dev = (struct eth_device *)malloc(sizeof(*dev));
	if (!dev) {
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(*dev));

	dev->iobase = synopGMACMappedAddr;
	dev->priv = synopGMACadapter;

	sprintf(dev->name, xname);
	dev->enetaddr[0] = mac_addr0[0];
	dev->enetaddr[1] = mac_addr0[1];
	dev->enetaddr[2] = mac_addr0[2];
	dev->enetaddr[3] = mac_addr0[3];
	dev->enetaddr[4] = mac_addr0[4];
	dev->enetaddr[5] = mac_addr0[5];

	synopGMAC_linux_open(dev);

	dev->init = gmac_init;
	dev->halt = gmac_halt;
	dev->send = gmac_send;
	dev->recv = gmac_recv;

	eth_register(dev);
	}

	return 1;
}

