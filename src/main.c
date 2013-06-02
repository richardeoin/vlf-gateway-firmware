/**
 * Timer usage
 *
 * Timer 0: Delays for radio
 * Timer 1: Activity LED
 * Timer 2: Triggering Radio IRQ
 * Timer 3: Timekeeping
 *
 * RIT: Regulating Processing Loop
 *
 */

/**
 * Interrupt Priorities
 *
 * 0:	I2C0_IRQn - Reading MAC Address. Unable to start correctly without this
 * 5:	EINT1_IRQn - Radio-triggered Radio Handler. Responds to radio events
 * 	TIMER2_IRQn - Software-triggered Radio Handler. Responds to software manipulation of radio
 * 30: 	TIMER1_IRQn - Flashing LED on network port
 * 31:	RIT_IRQn - Main Processing Loop
 */

#include "LPC17xx.h"

#include <string.h>
#include "init_service.h"
#include "sntp/time.h"
#include "debug.h"
#include "memory/sd_test.h"
#include "arch/lpc_arch.h"

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
  LPC_RIT->RICTRL = (1<<3)|(0<<2)|(1<<1); /* Enable RIT, Halt on Debug, Clear on match */

  /* Setup sleep mode */
  LPC_SC->PCON &= ~0x3; /* Sleep or Deep Sleep mode */
  SCB->SCR &= ~(1<<2); /* Sleep mode */

  /* Sleep forever */
  while(1) {
    __WFI();
  }

  return 0;
}
