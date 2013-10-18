/* 
 * Routines for the SPI perhiperal
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
#include "debug.h"

/* SSP 1 */
void SPI_Init(void) {
  uint8_t i, Dummy=Dummy;

  // Power up the port
  LPC_SC->PCONP |= (1 << 10);
  // Send it a clock = The main clock
  LPC_SC->PCLKSEL0 |= (1 << 20);

  /*  SSP I/O configuration */
  LPC_PINCON->PINSEL0 &= ~(3 << 14); /* P0[7]: SSP1 CLK */
  LPC_PINCON->PINSEL0 |= (2 << 14);
  LPC_PINCON->PINSEL0 &= ~(3 << 16); /* P0[8]: SSP1 MISO */
  LPC_PINCON->PINSEL0 |= (2 << 16);
  LPC_PINCON->PINSEL0 &= ~(3 << 18); /* P0[9]: SSP1 MOSI */
  LPC_PINCON->PINSEL0 |= (2 << 18);
  /* SSEL: P0[6], Active Low */
  LPC_PINCON->PINSEL0	&= ~(3 << 12); /* GPIO */
  LPC_GPIO0->FIODIR |= (1 << 6);

  /**
   * Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and
   * SCR is 7 (Divide by 8)
   */
  LPC_SSP1->CR0 = 0x0707;

  /* SSPCPSR clock pre-scale register, master mode, minimum divisor is 0x02 */
  LPC_SSP1->CPSR = 0x2;

  for (i = 0; i < FIFOSIZE; i++) {
    Dummy = LPC_SSP1->DR;		/* Clear the RxFIFO */
  }

  /* Enable the SSP Interrupt */
  NVIC_EnableIRQ(SSP1_IRQn);

  /* Device select as master, SSP Enabled */
  LPC_SSP1->CR1 = SSPCR1_SSE; /* Master mode */

  /* Set SSPINMS registers to enable interrupts */
  /* Enable all error related interrupts */
  LPC_SSP1->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;

  SD_SPI_DISABLE();
}

uint8_t SPI_Write(uint8_t data) {
  LPC_SSP1->DR = data;

  /* Wait until the Busy bit is cleared */
  while ((LPC_SSP1->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE);

  return LPC_SSP1->DR;
}

void SPI_Frequency(uint32_t frequency) {
  /* Assuming 100MHz clock into the SSP Module. Only *even* values for CPSR!! */

  if (frequency == 1000000) { // 1MHz
    /* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 1 */
    LPC_SSP1->CR0 = 0x0107;
    /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
    LPC_SSP1->CPSR = 50; // PCLK / (50 * [1+1])
  } else if (frequency == 100000) { // 100kHz
    /* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 3 */
    LPC_SSP1->CR0 = 0x0307;
    /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
    LPC_SSP1->CPSR = 250; // PCLK / (250 * [3+1])
  } else {
    debug_puts("Bad SD SPI Frequency!\n");
  }
}

// TODO - Sort this out!
void SSP1_IRQHandler(void)  {
  uint32_t regValue;

  regValue = LPC_SSP1->MIS;

  return;
}
