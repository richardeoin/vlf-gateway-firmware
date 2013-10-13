/* 
 * System Entry Point. Initialisation and Repeating Events
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

#include <string.h>
#include "init_service.h"
#include "sntp/time.h"
#include "debug.h"
#include "memory/sd_test.h"
#include "arch/lpc_arch.h"
#include "leds.h"

void RIT_IRQHandler(void) {
  LPC_RIT->RICTRL |= 1; /* Clear the interrupt */

  /* Run applications */
  net_service();
  radio_service();
}

int main(void) {
  SystemInit();

  debug_puts("Happy Face!\n");

  SystemCoreClockUpdate();
  uint32_t clock = SystemCoreClock;

  INIT_LEDS();

  /* Setup the memory */
  memory_init();

  /* Start applications */
  net_init();
  radio_init();
  init_current_time();

  /* Switch to a stable clock source from the radio */
  switch_to_stable_clock();

  /* Setup the Repetitive Interrupt Timer (RIT) */
  LPC_SC->PCONP |= (1 << 16); /* Power up the RIT, PCLK=CCLK/4 */
  LPC_RIT->RICTRL = 1; /* Disable RIT, Clear interrupt */
  NVIC_SetPriority(RIT_IRQn, 31); /* Set Lowest Priority */
  NVIC_EnableIRQ(RIT_IRQn); /* Enable the interrupt */
  LPC_RIT->RIMASK = 0; /* Compare all the bits */
  LPC_RIT->RICOMPVAL = 500*25; /* 500µS on 25MHz PCLK */
  /* Enable RIT, Halt on Debug, Clear on match */
  LPC_RIT->RICTRL = (1<<3)|(0<<2)|(1<<1);

  /* Setup sleep mode */
  LPC_SC->PCON &= ~0x3; /* Sleep or Deep Sleep mode */
  SCB->SCR &= ~(1<<2); /* Sleep mode */

  /* Sleep forever */
  while(1) {
    __WFI();
  }

  return 0;
}
