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

#include <stdarg.h>
#include <stdint.h>
#include "server_tcp.h"
#include "console.h"

#undef CONSOLE_ACTIVE

/**
 * Outputs the contents of our buffer to the server.  TODO Make this
 * actually do something. Currently disabled as it's not appropriate
 * for the database.
 */
void _console_flush(void) {
#ifdef CONSOLE_ACTIVE
  /* If we've got something to write, and there's space to do so */
  if (console_buf_index && server_tcp_check_free()) {
  /* Upload it over the TCP connection if possible */
    if (http_packet("/console", "text/plain", console_buf, console_buf_index)) {
  /* If we were able to upload */
      console_buf_index = 0;
    }
  }
#endif
}

int _console_putchar(char c) {
#ifdef CONSOLE_ACTIVE
  if (console_buf_index < CONSOLE_BUF_LEN) { /* Space in the buffer */
    console_buf[console_buf_index++] = c; /* Write */
  } else {
    _console_flush(); /* Attempt to make some space */
  }
#endif
  return 1;
}
void _console_printf(const char *format, ...) {
#ifdef CONSOLE_ACTIVE
  va_list args;

  va_start(args, format);
  printf_format(_console_putchar, format, args);
#endif
}
void _console_puts(const char *s) {
#ifdef CONSOLE_ACTIVE
  while(*s) {
    _console_putchar(*(s++));
  }
  _console_putchar('\n');
#endif
}

void _init_console(void) {
#ifdef CONSOLE_ACTIVE
  console_buf_index = 0;
#endif
}
