/* 
 * Functions for encoding and decoding IEEE 802.15.4 frames
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

#include "radio.h"
#include "ieee_frame.h"

void write_spi_word(uint16_t word, struct radif* radif) {
  radif->spi_xfer(word & 0xFF);
  radif->spi_xfer((word >> 8) & 0xFF);
}
uint16_t read_spi_word(struct radif* radif) {
  uint16_t lsb = radif->spi_xfer(BLANK_SPI_CHARACTER);
  return (radif->spi_xfer(BLANK_SPI_CHARACTER) << 8) | lsb;
}

uint8_t ieee_header_len(struct tx_frame* tx) {
  /* TODO this might be determined by what addressing is in use in the tx status */
  return 9;
}
void write_out_ieee_header(struct tx_frame* tx, struct radif* radif) {
  uint16_t fcf = (FRAME_PAN_ID_16BIT_ADDR << 14) | /* Source addressing mode */
    (FRAME_VERSION_IEEE_2006 << 12) | /* Frame version */
    (FRAME_PAN_ID_16BIT_ADDR << 10) | /* Destination addressing mode */
    FCF_PAN_ID_COMPRESSION |
    FRAME_TYPE_DATA; /* Frame type = data */
  /* Set the acknowledge flag if we've been asked */
  if (tx->ack > 0) { fcf |= FCF_ACKNOWLEDGE_REQUEST; }

  /* Frame Control Field (FCF) */
  write_spi_word(fcf, radif);

  /* Sequence Number */
  radif->spi_xfer(radif->seq++);

  /* Destination PAN ID */
  write_spi_word(radif->pan_id, radif);

  /* Destination Address */
  write_spi_word(tx->destination_address, radif);

  /* Source Address */
  write_spi_word(radif->short_address, radif);

  /* Possible Security Header */
}

/* Returns: How many bytes have been read, unless the frame is invalid, in which case 0 is returned. */
uint8_t read_in_ieee_header(struct rx_frame* rx, struct radif* radif) {
  /* Frame Control Field (FCF) */
  uint16_t fcf = read_spi_word(radif);
  /* Decode the FCF */
  uint8_t pan_id_compression = (fcf & FCF_PAN_ID_COMPRESSION);
  uint8_t dest_addr_mode = (fcf >> 10) & 0x3;
  uint8_t frame_version = (fcf >> 12) & 0x3;
  uint8_t src_addr_mode = (fcf >> 14) & 0x3;

  /* Sequence Number */
  uint8_t seq = radif->spi_xfer(BLANK_SPI_CHARACTER);

  /* Keep track for how many bytes we've read from this point on */
  uint8_t read = 3;

  if (dest_addr_mode == FRAME_PAN_ID_16BIT_ADDR) {
    read_spi_word(radif); /* PAN ID */
    read_spi_word(radif); /* Destination Address */
    read += 4;
  } else if (dest_addr_mode == FRAME_PAN_ID_64BIT_ADDR) {
    read_spi_word(radif); /* PAN ID */
    read += 2;
    /* TODO */
  }

  if (src_addr_mode == FRAME_PAN_ID_16BIT_ADDR) {
    if (pan_id_compression) {
      /* TODO */
    } else {
      read_spi_word(radif); /* PAN ID */
      read += 2;
    }
    rx->source_address = read_spi_word(radif); /* Source Address */
    read += 2;
  } else if (src_addr_mode == FRAME_PAN_ID_64BIT_ADDR) {
    if (pan_id_compression) {
      /* TODO */
    } else {
      read_spi_word(radif); /* PAN ID */
      read += 2;
    }
    /* TODO */
  }

  /* TODO: Possible Security Header */

  return read;
}
