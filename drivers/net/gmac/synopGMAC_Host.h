#ifndef SYNOP_GMAC_HOST_H
#define SYNOP_GMAC_HOST_H 1

#include "synopGMAC_plat.h"
#include "synopGMAC_Dev.h"

//#define ENH_DESC
//#define ENH_DESC_8W

struct net_device_stats
{
	unsigned int	rx_packets;		/* total packets received	*/
	unsigned int	tx_packets;		/* total packets transmitted	*/
	unsigned int	rx_bytes;		/* total bytes received 	*/
	unsigned int	tx_bytes;		/* total bytes transmitted	*/
	unsigned int	rx_errors;		/* bad packets received		*/
	unsigned int	tx_errors;		/* packet transmit problems	*/
	unsigned int	rx_dropped;		/* no space in linux buffers	*/
	unsigned int	tx_dropped;		/* no space available in linux	*/
	unsigned int	multicast;		/* multicast packets received	*/
	unsigned int	collisions;

	/* detailed rx_errors: */
	unsigned int	rx_length_errors;
	unsigned int	rx_over_errors;		/* receiver ring buff overflow	*/
	unsigned int	rx_crc_errors;		/* recved pkt with crc error	*/
	unsigned int	rx_frame_errors;	/* recv'd frame alignment error */
	unsigned int	rx_fifo_errors;		/* recv'r fifo overrun		*/
	unsigned int	rx_missed_errors;	/* receiver missed packet	*/

	/* detailed tx_errors */
	unsigned int	tx_aborted_errors;
	unsigned int	tx_carrier_errors;
	unsigned int	tx_fifo_errors;
	unsigned int	tx_heartbeat_errors;
	unsigned int	tx_window_errors;
	
	/* for cslip etc */
	unsigned int	rx_compressed;
	unsigned int	tx_compressed;
};

typedef struct synopGMACNetworkAdapter {
	synopGMACdevice *synopGMACdev;
	struct net_device_stats synopGMACNetStats;
} synopGMACPciNetworkAdapter;

#endif
