/* 
 * Functions for controlling the radio hardware
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
#include "radio.h"
#include "radio/rf212.h"
#include "radio/rf212_functions.h"

/* -------- Pins -------- */

/* Reset: Active Low */
void rf212_reset_enable() {
  RF212_RESET_PORT->FIOCLR = (1 << RF212_RESET_PIN);
}
void rf212_reset_disable() {
  RF212_RESET_PORT->FIOSET = (1 << RF212_RESET_PIN);
}
/* Sleep Trigger: Active High */
void rf212_slptr_enable() {
  RF212_SLPTR_PORT->FIOSET = (1 << RF212_SLPTR_PIN);
}
void rf212_slptr_disable() {
  RF212_SLPTR_PORT->FIOCLR = (1 << RF212_SLPTR_PIN);
}
/* Slave Select: Active Low */
void rf212_spi_enable() {
  RF212_SSEL_PORT->FIOCLR = (1 << RF212_SSEL_PIN);
}
void rf212_spi_disable() {
  RF212_SSEL_PORT->FIOSET = (1 << RF212_SSEL_PIN);
}

/* -------- SPI -------- */

uint8_t rf212_xfer(uint8_t data) { /* Actually performs an SPI transfer */
  RF212_SPI_BLOCK->DR = data;

  /* Wait until the Busy bit is cleared */
  while ((RF212_SPI_BLOCK->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE);

  return RF212_SPI_BLOCK->DR;
}
void rf212_spi_init() { /* Sets up the SPI port for the radio */
  uint8_t i, Dummy=Dummy;

  /* Power up the port */
#if RF212_SPI_NUM == 0
  LPC_SC->PCONP |= (1 << 21);
#else
  LPC_SC->PCONP |= (1 << 10);
#endif

  /* Send it a clock = The main clock */
#if RF212_SPI_NUM == 0
  LPC_SC->PCLKSEL0 |= (1 << 10);
#else
  LPC_SC->PCLKSEL0 |= (1 << 20);
#endif

  /*  SSP I/O configuration */
#if RF212_SPI_NUM == 0
  LPC_PINCON->PINSEL0 &= ~(3 << 30);	/* P0[15] = SSP0 CLK */
  LPC_PINCON->PINSEL0 |= (2 << 30);
  LPC_PINCON->PINSEL1 &= ~(3 << 2);	/* P0[17] = SSP0 MISO */
  LPC_PINCON->PINSEL1 |= (2 << 2);
  LPC_PINCON->PINSEL1 &= ~(3 << 4);	/* P0[18] = SSP0 MOSI */
  LPC_PINCON->PINSEL1 |= (2 << 4);
#else
  LPC_PINCON->PINSEL0 &= ~(3 << 14);	/* P0[7] = SSP1 CLK */
  LPC_PINCON->PINSEL0 |= (2 << 14);
  LPC_PINCON->PINSEL0 &= ~(3 << 16);	/* P0[8] = SSP1 MISO */
  LPC_PINCON->PINSEL0 |= (2 << 16);
  LPC_PINCON->PINSEL0 &= ~(3 << 18);	/* P0[9] = SSP1 MOSI */
  LPC_PINCON->PINSEL0 |= (2 << 18);
#endif

  /* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 8-1 */
  RF212_SPI_BLOCK->CR0 = 0x0707;

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  RF212_SPI_BLOCK->CPSR = 0x2; /* This results in a MCLK of PCLK/16 = 100/16 = 6.25MHz */

  for (i = 0; i < FIFOSIZE; i++) {
    Dummy = RF212_SPI_BLOCK->DR;		/* Clear the RxFIFO */
  }

  /* Device select as master, SSP Enabled */
  RF212_SPI_BLOCK->CR1 = SSPCR1_SSE; /* Master mode */

  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  RF212_SPI_BLOCK->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;

  rf212_spi_disable();
}

/* -------- Initialisation -------- */

void rf212_io_init() {
  /* Configure IOs as outputs */
  RF212_RESET_PORT->FIODIR |= (1 << RF212_RESET_PIN);	/* Reset */
  RF212_SLPTR_PORT->FIODIR |= (1 << RF212_SLPTR_PIN);	/* Sleep Trigger */
  RF212_SSEL_PORT->FIODIR |= (1 << RF212_SSEL_PIN);	/* Slave Select */

  /* Configure SPI for AT86RF212 access */
  rf212_spi_init();

  /* The interrupt pin is P2[11] / EINT1  */
  LPC_PINCON->PINSEL4 |= (1 << 22); /* Mode EINT1 */

  LPC_SC->EXTMODE |= (1<<1); /* Level Sensitive */
  LPC_SC->EXTPOLAR |= (1<<1); /* Rising Edge */
  LPC_SC->EXTINT |= (1<<1); /* Clear the interrupt */

  NVIC_SetPriority(EINT1_IRQn, 5);
  NVIC_EnableIRQ(EINT1_IRQn);
}

/* -------- Timer -------- */

void rf212_delay_us(uint32_t us) {
  LPC_SC->PCONP |= (1<<1); /* Power up Timer 0, CCLK/4 */

  LPC_TIM0->TCR = 0x2; /* Put the counter into reset */
  LPC_TIM0->PR = 25; /* 1µs on a 100MHz clock */
  LPC_TIM0->MR0 = us;
  LPC_TIM0->MCR |= (1<<0)|(1<<2); /* Interrupt and stop on MR0 */
  LPC_TIM0->IR |= 0x3F; /* Clear all the timer interrupts */
  LPC_TIM0->TCR = 0x1; /* Start the counter */

  while ((LPC_TIM0->IR & 0x1) == 0);

  LPC_TIM0->TCR = 0x2; /* Put the counter back into reset */
  LPC_SC->PCONP &= ~(1<<1); /* Power down Timer 0 */
}

/* -------- High Priority Interrupt -------- */

void rf212_trigger_interrupt(void) {
  LPC_SC->PCONP |= (1<<22); /* Power up Timer 2, CCLK/4 */

  LPC_TIM2->TCR = 0x2; /* Put the counter into reset */
  LPC_TIM2->PR = 1;
  LPC_TIM2->MR0 = 1;
  LPC_TIM2->MCR |= (1<<0)|(1<<2); /* Interrupt and stop on MR0 */
  LPC_TIM2->IR |= 0x3F; /* Clear all the timer interrupts */

  NVIC_SetPriority(TIMER2_IRQn, 5);
  NVIC_EnableIRQ(TIMER2_IRQn);

  LPC_TIM2->TCR = 0x1; /* Start the counter */
}
