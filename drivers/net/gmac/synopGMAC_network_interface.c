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

#include <miiphy.h>
#include <linux/compiler.h>
#include "designware.h"

#include "synopGMAC_plat.h"
#include "synopGMAC_Dev.h"

static synopGMACdevice *synopGMACdev;

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
static s32 synopGMAC_setup_tx_desc_queue(synopGMACdevice *gmacdev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->TxDescCount = 0;

	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc) * no_of_desc, &dma_addr);
	if (first_desc == NULL) {
		printf("Error in Tx Descriptors memory allocation\n");
		return -ESYNOPGMACNOMEM;
	}

	gmacdev->TxDescCount = no_of_desc;
	gmacdev->TxDesc      = first_desc;
	gmacdev->TxDescDma   = dma_addr;

	for (i=0; i<gmacdev->TxDescCount; i++) {
		synopGMAC_tx_desc_init_ring(gmacdev->TxDesc + i, i == gmacdev->TxDescCount-1);
	}

	gmacdev->TxNext = 0;
	gmacdev->TxBusy = 0;
	gmacdev->TxNextDesc = gmacdev->TxDesc;
	gmacdev->TxBusyDesc = gmacdev->TxDesc;
	gmacdev->BusyTxDesc = 0; 

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
static s32 synopGMAC_setup_rx_desc_queue(synopGMACdevice *gmacdev, u32 no_of_desc, u32 desc_mode)
{
	s32 i;
	DmaDesc *first_desc = NULL;
	dma_addr_t dma_addr;
	gmacdev->RxDescCount = 0;

	first_desc = (DmaDesc *)plat_alloc_consistent_dmaable_memory(sizeof(DmaDesc) * no_of_desc, &dma_addr);
	if (first_desc == NULL) {
		printf("Error in Rx Descriptor Memory allocation in Ring mode\n");
		return -ESYNOPGMACNOMEM;
	}

	gmacdev->RxDescCount = no_of_desc;
	gmacdev->RxDesc      = first_desc;
	gmacdev->RxDescDma   = dma_addr;

	for (i=0; i<gmacdev->RxDescCount; i++) {
		synopGMAC_rx_desc_init_ring(gmacdev->RxDesc + i, i == gmacdev->RxDescCount-1);
	}

	gmacdev->RxNext = 0;
	gmacdev->RxBusy = 0;
	gmacdev->RxNextDesc = gmacdev->RxDesc;
	gmacdev->RxBusyDesc = gmacdev->RxDesc;
	gmacdev->BusyRxDesc = 0; 

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
static void synop_handle_transmit_over(synopGMACdevice *gmacdev)
{
	s32 desc_index;
	u32 data1, data2;
	u32 length1, length2;
	u32 dma_addr1, dma_addr2;
	u32 status;

	/*Handle the transmit Descriptors*/
	do {
		desc_index = synopGMAC_get_tx_qptr(gmacdev, &status, &dma_addr1, &length1, &data1, &dma_addr2, &length2, &data2);
		if (desc_index >= 0 && data1 != 0) {
			plat_free_memory((void *)((data1 & 0x0fffffff) | 0x80000000));
			
			if (synopGMAC_is_desc_valid(status)) {

			}
			else {
				printf("Error in Status %08x\n",status);
			}
		}
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

static int synop_handle_received_data(synopGMACdevice *gmacdev)
{
	s32 desc_index;
	u32 data1;
	u32 data2;
	u32 len = 0;
	u32 status;
	u32 dma_addr1;
	u32 dma_addr2;
//	unsigned char *skb;

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
//				plat_free_memory((void *)skb);
			}
			else {
				printf("s: %08x\n",status);
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
	u32 skb;
	synopGMACdevice *gmacdev = synopGMACdev;

	/*Lets reset the IP*/
	synopGMAC_reset(gmacdev);

	/* we do not process interrupts */
	synopGMAC_disable_interrupt_all(gmacdev);

	/*Set up the tx and rx descriptor queue/ring*/
	synopGMAC_setup_tx_desc_queue(gmacdev, TRANSMIT_DESC_SIZE, RINGMODE);
	synopGMAC_init_tx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr
	
	synopGMAC_setup_rx_desc_queue(gmacdev, RECEIVE_DESC_SIZE, RINGMODE);
	synopGMAC_init_rx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

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
static s32 synopGMAC_linux_xmit_frames(struct eth_device *dev, void *packet, int length)
{
	s32 status = 0;
	u32 dma_addr;
	u32 offload_needed = 0;
	u32 skb;
	int len;
	synopGMACdevice *gmacdev = synopGMACdev;

	if (!synopGMAC_is_desc_owned_by_dma(gmacdev->TxNextDesc)) {
		skb = (u32)plat_alloc_memory(TX_BUF_SIZE);
		skb = (u32)(((unsigned int)skb & 0x0fffffff) | 0xa0000000);
		if (skb == 0)
			return -1;

		len = length;

		memcpy((void *)skb, packet, len);
		dma_addr = plat_dma_map_single(skb, len);

		status = synopGMAC_set_tx_qptr(gmacdev, dma_addr, len, skb, 0, 0, 0, offload_needed);
		if (status < 0) {
			printf("%s No More Free Tx Descriptors\n",__FUNCTION__);
			return -1;
		}
	}
	else
		printf("===%x: next txDesc belongs to DMA don't set it\n", gmacdev->TxNextDesc);

	/*Now force the DMA to start transmission*/
	synopGMAC_resume_dma_tx(gmacdev);
	return 0;
}

static int gmac_recv(struct eth_device *dev)
{
	return synop_handle_received_data(synopGMACdev);
}

static int gmac_send(struct eth_device *dev, void *packet, int length)
{
	synopGMACdevice *gmacdev = synopGMACdev;
	u32 dma_status_reg;

	synopGMAC_linux_xmit_frames(dev, packet, length);

	while (1) {
		dma_status_reg = synopGMACReadReg(gmacdev->DmaBase, DmaStatus);
		if (dma_status_reg & DmaIntTxCompleted) {
			synop_handle_transmit_over(gmacdev);
			break;
		}
	}
	return 0;
}

static int mac_reset(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;

	ulong start;
	int timeout = CONFIG_MACRESET_TIMEOUT;

	writel(DMAMAC_SRST, &dma_p->busmode);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		if (!(readl(&dma_p->busmode) & DMAMAC_SRST))
			return 0;

		/* Try again after 10usec */
		udelay(10);
	};

	return -1;
}

static int dw_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct eth_mac_regs *mac_p = bus->priv;
	ulong start;
	u16 miiaddr;
	int timeout = CONFIG_MDIO_TIMEOUT;

	miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) |
		  ((reg << MIIREGSHIFT) & MII_REGMSK);

	writel(miiaddr | MII_CLKRANGE_20_35M | MII_BUSY, &mac_p->miiaddr);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		if (!(readl(&mac_p->miiaddr) & MII_BUSY))
			return readl(&mac_p->miidata);
		udelay(10);
	};

	return -1;
}

