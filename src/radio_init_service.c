/* 
 * Top-level radio routines
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

#include <stdlib.h>
#include <time.h>
#include "radio/rf212.h"
#include "radio.h"
#include "sntp/time.h"
#include "memory/write.h"
#include "frame_processor.h"
#include "console.h"
#include "debug.h"
#include "leds.h"

#define RADIO_DEBUGF		console_printf

/**
 * A debug message has been received
 */
static void radio_debug_frame(struct rx_frame* rx) {
  uint8_t buffer[4];
  buffer[0] = 'D'; buffer[1] = 'D';
  buffer[2] = 'D'; buffer[3] = 'D'; /* Send a quick acknowledgement */
  radif_send(buffer, 4, rx->source_address, 1, &rf212_radif);

  RADIO_DEBUGF("Remote Debug: %s", (char*)rx->data+1);
}
/**
 * A request for the current time has been received
 */
static void radio_timereq_frame(struct rx_frame* rx) {
  uint8_t buffer[9];

  /* If our value for the time is valid */
  if (is_time_valid() != 0) {

    /* Get the current time */
    uint64_t time_now = get_current_time();

    RADIO_DEBUGF("Transmitting current time... "); print_current_time();

    buffer[0] = 'T';
    buffer[1] = time_now & 0xFF; time_now >>= 8;
    buffer[2] = time_now & 0xFF; time_now >>= 8;
    buffer[3] = time_now & 0xFF; time_now >>= 8;
    buffer[4] = time_now & 0xFF; time_now >>= 8;
    buffer[5] = time_now & 0xFF; time_now >>= 8;
    buffer[6] = time_now & 0xFF; time_now >>= 8;
    buffer[7] = time_now & 0xFF; time_now >>= 8;
    buffer[8] = time_now & 0xFF;

    radif_send(buffer, 9, rx->source_address, 1, &rf212_radif);
  } else {
    RADIO_DEBUGF("Ignoring request for time: Our Time is Invalid\n");
  }

  /* Save the RSSI */
  write_sample_to_mem(61 << 26, rx->energy_detect, 0, 0);
}

void rf212_rx_callback(struct rx_frame* rx) {

  switch (rx->data[0]) {
    case 'D':	radio_debug_frame(rx);
      break;
    case 'T':	radio_timereq_frame(rx);
      break;
    case 'U':	block_uploaded(rx);
      break;
    default:	RADIO_DEBUGF("Unknown radio frame type '%c' received from %02X\n",
			     rx->data[0], rx->source_address);
      break;
  }
}

/**
 * Used to start all radio operations
 */
void radio_init(void) {
  rf212_init(rf212_rx_callback);
}
/* Processes radio operations */
void radio_service(void) {
  rf212_service();
}
