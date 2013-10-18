/* 
 * Makes a TCP connection to a remote server.
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
/**
 * This file is based upon echo.c but has been heavily modifed. The
 * original header is included below.
 */
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of and a contribution to the lwIP TCP/IP stack.
 *
 * Credits go to Adam Dunkels (and the current maintainers) of this software.
 *
 * Christiaan Simons rewrote this file to get a more stable echo example.
 */

#include <string.h>
#include <stdio.h>
#include "server_tcp.h"
#include "http_tx.h"
#include "http_rx.h"
#include "memory/memory.h"
#include "lwip/ip_addr.h"
#include "lwip/ephemeral.h"

/* -------- An object to hold the state of the server connection -------- */
/* Connected */
enum {
  CONNECTED    = 1,
  DISCONNECTED = 0
};
/* Output Phase */
enum {
  OP_HTTP_HEADER       = 0,
  OP_JSON_HEADER       = 1,
  OP_JSON_DATA         = 2,
  OP_JSON_FOOTER       = 3,
  OP_WAIT_BEFORE_CLOSE = 4,
  OP_CLOSE             = 5
};

/* Approximately 15 seconds */
#define WAIT_BEFORE_CLOSE	2000*15

struct server_tcp_state {
  struct tcp_pcb* pcb;	/* A pointer to our current packet control block */

  /* ======== Status ======== */
  uint8_t connected; /* O = Disconnected, 1 = Connected */

  /* ======== HTTP ======== */
  char* host;
  char* auth;

  /* ======== IP ======== */
  ip_addr_t* remote_ip;
  uint16_t remote_port;

  /* ======== Transmit ======== */
  uint8_t output_phase;			/* The current stage of our packet output */
  uint32_t mem_output_index;      /* The memory index that we're currently outputting */

  uint16_t records_index;			/* Our current index in the records */
  uint16_t records_count;			/* The total number of records that are going to be output */

  char output_buffer[400];
  uint16_t current_pos;			/* Our current position in the output buffer */
  uint16_t current_len;			/* The length of the data in the output buffer */

  /* ======== Closing ======== */
  uint16_t wait_before_close_counter;
  tcp_close_func tcp_close_callback; /* Called when the connection is closed and ready to be re-used */

  /* ======== Errors ======== */
  uint32_t write_err;
};

/* -------- TCP Connection Callbacks -------- */

/**
 * A connection on this PCB has beens successfully opened.
 */
static err_t server_tcp_connected(void* arg, struct tcp_pcb* pcb, err_t err) {
  struct server_tcp_state* ss = (struct server_tcp_state*)arg;
  LWIP_UNUSED_ARG(err);

  /* If we haven't already cleared up this transaction */
  if (ss != NULL) {
    ss->connected = CONNECTED;
    /* Reset the parser ready for the response */
    reset_http_response_parser();
  }

  return ERR_OK;
}
/**
 * The connection shall be actively closed.
 * Reset the sent and receive callbacks.
 */
static err_t server_tcp_close_conn(struct tcp_pcb* pcb, struct server_tcp_state* ss) {
  err_t err;
  LWIP_DEBUGF(SERVER_TCP_DEBUG, ("Closing connection %p\n", (void*)pcb));

  /* Clear up all the callbacks */
  tcp_arg(pcb, NULL);
  tcp_err(pcb, NULL);
  tcp_recv(pcb, NULL);
  tcp_sent(pcb, NULL);

  err = tcp_close(pcb);
  if (err != ERR_OK) {
    LWIP_DEBUGF(SERVER_TCP_DEBUG, ("Error %d closing %p\n", err, (void*)pcb));
  } else {
    /* Make our callback to say we're closed */
    if (ss->tcp_close_callback != NULL) {
      ss->tcp_close_callback();
    }
  }

  return err;
}
/**
 * The PCB had an error and is already deallocated.
 */
static void server_tcp_err(void* arg, err_t err) {
  struct server_tcp_state* ss = (struct server_tcp_state*)arg;
  LWIP_UNUSED_ARG(err);

  /* We are now most definitely disconnected */
  ss->connected = DISCONNECTED;

  if (ss->pcb != NULL) {
    server_tcp_close_conn(ss->pcb, ss);
  }

  LWIP_DEBUGF(SERVER_TCP_DEBUG, ("server_tcp_err: %s", lwip_strerr(err)));
}

/* -------- TCP Read, Write and Poll Functions and Callbacks -------- */

/** Call tcp_write() in a loop trying smaller and smaller length
 *
 * @param pcb tcp_pcb to send
 * @param ptr Data to send
 * @param length Length of data to send (in/out: on return, contains the
 *        amount of data sent)
 * @param apiflags directly passed to tcp_write
 * @return the return value of tcp_write
 */
