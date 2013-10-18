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

#include "LPC17xx.h"
#include "memory/sd_spi.h"
#include "memory/memory.h"
#include "memory/checksum.h"
#include "memory/sd.h"

#include "console.h"

#define NULL (void*)0

#define INDEXES_LEN 32
uint8_t indexes_buf[INDEXES_LEN];

uint8_t get_memory_indexes(struct memory_indexes* indexes) {
  /* If the indexes are invalid they will be set to the value 0xFFFFFFFF */

  /* Read the first page of the memory */
  disk_read(indexes_buf, INDEXES_LEN, 0);

  uint32_t* words = (uint32_t*)indexes_buf;

  indexes->read_index = words[0];
  indexes->write_index = words[1];

  uint16_t i;
  for (i = 2; i < INDEXES_LEN/4; i+=2) {
    if (words[i] != indexes->read_index+i || /* If later copies of the indexes don't match */
	words[i+1] != indexes->write_index+i) {
      indexes->read_index = indexes->write_index = 0xFFFFFFFF; /* Invalid indexes */
      return 0;
    }
  }

  return 1;
}
uint8_t put_memory_indexes(struct memory_indexes* indexes) {
  uint16_t i;

  uint32_t* words = (uint32_t*)indexes_buf;

  /* Copy the indexes lots of times through the page */
  for (i = 0; i < INDEXES_LEN/4; i+=2) {
    words[i] = indexes->read_index+i;
    words[i+1] = indexes->write_index+i;
  }

  /* Write the first page of the memory */
  disk_write(indexes_buf, INDEXES_LEN, 0);

  return 1;
}

/* ======== Block Control ======== */

/**
 * Returns 1 if the block is valid, 0 is it isn't.
 */
uint8_t is_block_valid(uint32_t block) {
  return (block == 0x00000000 ||	/* Reserved for the indexes */
	  block > 0x007FFFFF ||		/* Outside the 32-bit address space */
	  block > disk_sectors()-1) ? 0 : 1;	/* Beyond the size of the disk */
}
/**
 * Returns the address of the first valid block following `current_block`.
 */
uint32_t next_block(uint32_t current_block) {
  uint32_t next = current_block+1;

  if (!is_block_valid(next)) {
    next = 0x00000001; /* Goto the first block */
  }

  return next;
}

/* ======== Reading and Writing ======== */

uint32_t last_write_time;

/**
 * Returns the record at the given index.
 */
uint32_t* get_sample(uint32_t index) {
  /* Read the page */
  if (disk_read((uint8_t*)mem_buf, MEMORY_RECORD_SIZE, index)) {
    /* Disk Read Error */
    return NULL;
  }
  if (evaluate_checksum((uint8_t*)mem_buf) == CHECKSUM_FAIL) {
    /* Bad Checksum */
    return NULL;
  }

  return mem_buf;
}
/**
 * Returns the current read index.
 */
uint32_t get_current_read_block(void) {
  return memory_indexes.read_index;
}
/**
 * Returns the number of blocks that are available to be read from memory.
 */
uint16_t get_blocks_to_read(void) {
  int32_t blocks = memory_indexes.write_index - memory_indexes.read_index;

  if (blocks < 0) {
    /* There are no blocks available */
    return 0;
  }

  /* If we wrote to memory during the last 3 seconds */
  if (LPC_TIM3->TC - last_write_time < 3) {
    /* There are no blocks available */
    return 0;
  }

  /* Only 1000 blocks can be available at once */
  return (blocks < 1000) ? blocks : 1000;
}
/**
 * Puts a record into memory.
 */
uint8_t put_sample(uint8_t* block) {
  uint32_t next = next_block(memory_indexes.write_index);

  if (next != memory_indexes.read_index) { /* If we're not about to overwrite valid data */
    /* Write to disk */
    disk_write(block, MEMORY_RECORD_SIZE, next);

    /* Update the indexes */
    memory_indexes.write_index = next;

    /* Set the last write time to now */
    last_write_time = LPC_TIM3->TC;

    if ((next & 0x7) == 0) { /* Every eighth address */
      put_memory_indexes(&memory_indexes); /* Write the indexes */
    }

    return 1;
  } else { /* We've hit the location where data is being written out */
    return 0;
  }
}
/* ======== Upload Done ======== */

/**
 * Called when a successful upload of a block has occurred.
 */
void good_upload_done(void) {
  /* Move the index forward one block */
  memory_indexes.read_index = next_block(memory_indexes.read_index);

  if ((memory_indexes.read_index & 0x7) == 0) { /* Every eighth address */
    /* Write the memory indexes to disk */
    put_memory_indexes(&memory_indexes);
  }
}
/**
 * Called when an upload of a block has failed.
 */
void bad_upload_done(void) {
  /* Read the block in question */
  if (disk_read((uint8_t*)mem_buf, MEMORY_RECORD_SIZE, memory_indexes.read_index)) {
    return; /* Fail */
  }

  /* TODO: Evaluate if the checksum on the block itself is okay */
  /* If it is, re-write the block at the current memory address */
  //	uint32_t checksum = get_checksum(mem_buf+1);

  /* Move the indexes forward as if the upload had been successful */
  good_upload_done();
}

/* ======== Initialisation ======== */
uint8_t memory_init(void) {
  /* Set up the SD card */
  SPI_Init();
  disk_initialize();

  /* Last Write Time */
  last_write_time = 0;

  /* Init indexes */
  memory_indexes.read_index = memory_indexes.write_index = 0xFFFFFFFF;

  /* Get the memory indexes we need */
  get_memory_indexes(&memory_indexes);

  /* Optionally reset the indexes */
  //memory_indexes.read_index = memory_indexes.write_index = 0;
  //put_memory_indexes(&memory_indexes);

  return 1;
}