static int dw_mdio_write(struct mii_dev *bus, int addr, int devad, int reg,
			u16 val)
{
	struct eth_mac_regs *mac_p = bus->priv;
	ulong start;
	u16 miiaddr;
	int ret = -1, timeout = CONFIG_MDIO_TIMEOUT;

	writel(val, &mac_p->miidata);
	miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) |
		  ((reg << MIIREGSHIFT) & MII_REGMSK) | MII_WRITE;

	writel(miiaddr | MII_CLKRANGE_20_35M | MII_BUSY, &mac_p->miiaddr);

	start = get_timer(0);
	while (get_timer(start) < timeout) {
		if (!(readl(&mac_p->miiaddr) & MII_BUSY)) {
			ret = 0;
			break;
		}
		udelay(10);
	};

	return ret;
}

static int dw_mdio_init(char *name, struct eth_mac_regs *mac_regs_p)
{
	struct mii_dev *bus = mdio_alloc();

	if (!bus) {
		printf("Failed to allocate MDIO bus\n");
		return -1;
	}

	bus->read = dw_mdio_read;
	bus->write = dw_mdio_write;
	sprintf(bus->name, name);

	bus->priv = (void *)mac_regs_p;

	return mdio_register(bus);
}

static int dw_write_hwaddr(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	u32 macid_lo, macid_hi;
	u8 *mac_id = &dev->enetaddr[0];

	macid_lo = mac_id[0] + (mac_id[1] << 8) + (mac_id[2] << 16) +
		   (mac_id[3] << 24);
	macid_hi = mac_id[4] + (mac_id[5] << 8);

	writel(macid_hi, &mac_p->macaddr0hi);
	writel(macid_lo, &mac_p->macaddr0lo);

	return 0;
}