static err_t server_tcp_write(struct tcp_pcb* pcb, const void* ptr, uint16_t* length, uint8_t apiflags) {
  uint16_t len;
  err_t err;
  LWIP_ASSERT("length != NULL", length != NULL);
  len = *length;
  do {
    LWIP_DEBUGF(SERVER_TCP_DEBUG | LWIP_DBG_TRACE, ("Trying to send %d bytes\n", len));
    err = tcp_write(pcb, ptr, len, apiflags);

    /* If there's not enough memory to send this data */
    if (err == ERR_MEM) {
      /* If there's no space in the buffers at all */
      if ((tcp_sndbuf(pcb) == 0) || (tcp_sndqueuelen(pcb) >= TCP_SND_QUEUELEN)) {
	len = 1; /* No need to try smaller sizes, exit */
      } else {
	len /= 2;
      }
      LWIP_DEBUGF(SERVER_TCP_DEBUG | LWIP_DBG_TRACE, ("Send failed, trying less (%d bytes)\n", len));
    }
  } while ((err == ERR_MEM) && (len > 1));

  if (err == ERR_OK) {
    LWIP_DEBUGF(SERVER_TCP_DEBUG | LWIP_DBG_TRACE, ("Sent %d bytes\n", len));
  } else {
    LWIP_DEBUGF(SERVER_TCP_DEBUG | LWIP_DBG_TRACE, ("Send failed with err %d (\"%s\")\n", err, lwip_strerr(err)));
  }

  *length = len;
  return err;
}
/**
 * Outputs the current data block. Returns true if the current data block is empty.
 */
static uint8_t server_current_tcp_out(struct tcp_pcb *pcb, struct server_tcp_state *ss) {
  /* Find the length left to output */
  uint16_t len_val = ss->current_len - ss->current_pos;

  if (len_val > 0) {
    /* Attempt to write out this data */
    if (server_tcp_write(pcb, (ss->output_buffer+ss->current_pos), &len_val, TCP_WRITE_FLAG_COPY) == ERR_OK) {
      ss->current_pos += len_val; /* Move forward by how many bytes we've successfully sent */

      if (ss->current_pos >= ss->current_len) { /* We've reached the end of this block */
	return 1; /* The current data block is now empty */
      }
      ss->write_err = 0;
    } else { /* Our write failed */
      if (++ss->write_err > MAX_WRITE_ERR_BEFORE_CLOSE) { /* If this appears to be a permanent problem */
	server_tcp_close_conn(pcb, ss);
      }
    }
    return 0; /* More data to write */
  }
  return 1; /* The current data block is empty */
}
/**
 * Supplies blocks of data to server_current_tcp_out.
 */
static void server_tcp_out(struct tcp_pcb *pcb, struct server_tcp_state *ss) {
  if (ss->connected) { /* If we're connected */
    if (server_current_tcp_out(pcb, ss)) { /* If the current block is empty */
      /* Clear our position */
      ss->current_pos = 0;
      ss->current_len = 0;
      /* Select a new data block to output */
      switch(ss->output_phase) {
	case OP_HTTP_HEADER: /* Send a header that specifies how many records we are going to send */
	  ss->current_len = http_header(ss->output_buffer, ss->auth, ss->records_count);
	  ss->output_phase++;
	  break;
	case OP_JSON_HEADER: /* Start the JSON object */
	  ss->current_len = json_header(ss->output_buffer);
	  ss->output_phase++;
	  break;
	case OP_JSON_DATA:
	  if (ss->records_index++ < ss->records_count) { /* If there are more records to be output */
	    /* Read from memory, and encode as a JSON element */
	    ss->current_len = json_element(ss->output_buffer,
					   get_sample(ss->mem_output_index++), /* Read in the data from memory */
					   (ss->records_index < ss->records_count)); /* If this isn't the last, we need a comma */
	  } else { ss->output_phase++; }
	  break;
	case OP_JSON_FOOTER: /* End the JSON object */
	  ss->current_len = json_footer(ss->output_buffer);
	  ss->output_phase++;
	  break;
	case OP_WAIT_BEFORE_CLOSE: /* Wait before closing the connection */
	  if (ss->wait_before_close_counter++ > WAIT_BEFORE_CLOSE) {
	    ss->output_phase++;
	  }
	  break;
	case OP_CLOSE:
	  tcp_close(ss->pcb); /* Close the connection */
	  ss->output_phase++;
	  break;
      }

      /* And output that */
      server_current_tcp_out(pcb, ss);
    }
  }
}
/**
 * Data has been sent and acknowledged by the remote host.
 * This means that more data can be sent.
 */
static err_t server_tcp_sent(void* arg, struct tcp_pcb* pcb, u16_t len) {
  struct server_tcp_state* ss = (struct server_tcp_state*)arg;

  LWIP_DEBUGF(SERVER_TCP_DEBUG | LWIP_DBG_TRACE, ("server_sent %p\n", (void*)pcb));

  /* If we haven't already cleared up this transaction */
  if (ss != NULL) {
    /* Try to output some more data from the server buffer */
    server_tcp_out(pcb, ss);
  }

  return ERR_OK;
}
/**
 * Called every 500ms. Doesn't do anything at the moment.
 */
