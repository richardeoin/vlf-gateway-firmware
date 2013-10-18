/* 
 * I2C Master, intended for retreiving MAC address from I2C slave
 * Copyright (C) 2012 Richard Meadows
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

#ifndef __I2C_H 
#define __I2C_H

volatile uint8_t I2CMode;	/* 0 = write, 1 = read, 2 = ping */
volatile int8_t I2CDone;	/* 0 = working, 1 = done */

volatile uint8_t I2CSlave;
volatile uint8_t I2CRegister;

volatile uint8_t* I2CData;
volatile uint8_t I2CDataValue;
volatile uint32_t I2CLen;
volatile uint32_t I2CIndex;

#define I2CMAX_TIMEOUT		0x00FFFFFF
#define RD_BIT			0x01

/* I2C Control Set & Clear Registers */
#define I2C_I2EN		(1<<6)
#define I2C_STA			(1<<5) /* AKA STAC */
#define I2C_STO			(1<<4)
#define I2C_SI			(1<<3) /* AKA SIC */
#define I2C_AA			(1<<2) /* AKA AAC */

/* Setup for just a tad under 100kHz @ PCLK = 25MHz */
#define I2SCLHVAL		0x80; /* I2C SCL Duty Cycle High Reg */
#define I2SCLLVAL		0x80; /* I2C SCL Duty Cycle Low Reg */

void WriteI2C(uint8_t sl, uint8_t sl_reg, uint8_t data);
/* The data array must be preserved until the transaction is flagged as complete! */
void WriteI2CBlock(uint8_t sl, uint8_t sl_reg, uint8_t* data, uint32_t len);

int8_t PingI2C(uint8_t sl);

int16_t ReadI2C(uint8_t sl, uint8_t sl_reg);
void ReadI2CBlock(uint8_t sl, uint8_t sl_reg, uint8_t* data, uint32_t len);

int8_t WaitForI2C();
extern void I2C0_IRQHandler(void);
uint32_t InitI2C();
uint32_t ShutdownI2C();

#endif

