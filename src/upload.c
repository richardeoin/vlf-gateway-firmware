/* 
 * Manages uploads to a remote database. You will probably need to
 * change the address!
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

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "server_tcp.h"
#include "lwip/opt.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "http_tx.h"
#include "upload.h"
#include "memory/memory.h"

uint8_t active = 0;

/* About 10 seconds until we make our first connection */
uint32_t upload_countdown = 2000*10;
/* How long we wait between uploads. About 5 minutes */
#define UPLOAD_WAIT (2000*60*5)

/* Addressing */
//#define COUCHDB_SERVER_DNS
static char* couchdb_server_address = "192.168.1.8";
ip_addr_t couchdb_ip;
static char* couchdb_server_auth = NULL;

/**
 * Called when the upload process ends, either through success or failure.
 */
void upload_end(void) {
  active = 0;
  /* Reset the countdown */
  upload_countdown = UPLOAD_WAIT;
}
/**
 * Called to start the upload process.
 */
void couchdb_start_upload(ip_addr_t* ip_addr) {
  /* Make a connection to the database */
  if (server_tcp_init(couchdb_server_address, ip_addr, 5984, couchdb_server_auth,
		      get_current_read_block(), get_blocks_to_read(), upload_end) != ERR_OK) {
    /* Failure */
    upload_end();
  }
}

#ifdef COUCHDB_SERVER_DNS

/**
 * DNS found callback.
 */
static void couchdb_dns_found(const char* hostname, ip_addr_t *ipaddr, void *arg) {
  LWIP_UNUSED_ARG(hostname);
  LWIP_UNUSED_ARG(arg);

  if (ipaddr != NULL) {
    /* Address resolved, start upload */
    couchdb_start_upload(ipaddr);
  } else {
    /* DNS resolving failed */
    upload_end();
  }
}
/**
 * DNS lookup.
 */
void couchdb_dns_lookup(void) {
  err_t err;

  /* Attempt the DNS lookup */
  err = dns_gethostbyname(couchdb_server_address, &couchdb_ip, couchdb_dns_found, NULL);

  if (err == ERR_INPROGRESS) {
    /* DNS request sent, waiting for sntp_dns_found being called */
  } else if (err == ERR_OK) {
    /* We already have the IP Address */
    couchdb_start_upload(&couchdb_ip);
  } else {
    /* Address conversion failed */
    upload_end();
  }
}

#endif

/**
 * Attempts to transfer the contents of the memory to the database.
 */
void upload(struct netif* netif) {
  if (upload_countdown == 0) {
    if (!active) {
      /* If both the protocol and hardware layers are operational */
      if ((netif->flags & NETIF_FLAG_UP) && (netif->flags & NETIF_FLAG_LINK_UP)) {
	/* The upload process is now starting */
	active = 0xF;

#ifdef COUCHDB_SERVER_DNS
	/* Put in a DNS request for the server */
	couchdb_dns_lookup();
#else
	/* Attempt to parse the address */
	if (ipaddr_aton(couchdb_server_address, &couchdb_ip)) {
	  /* Connect to the server */
	  couchdb_start_upload(&couchdb_ip);
	} else {
	  /* Address parsing failed */
	  upload_end();
	}
#endif
      }
    }
  } else {
    upload_countdown--;
  }
}