static err_t server_tcp_poll(void* arg, struct tcp_pcb* pcb) {
  //struct server_tcp_state* ss = (struct server_tcp_state*)arg;

  return ERR_OK;
}
/**
 * Called when data has been received on this pcb.
 */
static err_t server_tcp_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err) {
  struct server_tcp_state* ss = (struct server_tcp_state*)arg;
  struct pbuf* pp;

  LWIP_DEBUGF(SERVER_TCP_DEBUG | LWIP_DBG_TRACE, ("server_tcp_recv: pcb=%p pbuf=%p err=%s\n", (void*)pcb,
						  (void*)p, lwip_strerr(err)));

  /* If there's an error */
  if ((err != ERR_OK) || (p == NULL) || (ss == NULL)) {
    /* Error or closed by other side? */
    if (p != NULL) {
      /* Inform TCP that we have taken the data. */
      tcp_recved(pcb, p->tot_len);
      pbuf_free(p);
    }
    if (ss == NULL) {
      /* This should not happen, only to be robust */
      LWIP_DEBUGF(SERVER_TCP_DEBUG, ("Error, server_tcp_recv: ss is NULL, close\n"));
    }

    server_tcp_close_conn(pcb, ss);
    return ERR_OK;
  }

  /* Make a secondary copy of the pbuf that we can traverse */
  pp = p;

  /* Traverse the pbuf chain */
  do {
    /* Parse this data in the http module */
    parse_http_response_buffer(pp->payload, pp->len);
  } while((pp = pp->next)); /* While there's another item in the chain */

  /* Inform TCP that we have taken the data. */
  tcp_recved(pcb, p->tot_len);

  /* We're finished with the pbuf now */
  pbuf_free(p);

  return ERR_OK;
}

/* -------- External Functions -------- */

/* Create an instance of the server state struct we'll use for our entire lifetime */
struct server_tcp_state server_tcp;

/**
 * Prints the hostname of the server we're currently attached to.
 * It is the caller's job to make sure that the passed string is long enough.
 * Returns the number of bytes written.
 */
uint16_t server_tcp_print_hostname(char* str) {
  return sprintf(str, "%s", server_tcp.host);
}

/**
 *  Called as part of the processing loop
 */
void server_tcp_service(struct netif* netif) {
  if (server_tcp.connected == CONNECTED) { /* If we're connected to the server */
    /* Get some data sent */
    server_tcp_out(server_tcp.pcb, &server_tcp);
  }
}

/**
 * Initialises the server_tcp struct and sets various properties of the server_tcp object.
 * It then attempts to make a connection to the remote end.
 */
err_t server_tcp_init(char* host, ip_addr_t* remote_ip, uint16_t remote_port, char* auth,
		      uint32_t read_index, uint16_t records_count, tcp_close_func callback) {

  err_t err;

  if (records_count > 0) { /* If there really is some data to be sent */
    /* Clear the struct */
    memset(&server_tcp, 0, sizeof(server_tcp));

    /* Set various properties */
    server_tcp.host = host;
    server_tcp.auth = auth;
    server_tcp.remote_ip = remote_ip;
    server_tcp.remote_port = remote_port;
    server_tcp.mem_output_index = read_index;
    server_tcp.records_count = records_count;
    server_tcp.tcp_close_callback = callback;

    /* Create a new Packet Control Block */
    server_tcp.pcb = tcp_new(); if (server_tcp.pcb == NULL) { return ERR_MEM; }

    /* Set the priority of this Packet Control Block */
    tcp_setprio(server_tcp.pcb, TCP_PRIO_NORMAL);

    /* Bind to all the local addresses */
    uint8_t i = 100;
    while ((err = tcp_bind(server_tcp.pcb, IP_ADDR_ANY, EPHEMERAL_PORT())) != ERR_OK) {
      if (--i == 0) { return err; } /* Timeout - we simply can't bind */
    }

    /* Send the server state to every callback */
    tcp_arg(server_tcp.pcb, &server_tcp);

    /* Set the error callback */
    tcp_err(server_tcp.pcb, server_tcp_err);
    /* Setup a poll for the application every 500ms */
    tcp_poll(server_tcp.pcb, server_tcp_poll, 1);

    /* Setup sent callback (called once data has been successfully *sent*) */
    tcp_sent(server_tcp.pcb, server_tcp_sent);
    /* Called when data is received */
    tcp_recv(server_tcp.pcb, server_tcp_recv);

    /* Connect to the remote end */
    return tcp_connect(server_tcp.pcb, server_tcp.remote_ip, server_tcp.remote_port, server_tcp_connected);
  } else {
    return ERR_ARG;
  }
}
