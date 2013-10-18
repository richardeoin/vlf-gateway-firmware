/*
 * Overrides for LWIP options
 * Copyright (C) 2013  Richard Meadows
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* Standalone build */
#define NO_SYS						1

/* Use LWIP timers */
#define NO_SYS_NO_TIMERS				0

/* 32-bit alignment */
#define MEM_ALIGNMENT					4

/* The number of pbuf buffers in pool. */
#define PBUF_POOL_SIZE					10

/* No padding needed */
#define ETH_PAD_SIZE					0

#define IP_SOF_BROADCAST				1
#define IP_SOF_BROADCAST_RECV				1

/* Stops a memory leak in ip_frag, still doesn't make it work though */
#define IP_FRAG_USES_STATIC_BUF				1

/* The ethernet FCS is performed in hardware. The IP, TCP, and UDP
   CRCs still need to be done in software. */
#define CHECKSUM_GEN_IP					1
#define CHECKSUM_GEN_UDP				1
#define CHECKSUM_GEN_TCP				1
#define CHECKSUM_CHECK_IP				1
#define CHECKSUM_CHECK_UDP				1
#define CHECKSUM_CHECK_TCP				1
#define LWIP_CHECKSUM_ON_COPY				1

/* Use inline assembler for converting between network and host byte order */
#define LWIP_PLATFORM_BYTESWAP				__rev

/**
 * MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high.
 *
 * In this case the Peripheral RAM Blocks are used. 32K = 0x8000
 * We need some room for alignment and the memory structs too though.
 */
#define MEM_SIZE					0x7E94

/* DHCP is ok, UDP is required with DHCP */
#define LWIP_DHCP					1
#define LWIP_UDP					1
#define LWIP_DNS					1

/* Only keep 2 DNS lookups in memory */
#define DNS_TABLE_SIZE					2

/* Only keep 4 ARP lookups in memory */
#define ARP_TABLE_SIZE					4

/* Hostname can be used */
#define LWIP_NETIF_HOSTNAME				0

/* Have a callback when the status of the network connection changes */
#define LWIP_NETIF_STATUS_CALLBACK			1

/* Don't respond to broadcast pings */
#define LWIP_BROADCAST_PING				0

/* MSS should match the hardware packet size */
#define TCP_MSS						1460
#define TCP_SND_BUF					(4 * TCP_MSS)

#define LWIP_SOCKET					0
#define LWIP_NETCONN					0
#define MEMP_NUM_SYS_TIMEOUT				300

#define LWIP_STATS					1
#define LINK_STATS					0
#define LWIP_STATS_DISPLAY				0

/* There are more *_DEBUG options that can be selected.
   See opts.h. Make sure that LWIP_DEBUG is defined when
   building the code to use debug. */
#define LWIP_DEBUG
#include "debug.h"

#define TCP_DEBUG					LWIP_DBG_OFF
#define ETHARP_DEBUG					LWIP_DBG_OFF
#define PBUF_DEBUG					LWIP_DBG_OFF
#define IP_DEBUG					LWIP_DBG_OFF
#define TCPIP_DEBUG					LWIP_DBG_OFF
#define DHCP_DEBUG					LWIP_DBG_OFF
#define UDP_DEBUG					LWIP_DBG_OFF
#define DNS_DEBUG					LWIP_DBG_ON

/* This define is custom for the LPC EMAC driver. Enabled it to
   get debug messages for the driver. */
#define UDP_LPC_EMAC					LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */
