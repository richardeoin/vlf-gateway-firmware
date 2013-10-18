/*
 * time.c
 *
 *  Created on: 23 Aug 2012
 *      Author: richard
 */

#include "LPC17xx.h"
/* C */
#include <time.h>
/* Time */
#include "sntp/time.h"
/* Console */
#include "console.h"

uint64_t current_time;
uint8_t time_valid;

static uint64_t get_last_recorded_time() {
	// TODO

	return 0;
}
static void set_last_recorded_time(uint64_t time) {
	// TODO
}


/* Sets the current time using the number of seconds and microseconds since the start of the last
 * epoch (1970.0 + n(2^32secs)). Also sets the time on the EEPROM.
 */
void ntp_set_current_time(uint32_t secs, uint32_t us) {
	uint32_t* ct_ptr = (uint32_t*)&current_time;

	/* Get the last recorded time from the EEPROM */
	uint64_t last_recorded_time = get_last_recorded_time();
	uint32_t* lr_ptr = (uint32_t*)&last_recorded_time;

	/* If the NTP time is before the first 32-bits of the last_recorded one....
	 *
	 * We've crossed an epoch boundary since we last recorded time!
	 * Or gone back in time, but the 2nd law of thermodynamics has a problem with this...
	 * NOTE: This strategy relies on NTP always being accurate!!!! TODO Fix/Mitigate
	 */
	if (secs < lr_ptr[0]) {
		ct_ptr[1] = lr_ptr[1] + 1;
	} else {
		ct_ptr[1] = lr_ptr[1];
	}

	/* The lowest 32-bits are just what NTP gave us */
	ct_ptr[0] = secs;

	/* Load the timer with the current time */
	LPC_TIM3->PC = 25*us;
	LPC_TIM3->TC = secs;

	/* The time is now valid */
	time_valid = 1;

	/* Set this new time in EEPROM */
	set_last_recorded_time(current_time);
}

/* Returns the current 64-bit time (Epoch 1970.0) */
uint64_t get_current_time(void) {
	uint32_t* ct_ptr = (uint32_t*)&current_time;

	/* Get the current time from the timer */
	uint32_t timer_val = LPC_TIM3->TC;

	/* If the value in the timer is less that what out stored current_time is...
	 * Then we've crossed an epoch!
	 */
	if (timer_val < ct_ptr[0]) {
		ct_ptr[1]++;
	}
	/* Bring our value up to speed */
	ct_ptr[0] = timer_val;

	return current_time;
}
/* Returns a boolean that indicates if the current time is valid */
uint8_t is_time_valid(void) {
	return time_valid;
}

void print_current_time() {
	/* Bring the current time up to speed */
	uint64_t time = get_current_time();
	uint32_t* time_ptr = (uint32_t*)&time;

	/* Put the lower 32-bits into a time_t object */
	time_t t = time_ptr[0]; /* Little endian */

	/* And print.. */
	console_printf("Gateway time is %s\n", ctime(&t));
}

/* Starts this internal timer and sets values */
void init_current_time() {
	time_valid = 0;

	LPC_SC->PCONP |= (1<<23); /* Power up Timer 3, CCLK/4 */

	/* Put the timer into reset */
	LPC_TIM3->TCR = 0x02;

	/* Set the Prescale Register so that the TC is incremented at 1Hz */
	LPC_TIM3->PR = (25*1000*1000)-1;

	/* Take the timer out of reset */
	LPC_TIM3->TCR = 0x01;
}
