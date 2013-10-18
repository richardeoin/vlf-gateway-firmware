/* 
 * Utility functions used to construct a HTTP POST packet
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

#include <stdio.h>
#include <string.h>
#include "server_tcp.h"
#include "base64.h"

/**
 * ======== Packet Structure ========
 *
 * Packet: Little endian
 * ------------------------------------------------------------------------------------------------------------
 * | Header (4 octets) | Time (8 octets) | Left Data (4 octets) | Right Data (4 octets) | Checksum (4 octets) |
 * ------------------------------------------------------------------------------------------------------------
 *
 * Header: Little endian. First two octets are for Left Channel, the second pair is for the right.
 * ---------------------------------------------------------------------------------
 * | 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
 * ---------------------------------------------------------------------------------
 * | Record Type/Target Frequecy |Mic Boost|              PGA Gain                 |
 * ---------------------------------------------------------------------------------
 *
 * Target Frequency:
 *      0-59: Energy Measurement - Value is the target frequency to the nearest kHz
 *      60: Left Data is actually battery measurement.
 *      61: Left Data is actually an radio RSSI measurement.
 *      62: Left Data is actually a time jump record.
 *      63: Data is actually an envelope measurement.
 * Mic Boost:
 *      0 = 33dB
 *      1 = 28dB
 *      2 = 18dB
 *      3 = 13dB
 * PGA Gain:
 *      0 = Mute (-Infinty)
 *      1-255 = (Gain = -97.5+(PGA/2)dB)
 *
 */

/* ======== Defines ======== */

/**
 * The number of characters that the {"docs":[...]} wrapper uses.
 */
#define JSON_OVERHEAD_CHARS	    11
/**
 * The number of characters that each record uses. Records that do not
 * reach this size should be padded out with whitespace until they do
 * (which is fine, whitespace in JSON is cool).
 */
#define JSON_RECORD_CHARS      180

/* ======== Helper Functions for JSONification ======== */

int sprintf_em_channel(char* buffer, uint16_t flags, uint32_t value) {
  uint16_t buff_offset = 0;
  /* Target Frequency (Highest 10 Bits) */
  uint16_t target_freq = (flags >> 10) & 0x3F;
  /* Microphone Boost (Next 2 Bits) */
  uint16_t mic_gain = (flags >> 8) & 0x3;
  /* PGA gain (Lowest 8 Bits) */
  uint16_t pga_gain = flags & 0xFF;
  uint16_t pga_gain_lsb = pga_gain & 1;
  int16_t pga_gain_int;

  switch(mic_gain) {
    case 0: mic_gain = 33; break;
    case 1: mic_gain = 28; break;
    case 2: mic_gain = 18; break;
    case 3: mic_gain = 13; break;
  }

  buff_offset += sprintf(buffer+buff_offset,
			 "{\"target_freq\":%u,\"mic_gain\":%u,\"pga_gain\":",
			 target_freq, mic_gain);

  if (pga_gain == 0) { /* Mute */
    buff_offset += sprintf(buffer+buff_offset, "null");
  } else {
    pga_gain >>= 1; /* Divide by 2 */
    pga_gain_int = pga_gain - 97; /* Adjust for a minimum of -97.5 dB */

    buff_offset += sprintf(buffer+buff_offset, "%d", pga_gain_int);
    if (!pga_gain_lsb) {
      buff_offset += sprintf(buffer+buff_offset, ".5");
    }
  }

  buff_offset += sprintf(buffer+buff_offset, ",\"value\":%lu}", value);

  return buff_offset; /* Max Length 60 chars */
}
int sprintf_em(char* buffer, uint32_t flags, uint32_t left, uint32_t right) {
  uint16_t buff_offset = 0;

  buff_offset += sprintf(buffer+buff_offset, "\"left\":");
  buff_offset += sprintf_em_channel(buffer+buff_offset, flags >> 16, left);

  buff_offset += sprintf(buffer+buff_offset, ",\"right\":");
  buff_offset += sprintf_em_channel(buffer+buff_offset, flags, right);

  return buff_offset; /* Max 140 characters */
}
int sprintf_battery(char* buffer, uint32_t value) {
  return sprintf(buffer, "\"battery\":{\"value\":%lu}", value);
}
int sprintf_rssi(char* buffer, uint32_t value) {
  return sprintf(buffer, "\"rssi\":{\"value\":%lu}", value);
}
int sprintf_envelope(char* buffer, uint32_t left, uint32_t right) {
  uint16_t buff_offset = 0;

  buff_offset += sprintf(buffer+buff_offset,
			 "\"left_envelope\":{\"value\":%lu}", left);
  buff_offset += sprintf(buffer+buff_offset,
			 ",\"right_envelope\":{\"value\":%lu}", right);

  return buff_offset; /* Max 140 characters */
}

