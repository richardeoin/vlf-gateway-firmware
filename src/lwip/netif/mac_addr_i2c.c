/* 
 * I2C Master, intended for retreiving MAC address from I2C slave
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
#include <stdlib.h>
#include "netif/mac_addr_i2c.h"

void WriteI2C(uint8_t sl, uint8_t sl_reg, uint8_t data) { /* Non-Blocking */
  WaitForI2C();

  I2CSlave = sl & 0xFE; I2CRegister = sl_reg; I2CMode = 0; I2CDone = 0;

  /* Use static memory for our data array, then the user doesn't need
   * to worry about freeing it */
  I2CData = &I2CDataValue;
  *I2CData = data; I2CLen = 1; I2CIndex = 0;

  LPC_I2C0->I2CONSET = I2C_STA;	/* Set Start flag */
}
/* The data array must be preserved until the transaction is flagged as complete! */
void WriteI2CBlock(uint8_t sl, uint8_t sl_reg, uint8_t* data, uint32_t len) { /* Non-Blocking */
  WaitForI2C();

  I2CSlave = sl & 0xFE; I2CRegister = sl_reg; I2CMode = 0; I2CDone = 0;

  I2CData = data; I2CLen = len; I2CIndex = 0;

  LPC_I2C0->I2CONSET = I2C_STA;	/* Set Start flag */
}
int8_t PingI2C(uint8_t sl) { /* Blocking */
  WaitForI2C();

  I2CSlave = sl & 0xFE; I2CMode = 2; I2CDone = 0;

  LPC_I2C0->I2CONSET = I2C_STA;	/* Set Start flag */

  return WaitForI2C();
}
int16_t ReadI2C(uint8_t sl, uint8_t sl_reg) { /* Blocking */
  WaitForI2C();

  I2CSlave = sl & 0xFE; I2CRegister = sl_reg; I2CMode = 1; I2CDone = 0;

  /* Use static memory for our data array, then we don't need to worry
   * about freeing it */
  I2CData = &I2CDataValue;
  I2CLen = 1; I2CIndex = 0;

  LPC_I2C0->I2CONSET = I2C_STA;	/* Set Start flag */

  /* Wait for the transaction to complete */
  if (WaitForI2C() > 0) {
    uint8_t value = *I2CData;

    return (int16_t)value;
  } else {
    return -1;
  }
}
void ReadI2CBlock(uint8_t sl, uint8_t sl_reg, uint8_t* data, uint32_t len) { /* Non-Blocking */
  WaitForI2C();

  I2CSlave = sl & 0xFE; I2CRegister = sl_reg; I2CMode = 1; I2CDone = 0;

  I2CData = data; I2CLen = len; I2CIndex = 0;

  LPC_I2C0->I2CONSET = I2C_STA;	/* Set Start flag */
}

/* Blocks until any ongoing transaction completes, or a timeout occurs and it is abandoned */
int8_t WaitForI2C() {
  uint32_t i = 0;

  while (I2CDone == 0 && i++ < I2CMAX_TIMEOUT);

  if (I2CDone == 0) { // Still haven't finished...
    /* Attempt to dispatch a Stop bit */
    LPC_I2C0->I2CONSET = I2C_STO;
    I2CDone = 1;
    return -1;
  }

  return 1;
}

