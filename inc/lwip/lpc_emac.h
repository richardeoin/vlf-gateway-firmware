/**********************************************************************
 * $Id$		lpc_emac.h			2011-11-20
 *//**
    * @file		lpc_emac.h
    * @brief	LPC ethernet driver header file for LWIP
    * @version	1.0
    * @date		20. Nov. 2011
    * @author	NXP MCU SW Application Team
    * 
    * Copyright(C) 2011, NXP Semiconductor
    * All rights reserved.
    *
    ***********************************************************************
    * Software that is described herein is for illustrative purposes only
    * which provides customers with programming information regarding the
    * products. This software is supplied "AS IS" without any warranties.
    * NXP Semiconductors assumes no responsibility or liability for the
    * use of the software, conveys no license or title under any patent,
    * copyright, or mask work right to the product. NXP Semiconductors
    * reserves the right to make changes in the software without
    * notification. NXP Semiconductors also make no representation or
    * warranty that such application will be suitable for the specified
    * use without further testing or modification.
    **********************************************************************/

#ifndef __LPC_EMAC_H
#define __LPC_EMAC_H

#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lpc17xx_emac.h"
#include "lpc_emac_config.h"
#include "lpc_phy.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Ethernet power/clock control bit in PCONP register */
#define PCENET			0x40000000
/* Ethernet configuration for PINSEL2, as per User Guide §5.3 */
#define ENET_PINSEL2_CONFIG	0x50150105
/* Ethernet configuration for PINSEL3, as per User Guide §5.4 */
#define ENET_PINSEL3_CONFIG	0x00000005
/* Only bottom byte of PINSEL3 relevant to Ethernet */
#define ENET_PINSEL3_MASK	0x0000000F

/**
 * Structure of a TX/RX descriptor
 */
typedef struct
{
  volatile u32_t packet;			/* Pointer to buffer */
  volatile u32_t control;			/* Control word */
} LPC_TXRX_DESC_T;
  
/**
 * Structure of a RX status entry
 */
typedef struct
{
  volatile u32_t statusinfo;			/* RX status word */
  volatile u32_t statushashcrc;			/* RX hash CRC */
} LPC_TXRX_STATUS_T;

/**
 * LPC EMAC driver data structure
 */
//__attribute__ ((nocommon, section (".lwip_ram")))
struct lpc_enetdata {
  struct netif *netif;				/* Reference back to LWIP parent netif */

  LPC_TXRX_DESC_T ptxd[LPC_NUM_BUFF_TXDESCS];	/* Pointer to TX descriptor list */
  LPC_TXRX_STATUS_T ptxs[LPC_NUM_BUFF_TXDESCS];	/* Pointer to TX statuses */
  LPC_TXRX_DESC_T prxd[LPC_NUM_BUFF_RXDESCS];	/* Pointer to RX descriptor list */
  __attribute__ ((aligned (8)))			/* See User Manual §10.15.1 */
  LPC_TXRX_STATUS_T prxs[LPC_NUM_BUFF_RXDESCS];	/* Pointer to RX statuses */

#if LPC_PBUF_RX_ZEROCOPY
  struct pbuf *rxb[LPC_NUM_BUFF_RXDESCS];	/* RX pbuf pointer list, zero-copy mode */
  u32_t rx_fill_desc_index;			/* RX descriptor next available index */
  u32_t rx_free_descs;				/* Count of free RX descriptors */
#else
  /* Array of contiguous RX buffers used with copied pbufs */
  u32_t lpc_rx_buffs[LPC_NUM_BUFF_RXDESCS][1 + (EMAC_ETH_MAX_FLEN / 4)];
#endif

#if LPC_PBUF_TX_ZEROCOPY
  struct pbuf *txb[LPC_NUM_BUFF_TXDESCS];	/* TX pbuf pointer list, zero-copy mode */
  u32_t lpc_last_tx_idx;			/* TX last descriptor index, zero-copy mode */
#else
  /* Array of contiguous TX buffers used with copied pbufs */
  u32_t lpc_tx_buffs[LPC_NUM_BUFF_TXDESCS][1 + (EMAC_ETH_MAX_FLEN / 4)];
#endif
};

 void lpc_mii_write_noblock(u32_t PhyReg, u32_t Value);
err_t lpc_mii_write(u32_t PhyReg, u32_t Value);
u32_t lpc_mii_read_status(void);
u32_t lpc_mii_read_data(void);
err_t lpc_mii_read(u32_t PhyReg, u32_t *data);
 void lpc_mii_read_noblock(u32_t PhyReg);
 void lpc_enetif_input(struct netif *netif);
u32_t lpc_tx_ready(struct netif *netif);
err_t lpc_enetif_init(struct netif *netif);

#if LPC_PBUF_RX_ZEROCOPY
s32_t lpc_rx_queue(struct netif *netif);
#endif

#if LPC_PBUF_TX_ZEROCOPY
void lpc_tx_reclaim(struct netif *netif);
#endif

#ifdef __cplusplus
}
#endif

#endif