/**
 * Writes a HTTP header to `buffer` with optional HTTP basic authorization
 * string (Base64 encoded). Returns the number of bytes written.
 */
int http_header(char* buffer, char* auth, uint16_t records_count) {
  int buff_offset = 0;
  /* Calculate the content length */
  unsigned int content_len = JSON_OVERHEAD_CHARS + /* JSON wrapper */
    (records_count * JSON_RECORD_CHARS) + /* JSON elements */
    (records_count - 1); /* Commas between JSON elements */

  /* Header */
  buff_offset += sprintf(buffer+buff_offset, "POST /vlf_fft/_bulk_docs HTTP/1.1\r\nHost: ");
  buff_offset += server_tcp_print_hostname(buffer+buff_offset);
  buff_offset += sprintf(buffer+buff_offset, "\r\nContent-Length: %d\r\n", content_len);
  if (auth) {
    buff_offset += sprintf(buffer+buff_offset, "Authorization: Basic ");
    buff_offset += base64_encode((unsigned char*)auth, buffer+buff_offset, strlen(auth));
    buff_offset += sprintf(buffer+buff_offset, "\r\n");
  }
  buff_offset += sprintf(buffer+buff_offset, "Content-Type: application/json\r\n");
  buff_offset += sprintf(buffer+buff_offset, "\r\n");

  return buff_offset;
}
/**
 * Writes a JSON header to `buffer`. Returns the number of bytes written.
 */
int json_header(char* buffer) {
  return sprintf(buffer, "{\"docs\":[");
}
/**
 * Writes a JSON object that represents the binary data passed to `buffer`.
 */
int json_element(char* buffer, uint32_t* binary_data, uint8_t comma) {
  int buff_offset = 0;
  uint32_t record_type;

  /* Start the object */
  buff_offset += sprintf(buffer+buff_offset, "{");

  if (binary_data != NULL) {
    /* Print the time as a pair of hex words */
    buff_offset += sprintf(buffer+buff_offset, "\"time\":\"%x%08x\",",
			   (unsigned int)binary_data[2], (unsigned int)binary_data[1]);

    /* Get the record type (Highest 6 Bits) */
    record_type = (binary_data[0] >> 26) & 0x3F;

    switch (record_type) {
      case 60: /* Battery */
	buff_offset += sprintf_battery(buffer+buff_offset, binary_data[3]);
	break;
      case 61: /* Radio RSSI */
	buff_offset += sprintf_rssi(buffer+buff_offset, binary_data[3]);
	break;
      case 63: /* Envelope Measurement */
	buff_offset += sprintf_envelope(buffer+buff_offset, binary_data[3],
					binary_data[4]);
	break;
      default: /* EM */
	buff_offset += sprintf_em(buffer+buff_offset, binary_data[0],
				  binary_data[3], binary_data[4]);
    }

    /* Max Size: 180 */
  }

  /* Pad the object out to it's full length */
  while (buff_offset < (JSON_RECORD_CHARS-1)) {
    *(buffer+buff_offset) = ' ';
    buff_offset++;
  }

  /* Finish the object */
  buff_offset += sprintf(buffer+buff_offset, "}");

  /* At this point buff_offset should always equal JSON_RECORD_CHARS */

  if (comma) {
    buff_offset += sprintf(buffer+buff_offset, ",");
  }

  return buff_offset;
}
/**
 * Returns a pointer to a buffer containing a JSON footer. Returns the number of
 * bytes written.
 */
int json_footer(char* buffer) {
  return sprintf(buffer, "]}");
}