static void dw_adjust_link(struct eth_mac_regs *mac_p,
			   struct phy_device *phydev)
{
	u32 conf = readl(&mac_p->conf) | FRAMEBURSTENABLE | DISABLERXOWN;

	if (!phydev->link) {
		printf("%s: No link.\n", phydev->dev->name);
		return;
	}

	if (phydev->speed != 1000)
		conf |= MII_PORTSELECT;

	if (phydev->speed == 100)
		conf |= FES_100;

	if (phydev->duplex)
		conf |= FULLDPLXMODE;

	writel(conf, &mac_p->conf);

	printf("Speed: %d, %s duplex%s\n", phydev->speed,
	       (phydev->duplex) ? "full" : "half",
	       (phydev->port == PORT_FIBRE) ? ", fiber mode" : "");
}

static int dw_phy_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct phy_device *phydev;
	int mask = 0xffffffff;

#ifdef CONFIG_PHY_ADDR
	mask = 1 << CONFIG_PHY_ADDR;
#endif

	phydev = phy_find_by_mask(priv->bus, mask, priv->interface);
	if (!phydev)
		return -1;

	phy_connect_dev(phydev, dev);

	phydev->supported &= PHY_GBIT_FEATURES;
	phydev->advertising = phydev->supported;

	priv->phydev = phydev;
	phy_config(phydev);

	return 1;
}

static void gmac_halt(struct eth_device *dev)
{
/*	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;

	writel(readl(&mac_p->conf) & ~(RXENABLE | TXENABLE), &mac_p->conf);
	writel(readl(&dma_p->opmode) & ~(RXSTART | TXSTART), &dma_p->opmode);

	phy_shutdown(priv->phydev);*/
}

static int gmac_init(struct eth_device *dev, bd_t * bd)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	u32 conf;

	/* Resore the HW MAC address as it has been lost during MAC reset */
	dw_write_hwaddr(dev);

#ifdef CONFIG_CPU_LOONGSON1C
	writel(/*FIXEDBURST | PRIORXTX_41*/(0x2 << 2) | DMA_PBL,
			&dma_p->busmode);
#else
	writel(/*FIXEDBURST | PRIORXTX_41*/(0x1 << 2) | DMA_PBL,
			&dma_p->busmode);
#endif

	writel(FLUSHTXFIFO | STOREFORWARD | TXSECONDFRAME, &dma_p->opmode);

	writel(readl(&dma_p->opmode) | RXSTART | TXSTART, &dma_p->opmode);

	/* Start up the PHY */
	if (phy_startup(priv->phydev)) {
		printf("Could not initialize PHY %s\n",
		       priv->phydev->dev->name);
		return -1;
	}

	dw_adjust_link(mac_p, priv->phydev);

	if (!priv->phydev->link)
		return -1;

	writel(readl(&mac_p->conf) | RXENABLE | TXENABLE, &mac_p->conf);

	return 0;
}

