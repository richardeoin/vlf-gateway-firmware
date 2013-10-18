/* 
 * Utility functions used to parse a HTTP response
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

#include "LPC17xx.h"
#include "memory/memory.h"

/**
 * The 'good' response string: "id":
 */
const char* good = "\"id\":";
const uint16_t good_len = 5; uint16_t good_index;
/**
 * The 'bad' response string: unused-unused-unused
 */
const char* bad = "unused-unused-unused";
const uint16_t bad_len = 20; uint16_t bad_index;

/**
 * Parses the HTTP response from a CouchDB _bulk_docs call, calling the
 * good_upload_done and bad_upload_done functions as appropriate.
 */
void parse_http_response_buffer(uint8_t* ptr, uint16_t len) {
  while (len) { /* While there are more octets left */

    if (*ptr != good[good_index++]) { /* Fails to match good */
      good_index = 0;

    } else if (good_index == good_len) {
      /* If we've matched all the characters in the good string */
      /* Call the good upload handler */
      good_upload_done();
      good_index = 0;
    }

    if (*ptr != bad[bad_index]) { /* Fails to match bad */
      bad_index = 0;
    } else if (bad_index == bad_len) {
      /* If we've matched all the characters in the bad string */
      /* Call the bad upload handler */
      // TODO bad_upload_done();
      bad_index = 0;
    }

    /* Move on the the next octet */
    ptr++; len--;
  }
}

/**
 * Resets the HTTP Response Parser ready to receive a new response.
 */
void reset_http_response_parser(void) {
  good_index = bad_index = 0;
}
