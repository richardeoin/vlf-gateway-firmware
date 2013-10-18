/* 
 * Sets up an interface to the AT86RF212 radio
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
#include "radio/rf212.h"
#include "radio/rf212_functions.h"
#include "radio.h"
#include "radio_irq.h"
#include "debug.h"

void rf212_init(rx_callback_func callback) {
  /* Setup the interface struct */
  radif_init_struct(&rf212_radif);

  /* Connect up the various hardware functions the interface needs to operate */
  rf212_radif.spi_start = &rf212_spi_enable;
  rf212_radif.spi_xfer = rf212_xfer;
  rf212_radif.spi_stop = rf212_spi_disable;
  rf212_radif.slptr_set = rf212_slptr_enable;
  rf212_radif.slptr_clear = rf212_slptr_disable;
  rf212_radif.reset_set = rf212_reset_enable;
  rf212_radif.reset_clear = rf212_reset_disable;
  rf212_radif.delay_us = rf212_delay_us;
  rf212_radif.interrupt_trigger = &rf212_trigger_interrupt;
  /* Set radio properties */
  rf212_radif.auto_crc_gen = 0xFF;
  rf212_radif.clkm_config = 0x19;
  rf212_radif.freq = 8683;
  rf212_radif.power = 0xe8;
  rf212_radif.modulation = RADIF_OQPSK_400KCHIPS_200KBITS_S;
  rf212_radif.pan_id = 0x1234;
  rf212_radif.short_address = 0x0001; /* The base station has address 1 */
  /* Set the receive callback */
  rf212_radif.rx_callback = callback;

  /* Initialise our hardware interface */
  rf212_io_init();

  /* Initialise the radio */
  radif_command(RADIF_RESET, &rf212_radif);

  /* Setup various parameters */
  radif_command(RADIF_SET_MODULATION, &rf212_radif);
  radif_command(RADIF_SET_FREQ, &rf212_radif);
  radif_command(RADIF_SET_POWER, &rf212_radif);
  radif_command(RADIF_SET_ADDRESS, &rf212_radif);

  /* Make the radio interface operational */
  radif_command(RADIF_STARTUP, &rf212_radif);
}
void rf212_service() {
  radif_service(&rf212_radif);
}
void TIMER2_IRQHandler(void) {
  LPC_TIM2->IR |= 0x3F; /* Clear all the timer interrupts */

  NVIC_DisableIRQ(TIMER2_IRQn);

  LPC_TIM2->TCR = 0x2; /* Put the counter back into reset */
  LPC_SC->PCONP &= ~(1<<22); /* Power down Timer 2 */

  /* Service the interrupt */
  radio_irq(&rf212_radif);
}

/**
 * Radio IRQ
 */
void EINT1_IRQHandler(void) {
  /* Clear the interrupt  */
  LPC_SC->EXTINT |= (1<<1);
  /* And service it */
  radio_irq(&rf212_radif);
}
