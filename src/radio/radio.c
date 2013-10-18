/* 
 * High level functions for operating the radio
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

#include <string.h>
#include <stdlib.h>
#include "radio.h"

/**
 * Do a command
 */
uint8_t radif_command(uint8_t command, struct radif* radif) {
  radif->Command = command;

  /* Trigger the interrupt */
  radif->interrupt_trigger();

  /* Wait for the command to be completed */
  while (radif->Command != 0);

  return radif->CommandOutput;
}
/**
 * Calls the receive callback on all pending frames
 */
void radif_service(struct radif* radif) {
  /* While there's something to read from the rx buffer */
  while (radif->RxConsumeIndex != radif->RxProduceIndex) {
    if (radif->rx_callback != 0) { /* If we actually have a callback to use */
      uint16_t index = radif->RxConsumeIndex;

      /* Make the callback */
      radif->rx_callback(&radif->RxFrames[index]);

      /* Increment our consume index */
      radif->RxConsumeIndex = (index+1) % NUM_RXFRAMES;
    }
  }
}
/**
 * Transmits a frame over the radio interface
 */
void radif_send(uint8_t* frame, uint8_t len, uint16_t dest_addr, uint8_t ack, struct radif* radif) {
  /* If the length is too big, halt! */
  if (len > 127) {
    while(1); // ERROR - Frame too long!
  }

  uint8_t index = radif->TxProduceIndex;
  uint8_t next = (index+1) % NUM_TXFRAMES;

  if (next == radif->TxConsumeIndex) { /* If we're about to run into the consume index */
    if (radif->up) { /* If the interface is up */
      radif->interrupt_trigger(); /* Trigger some data getting sent */
      while (next == radif->TxConsumeIndex); /* Wait for the condition to clear */
    } else { return; } /* We simply can't send */
  }

  struct tx_frame* tx = &radif->TxFrames[index];

  /* Setup the frame in the buffer */
  memcpy(tx->data, frame, len);
  tx->destination_address = dest_addr;
  tx->length = len;
  tx->ack = ack;

  /* Increment the produce index */
  radif->TxProduceIndex = next;

  /* Trigger the interrupt to get the frame sent if possible */
  radif->interrupt_trigger();
}

/**
 * Initialises the radio interface
 */
void radif_init_struct(struct radif* radif) {
  /* Clear everything in the struct to zero */
  memset((void*)radif, 0, sizeof(struct radif));
}