int synopGMAC_initialize(ulong base_addr, u32 interface)
{
	struct eth_device *dev;
	struct dw_eth_dev *priv;

#if defined(CONFIG_CPU_LOONGSON1A)
	*((volatile unsigned int*)0xbfd00420) &= ~0x00800000;	/* 使能GMAC0 */
	#ifdef CONFIG_LS1X_GMAC0_100M
	*((volatile unsigned int*)0xbfd00420) |= 0x500;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00420) &= ~0x500;		/* 否则配置成千兆模式 */
	#endif
	if (base_addr == 0xbfe20000) {
		*((volatile unsigned int*)0xbfd00420) &= ~0x01000000;	/* 使能GMAC1 */
		#ifdef CONFIG_LS1X_GMAC1_100M
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
#elif defined(CONFIG_CPU_LOONGSON1B)
	/* 寄存器0xbfd00424有GMAC的使能开关 */
	*((volatile unsigned int*)0xbfd00424) &= ~0x1000;	/* 使能GMAC0 */
	#ifdef CONFIG_LS1X_GMAC0_100M
	*((volatile unsigned int*)0xbfd00424) |= 0x5;		/* 配置成百兆模式 */
	#else
	*((volatile unsigned int*)0xbfd00424) &= ~0x5;	/* 否则配置成千兆模式 */
	#endif
	/* GMAC1初始化 使能GMAC1 和UART0复用，导致UART0不能使用 */
	if (base_addr == 0xbfe20000) {
		*((volatile unsigned int*)0xbfd00420) |= 0x18;
		*((volatile unsigned int*)0xbfd00424) &= ~0x2000;	/* 使能GMAC1 */
		#ifdef CONFIG_LS1X_GMAC1_100M
		*((volatile unsigned int*)0xbfd00424) |= 0xa;		/* 配置成百兆模式 */
		#else
		*((volatile unsigned int*)0xbfd00424) &= ~0xa;	/* 否则配置成千兆模式 */
		#endif
	}
#elif defined(CONFIG_CPU_LOONGSON1C)
	*((volatile unsigned int *)0xbfd00424) &= ~(7 << 28);
#ifdef CONFIG_LS1X_GMAC_RMII
    *((volatile unsigned int *)0xbfd00424) |= (1 << 30); //wl rmii
#endif
#endif

	/*Allocate Memory for the the GMACip structure*/
	synopGMACdev = (synopGMACdevice *)plat_alloc_memory(sizeof(synopGMACdevice));
	memset((char *)synopGMACdev, 0, sizeof(synopGMACdevice));
	if(!synopGMACdev) {
		printf("Error in Memory Allocataion \n");
		return -ENOMEM;
	}
	synopGMACdev->MacBase = (u32)base_addr + MACBASE;
	synopGMACdev->DmaBase = (u32)base_addr + DMABASE;

	dev = (struct eth_device *)malloc(sizeof(*dev));
	if (!dev) {
		return -ENOMEM;
	}

	/*
	 * Since the priv structure contains the descriptors which need a strict
	 * buswidth alignment, memalign is used to allocate memory
	 */
	priv = (struct dw_eth_dev *) memalign(ARCH_DMA_MINALIGN, sizeof(struct dw_eth_dev));
	if (!priv) {
		free(dev);
		return -ENOMEM;
	}

	memset(dev, 0, sizeof(struct eth_device));
	memset(priv, 0, sizeof(struct dw_eth_dev));

	sprintf(dev->name, "dwmac.%lx", base_addr);
	dev->iobase = (int)base_addr;
	dev->priv = priv;
//	dev->priv = synopGMACdev;

	priv->dev = dev;
	priv->mac_regs_p = (struct eth_mac_regs *)base_addr;
	priv->dma_regs_p = (struct eth_dma_regs *)(base_addr +
			DW_DMA_BASE_OFFSET);

	dev->init = gmac_init;
	dev->halt = gmac_halt;
	dev->send = gmac_send;
	dev->recv = gmac_recv;
	dev->write_hwaddr = dw_write_hwaddr;

	eth_register(dev);

	priv->interface = interface;

	dw_mdio_init(dev->name, priv->mac_regs_p);
	priv->bus = miiphy_get_dev_by_name(dev->name);

	dw_phy_init(dev);

//	mac_reset(dev);
	synopGMAC_linux_open(dev);

	return 1;
}

