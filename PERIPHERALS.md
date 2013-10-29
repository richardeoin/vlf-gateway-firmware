This file summarises the usage of the various on-chip peripherals.

## Timers

 * Timer 0: Delays for radio
 * Timer 1: Activity LED
 * Timer 2: Triggering Radio IRQ
 * Timer 3: Timekeeping

## Interrupt Priorities

 * 0:	I2C0_IRQn - Reading MAC Address. Unable to start correctly without this
 * 5:	EINT1_IRQn - Radio-triggered Radio Handler. Responds to radio events
 *  	TIMER2_IRQn - Software-triggered Radio Handler. Responds to software
 manipulation of radio.
 * 30: 	TIMER1_IRQn - Flashing LED on network port
 * 31:	RIT_IRQn - Main Processing Loop
