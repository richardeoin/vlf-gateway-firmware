/* 
 * Checks that we can write to the SD card successfully
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
#include "memory/sd.h"
#include "memory/sd_spi.h"
#include "memory/sd_test.h"
#include "debug.h"

#define BUFF_1_VAL	0x55
#define BUFF_2_VAL	0xAA

void SD_Test(void) {
  /* Set up the SD card */
  SPI_Init();
  disk_initialize();

  /* Init some buffers */
  uint8_t buff1[512], buff2[512], buff_rd[512];
  uint16_t i;
  for (i = 0; i < 512; i++) { buff1[i] = BUFF_1_VAL; buff2[i] = BUFF_2_VAL; }

  debug_puts("Starting SD Test!\n");

  /* Forever! */
  uint8_t val;

  for (cycles = 0; 1; cycles++) {
    /* Hammer thge first sector with alternating values */
    if (cycles % 2 == 0) {
      disk_write(buff1, 512, 0x0); val = BUFF_1_VAL;
    } else {
      disk_write(buff2, 512, 0x0); val = BUFF_2_VAL;
    }

    /* Read it back */
    disk_read(buff_rd, 512, 0x0);

    /* Flag up any discrepancies in the debug window */
    for (i = 0; i < 512; i++) {
      if (buff_rd[i] != val) {
      	debug_printf("Fail! Cycle: %d Index: %d Wrote: %d Read: %d\n",
		     cycles, i, val, buff_rd[i]);
      }
    }

    /* Remind the user where we are from time to time */
    if (cycles % 512 == 0) {
      debug_printf("Completed Cycle %d\n", cycles);
    }
  }
}
