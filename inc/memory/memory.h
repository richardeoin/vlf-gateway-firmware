/* 
 * Manages our internal memory
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

#ifndef MEMORY_H
#define MEMORY_H

#include "LPC17xx.h"

/**
 * Each sample is stored in a 0x200 byte block on the SD card. This is
 * one 512-byte page.  Hence just over eight million samples can be
 * stored in the full 32-bit address space.
 */

/**
 * Must be a multiple of 4 so a 32-bit word can be used.
 */
enum {
  MEMORY_RECORD_SIZE	= 24
};
/**
 * The size of the SD card in bytes (-1 encoded)
 */
enum {
  SD_SIZE = 0x3FFFFFFF /* 1 gigabyte */
};

uint32_t mem_buf[MEMORY_RECORD_SIZE/4];

struct memory_indexes {
  uint32_t write_index;
  uint32_t read_index;
} memory_indexes;

uint8_t get_memory_indexes(struct memory_indexes* indexes);
uint8_t put_memory_indexes(struct memory_indexes* indexes);

uint8_t invalidate_block(uint32_t address);
uint32_t next_block(uint32_t current_address);

uint32_t* get_sample(uint32_t index);
uint32_t get_current_read_block(void);
uint16_t get_blocks_to_read(void);
uint8_t put_sample(uint8_t* block);

void good_upload_done(void);
void bad_upload_done(void);

uint8_t memory_init(void);

#endif /* MEMORY_H */
