/*
 * time.h
 *
 *  Created on: 23 Aug 2012
 *      Author: richard
 */

#ifndef TIME_H_
#define TIME_H_

#include "LPC17xx.h"

/* We keep a roughly accurate 64-bit representation of the current
 * time in our external EEPROM so that we can identify the current
 * 64-bit time from the 32-bit time.  If the EEPROM is to be
 * programmed for the first time on or after 7th February 2106 then
 * some additional code will need to be written to program the EEPROM
 * with the correct value
 */

/* Sets the current time using the number of seconds and microseconds since the start of the last NTP
 * epoch (1970.0 + n(2^32secs)). Also sets the time on the EEPROM.
 */
void ntp_set_current_time(uint32_t secs, uint32_t us);

/* Returns the current 64-bit time (Epoch 1970.0) */
uint64_t get_current_time();
uint8_t is_time_valid();
void print_current_time();

/* Starts this internal timer and sets values */
void init_current_time();

#endif /* TIME_H_ */
