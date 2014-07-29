/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#ifndef __MMC_LS1X_P_H__
#define __MMC_LS1X_P_H__

//#define SDICON 0x0
# define SDICON_SDRESET (1 << 8)
# define SDICON_CLKEN (1 << 0) /* enable/disable external clock */

//#define SDIPRE 0x4

//#define SDICMDARG 0x8

//#define SDICMDCON 0xc
# define SDICMDCON_CRCCHECK (1 << 13)
# define SDICMDCON_ABORT (1 << 12)
# define SDICMDCON_WITHDATA (1 << 11)
# define SDICMDCON_LONGRSP (1 << 10)
# define SDICMDCON_WAITRSP (1 << 9)
# define SDICMDCON_CMDSTART (1 << 8)
# define SDICMDCON_SENDERHOST (1 << 6)
# define SDICMDCON_INDEX (0x3f)

//#define SDICMDSTAT 0x10
# define SDICMDSTAT_CRCFAIL (1 << 12)
# define SDICMDSTAT_CMDSENT (1 << 11)
# define SDICMDSTAT_CMDTIMEOUT (1 << 10)
# define SDICMDSTAT_RSPFIN (1 << 9)
# define SDICMDSTAT_XFERING (1 << 8)
# define SDICMDSTAT_INDEX (0xff)

//#define SDIRSP0 0x14
//#define SDIRSP1 0x18
//#define SDIRSP2 0x1C
//#define SDIRSP3 0x20

//#define SDITIMER 0x24
//#define SDIBSIZE 0x28

//#define SDIDCON 0x2c
# define SDIDCON_WIDEBUS (1 << 16)
# define SDIDCON_DMAEN (1 << 15)
# define SDIDCON_STOP (0 << 14)
# define SDIDCON_DATSTART (1 << 14)
# define SDIDCON_BLKNUM (0xfff)

//#define SDIDCNT 0x30
# define SDIDCNT_BLKNUM_SHIFT 12

//#define SDIDSTA 0x34
# define SDIDSTA_RDYWAITREQ (1 << 9)
# define SDIDSTA_SDIOIRQDETECT (1 << 8)
# define SDIDSTA_CRCFAIL (1 << 7)
# define SDIDSTA_RXCRCFAIL (1 << 6)
# define SDIDSTA_DATATIMEOUT (1 << 5)
# define SDIDSTA_XFERFINISH (1 << 4)
# define SDIDSTA_BUSYFINISH (1 << 3)
# define SDIDSTA_SBITERR (1 << 2)
# define SDIDSTA_TXDATAON (1 << 1)
# define SDIDSTA_RXDATAON (1 << 0)

//#define SDIFSTA 0x38
# define SDIFSTA_TFHALF (1<<11)
# define SDIFSTA_TFEMPTY (1<<10)
# define SDIFSTA_RFFULL (1<<8)
# define SDIFSTA_RFEMPTY (1<<7)

//#define SDIIMSK 0x3C
//#define SDIDATA 0x40
//#define SDIINTEN 0x64

#endif /* __MMC_LS1X_P_H__ */