void I2C0_IRQHandler(void) {
  uint8_t StatValue;

  /* This handler deals with master read and master write only. */
  StatValue = LPC_I2C0->I2STAT;

  switch (StatValue) {
    case 0x08: /* A Start condition was issued. */
      /* Send Slave Address*/
      LPC_I2C0->I2DAT = I2CSlave;
      LPC_I2C0->I2CONCLR = (I2C_SI | I2C_STA);
      return;
	
    case 0x10: /* A repeated started was issued */
      /* Send Slave Address with R bit set */
      LPC_I2C0->I2DAT = I2CSlave | RD_BIT;
      LPC_I2C0->I2CONCLR = (I2C_SI | I2C_STA);
      /* We will now be switched to master receive mode */
      return;
	
    case 0x18: /* ACK following Slave Address (Write) */
      if (I2CMode == 2) { /* Ping Mode */
	LPC_I2C0->I2CONSET = I2C_STO; /* Set Stop flag */
	LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
	I2CDone = 1; return; /* We're done */
      } else { /* Read or Write Mode */
	LPC_I2C0->I2DAT = I2CRegister;
	LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
	return;
      }

    case 0x40: /* ACK following Slave Address (Read) */
      /* We're going to have a byte shoved at us whatever, we need to ACK it or not */
      if (I2CLen > 1) {
	LPC_I2C0->I2CONSET = I2C_AA; /* ACK the currently incoming byte */
      } else {
	LPC_I2C0->I2CONCLR = I2C_AA; /* NACK the currently incoming byte */
      }
      LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
      return;

    case 0x20: /* NACK following Slave Address (Write) */
    case 0x48: /* NACK following Slave Address (Read) */
      /* Can't find slave... Clear up and go home */
      LPC_I2C0->I2CONSET = I2C_STO; /* Set Stop flag */
      LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */

      I2CDone = 1; return; /* We're done */


    case 0x28: /* ACK following Data Byte (Write Mode) */
      if (I2CMode == 1) { /* Read Mode */
	LPC_I2C0->I2CONSET = I2C_STA;	/* Set Repeated-start flag */
	LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
	return;
      } else { /* Must be write mode */
	if (I2CIndex < I2CLen) {
	  LPC_I2C0->I2DAT = I2CData[I2CIndex++]; /* Write out a byte */
	  LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
	  return;
	} else { /* Time to stop */
	  LPC_I2C0->I2CONSET = I2C_STO; /* Set Stop flag */
	  LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
	  I2CDone = 1; return;
	}
      }
      break;
	
    case 0x30: /* NACK following Data Byte (Write Mode) */
      /* The slave doesn't want the write to proceed */
      LPC_I2C0->I2CONSET = I2C_STO; /* Set Stop flag */
      LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
      I2CDone = 1; return; /* We'll call this an error, might not be fatal */
	
    case 0x50: /* Data Byte Received, we sent ACK */
      I2CData[I2CIndex++] = LPC_I2C0->I2DAT;

      if (I2CIndex+1 < I2CLen) { /* Is there more to be received after this byte? */
	LPC_I2C0->I2CONSET = I2C_AA; /* ACK the currently incoming byte */
      } else {
	LPC_I2C0->I2CONCLR = I2C_AA; /* NACK the currently incoming byte */
      }
      LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
      return;
	
    case 0x58: /* Data Byte Received, we sent NACK */
      I2CData[I2CIndex++] = LPC_I2C0->I2DAT;

      LPC_I2C0->I2CONSET = I2C_STO; /* Set Stop flag */
      LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
      I2CDone = 1; return; /* All done */
	
    case 0x38: /* Arbitration lost we don't deal with multiple master situation */
    default:
      LPC_I2C0->I2CONCLR = I2C_SI; /* Clear the SI bit */
      I2CDone = 1;
      return;
  }
  return;
}

uint32_t InitI2C(void) {
  I2CDone = 1; /* Non zero so we don't wait for a non-existent transaction to complete */

  LPC_SC->PCONP |= (1 << 7); /* I2C0 */

  /* Set P0[27] and P0[28] to I2C0 SDA and SCL */
  LPC_PINCON->PINSEL1 &= ~((3 << 22) | (3 << 24));
  LPC_PINCON->PINSEL1 |= ((1 << 22) | (1 << 24));
  LPC_PINCON->I2CPADCFG &= ~(0xF); /* Standard / Fast Mode, filtering enabled */

  /* Clear flags */
  LPC_I2C0->I2CONCLR = I2C_AA | I2C_SI | I2C_STO | I2C_STA | I2C_I2EN;

  /* Set speed */
  LPC_I2C0->I2SCLL = I2SCLLVAL;
  LPC_I2C0->I2SCLH = I2SCLHVAL;

  /* Enable the I2C Interrupt */
  NVIC_SetPriority(I2C0_IRQn, 0); /* Highest possible priority */
  NVIC_EnableIRQ(I2C0_IRQn);

  LPC_I2C0->I2CONSET = I2C_I2EN;
  return 1;
}
uint32_t ShutdownI2C(void) {
  LPC_SC->PCONP &= ~(1 << 7); /* I2C0 */

  /* Set P0[27] and P0[28] to GPIO */
  LPC_PINCON->PINSEL1 &= ~((3 << 22) | (3 << 24));

  return 1;
}
