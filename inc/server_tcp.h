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

#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include "lwip/tcp.h"

#define SERVER_TCP_DEBUG		LWIP_DBG_OFF

#define MAX_WRITE_ERR_BEFORE_CLOSE	20*1000

typedef void (*tcp_close_func) (void);

uint16_t server_tcp_print_hostname(char* str);
void server_tcp_service(struct netif* netif);
err_t server_tcp_init(char* host, ip_addr_t* remote_ip, uint16_t remote_port,
		      char* auth, uint32_t read_index, uint16_t records_count,
		      tcp_close_func callback);

#endif /* SERVER_TCP_H */
