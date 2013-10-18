/* 
 * Implements a console that prints over HTTP
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include "LPC17xx.h"

#define CONSOLE_BUF_LEN		200

uint16_t console_buf_index;
char console_buf[CONSOLE_BUF_LEN];

#ifndef DEBUG
#define console_printf(...)
#define console_putchar(c)
#define console_puts(s)
#define init_console(void)
#define console_flush(void)
#else
#define console_printf(...)			_console_printf(__VA_ARGS__)
#define console_putchar(c)			_console_putchar(c)
#define console_puts(s)				_console_puts(s)
#define init_console()				_init_console()
#define console_flush()				_console_flush()
#endif

void _console_flush(void);

void _console_printf(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));
int _console_putchar (char c);
void _console_puts(const char *s);

void _tftp_console(void);

#endif /* CONSOLE_H */
