/* 
 * Loads the MAC address from an external EEPROM
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
#include "netif/mac_addr_i2c.h"

/**
 * Microchip 24AA02E48T
 * http://ww1.microchip.com/downloads/en/DeviceDoc/22124D.pdf
 */
#define EEPROM_ADDR	0xAE
#define MAC_LOCATION	0xFA

/**
 * Loads the MAC Address from an external EEPROM.
 */
int8_t get_macaddr(uint8_t *macaddr) {
  InitI2C(); /* Initialise the interface */

  /* Read the MAC address */
  ReadI2CBlock(EEPROM_ADDR, MAC_LOCATION, macaddr, 6);

  /* Wait for the asynchronous function to finish */
  if (WaitForI2C() >= 0) { /* Successful */
    ShutdownI2C(); /* Shutdown the interface to save power */
    return 1;
  } else { /* I2C transfer failed */
    return -1;
  }
}
