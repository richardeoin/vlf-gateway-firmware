/* 
 * Clock functions
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

#define PLLE0		(1<<24)	/* Flag for when the PLL is enabled */
#define PLLC0		(1<<25)	/* Flag for when the PLL is connected */
#define PLOCK0		(1<<26)	/* Flag for when the PLL is locked */

/**
 * Switches to the stable 1MHz clock from the AT86RF212
 */
void switch_to_stable_clock(void) {
  /* Enable the Main Oscillator */
  LPC_SC->SCS |= 0x0020;

  /* Wait for it to stabilise */
  while ((LPC_SC->SCS & 0x0040) == 0);

  /* See User Manual §4.5.13 for details on the PLL0 setup sequence */

  /* Disconnect the PLL */
  LPC_SC->PLL0CON = 0x01;
  LPC_SC->PLL0FEED = 0xAA;
  LPC_SC->PLL0FEED = 0x55;

  /* Disable the PLL */
  LPC_SC->PLL0CON = 0x00;
  LPC_SC->PLL0FEED = 0xAA;
  LPC_SC->PLL0FEED = 0x55;

  /* Divide pllclk by 3 to get the CPU clock */
  LPC_SC->CCLKCFG = 0x02;

  /* Select Main Oscillator as Clock Source for PLL0 */
  LPC_SC->CLKSRCSEL = 0x00000001;

  /* Configure PLL0 */
  LPC_SC->PLL0CFG = 0x0001012B; /* 300 MHz FCCO for a 1 MHz clock */
  LPC_SC->PLL0FEED = 0xAA;
  LPC_SC->PLL0FEED = 0x55;

  /* PLL0 Enable */
  LPC_SC->PLL0CON = 0x01;
  LPC_SC->PLL0FEED = 0xAA;
  LPC_SC->PLL0FEED = 0x55;
  while (!(LPC_SC->PLL0STAT & PLOCK0)); /* Wait for the PLL to lock */

  /* PLL0 Enable & Connect */
  LPC_SC->PLL0CON = 0x03;
  LPC_SC->PLL0FEED = 0xAA;
  LPC_SC->PLL0FEED = 0x55;
  /* Wait for the PLL to be enabled and connected */
  while (!(LPC_SC->PLL0STAT & (PLLE0 | PLLC0)));

  /* The core should now be running at 100MHz on a 1MHz clock */
}
