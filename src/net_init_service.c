/* 
 * Top-level networking routines.
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

#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/init.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "netif/etharp.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#include "arch/lpc_arch.h"
#include "lpc_emac.h"
#include "lpc_phy.h"
#include "sntp/sntp.h"
#include "server_tcp.h"
#include "console.h"
#include "upload.h"
#include "memory/memory.h"
#include "debug.h"
#include "leds.h"
#include "lwip/stats.h"

/* ---- GLOBALS ---- */
struct netif lpc_netif;
int32_t rx_queue_stat;

const char* match = "\r\n\r\n";
uint8_t match_index = 0;
char body[8];
uint8_t body_index = 0;

/**
 * This *doesn't* correspond with the green led
 */
void netif_status_change(struct netif *netif) {
  if (netif->flags & NETIF_FLAG_UP) {
    debug_printf("Ethernet Active at IP: %d.%d.%d.%d\n",
		 ip4_addr1(&netif->ip_addr), ip4_addr2(&netif->ip_addr),
		 ip4_addr3(&netif->ip_addr), ip4_addr4(&netif->ip_addr));

    /* Initialise the SNTP Client */
    sntp_init();
  } else {
    /* Cleanup any connections */
    sntp_cleanup();
  }
}

/**
 * Used to start all networking operations
 */
void net_init(void) {
  /* Setup a 1mS sysTick for the primary time base */
  SysTick_Enable();

  /* Initialise LWIP */
  lwip_init();

  /* Initialise the LEDs on the Network Port */
  INIT_LEDS();

  /* Set the IP address */
  ip_addr_t ipaddr, netmask, gw;

#if LWIP_DHCP
  IP4_ADDR(&gw, 0, 0, 0, 0);
  IP4_ADDR(&ipaddr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
#else
  IP4_ADDR(&gw, 192, 168, 1, 1);
  IP4_ADDR(&ipaddr, 192, 168, 1, 234);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
#endif

  /* Add netif interface for lpc17xx */
  netif_add(&lpc_netif, &ipaddr, &netmask, &gw, NULL, lpc_enetif_init, ethernet_input);
  netif_set_status_callback(&lpc_netif, (netif_status_callback_fn)netif_status_change);
  netif_set_default(&lpc_netif);

  /* Queue RX buffers as needed */
  while (lpc_rx_queue(&lpc_netif));

#if LWIP_DHCP
  dhcp_start(&lpc_netif); /* Start DHCP */
#else
  netif_set_up(&lpc_netif); /* The netif is ready to use straight away */
#endif
}

/**
 * Used to service all networking operations
 */
void net_service(void) {
  do { /* Re-queue RX buffers as needed */
    rx_queue_stat = lpc_rx_queue(&lpc_netif);
  } while (rx_queue_stat == 1); /* While there are more descriptors to re-queue */

  if (rx_queue_stat != -1) { /* If re-queueing didn't error */
    /* Handle a packet */
    lpc_enetif_input(&lpc_netif);
  }

  /* Free TX buffers that are done sending */
  lpc_tx_reclaim(&lpc_netif);

  /* LWIP timers - ARP, DHCP, TCP, etc. */
  sys_check_timeouts();

  /* Call the PHY status update state machine once in a while to keep
   * the link status up-to-date */
  if (lpc_phy_sts_sm(&lpc_netif) != 0) { /* If the PHY status has changed */
    /* Set the state of the LED to on if the ethernet link is active
     * or off is disconnected. */
    if (lpc_netif.flags & NETIF_FLAG_LINK_UP) {
      GREEN_ON();
    } else {
      GREEN_OFF();
    }
  }

  /* Queue any console data that's hanging about back for transfer to the server */
  console_flush();

  upload(&lpc_netif);

  /* Sends data to the server, keeps it connected */
  server_tcp_service(&lpc_netif);
}
