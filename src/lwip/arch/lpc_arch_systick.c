#include "LPC17xx.h"

static volatile uint32_t msTicks;

void SysTick_Enable(void) {
  /* Setup SysTick Timer for 1 msec interrupts */
  if (SysTick_Config(SystemCoreClock / 1000)) {
    while (1); /* Capture error */
  }
}
void SysTick_Handler(void) {
  msTicks++;
}
uint32_t SysTick_GetMS(void) {
  return msTicks;
}

void msDelay(uint32_t dlyTicks) {
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks);
}
