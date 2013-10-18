/* 
 * Processes data as it is uploaded from the remote node
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
#include "frame_processor.h"
#include "radio.h"
#include "radio/rf212.h"
#include "memory/memory.h"
#include "memory/checksum.h"
#include "console.h"

uint8_t record[MEMORY_RECORD_SIZE];

uint8_t sample_ack_packet[9];

/**
 * Constructs and sends a frame acknowledgement packet.
 */
void send_upload_ack(uint16_t rf_address, uint32_t mem_address, uint32_t checksum) {
  /* Assemble an ack packet */
  sample_ack_packet[0] = 'A';
  uint32_t* addr_ptr = (uint32_t*)(sample_ack_packet+1); addr_ptr[0] = mem_address;
  uint32_t* chk_ptr = (uint32_t*)(sample_ack_packet+5); chk_ptr[0] = checksum;

  /* Send the frame */
  radif_send(sample_ack_packet, 9, rf_address, 1, &rf212_radif);
}

/**
 * Returns the memory address field encoded in a block.
 */
uint32_t get_memory_address_from_rx(struct rx_frame* rx) {
  return rx->data[1] |
    rx->data[2] << 8 |
    rx->data[3] << 16 |
    rx->data[4] << 24;
}

/**
 * Used to process a data frame that has just been uploaded.
 */
void block_uploaded(struct rx_frame* rx) {
  uint32_t mem_addr;
  uint32_t checksum, actual_checksum;

  if (rx->length >= MEMORY_RECORD_SIZE) {
    /* Copy the record from the radio packet. Skip the header and memory address */
    memcpy(record, rx->data+5, MEMORY_RECORD_SIZE);

    /* Calculate the checksum */
    actual_checksum = calculate_checksum(record);
    /* Read the checksum */
    checksum = get_checksum(record);

    if (checksum == actual_checksum) {
      /* Extract the memory address */
      mem_addr = get_memory_address_from_rx(rx);

      /* Acknowledge the frame */
      send_upload_ack(rx->source_address, mem_addr, checksum);

      /* Write the record out to memory */
      put_sample(record);

      console_puts("Radio Upload Frame OK\n");
    } else {
      console_puts("Radio Upload Frame Checksum Error!\n");
      console_printf("Frame: %04x\nCalc: %04x\n", checksum, actual_checksum);
    }
  } else {
    console_puts("Radio Upload Frame too short!\n");
  }
}
