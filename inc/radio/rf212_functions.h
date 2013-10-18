/* 
 * Functions for controlling the radio hardware
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

#ifndef RF212_FUNCTIONS_H
#define RF212_FUNCTIONS_H

/* -------- Defines -------- */

/* The SPI block used */
#define RF212_SPI_NUM			0

#if RF212_SPI_NUM == 0
#define RF212_SPI_BLOCK		LPC_SSP0
#else
#define	 RF212_SPI_BLOCK	LPC_SSP1
#endif

/* The slave select pin - P0[16] */
#define RF212_SSEL_PORT		LPC_GPIO0
#define RF212_SSEL_PIN		16

/* The reset pin - P2[5] */
#define RF212_RESET_PORT	LPC_GPIO2
#define RF212_RESET_PIN		1

/* The sleep trigger pin - P2[6] */
#define RF212_SLPTR_PORT	LPC_GPIO2
#define RF212_SLPTR_PIN		2

/* NOTE: The interrupt can be configured at the bottom of rf212_functions.c */

#define FIFOSIZE 8

/* SSP Status register */
#define SSPSR_TFE       (0x1<<0)
#define SSPSR_TNF       (0x1<<1)
#define SSPSR_RNE       (0x1<<2)
#define SSPSR_RFF       (0x1<<3)
#define SSPSR_BSY       (0x1<<4)

/* SSP CR0 register */
#define SSPCR0_DSS      (0x1<<0)
#define SSPCR0_FRF      (0x1<<4)
#define SSPCR0_SPO      (0x1<<6)
#define SSPCR0_SPH      (0x1<<7)
#define SSPCR0_SCR      (0x1<<8)

/* SSP CR1 register */
#define SSPCR1_LBM      (0x1<<0)
#define SSPCR1_SSE      (0x1<<1)
#define SSPCR1_MS       (0x1<<2)
#define SSPCR1_SOD      (0x1<<3)

/* SSP Interrupt Mask Set/Clear register */
#define SSPIMSC_RORIM   (0x1<<0)
#define SSPIMSC_RTIM    (0x1<<1)
#define SSPIMSC_RXIM    (0x1<<2)
#define SSPIMSC_TXIM    (0x1<<3)

/* SSP0 Interrupt Status register */
#define SSPRIS_RORRIS   (0x1<<0)
#define SSPRIS_RTRIS    (0x1<<1)
#define SSPRIS_RXRIS    (0x1<<2)
#define SSPRIS_TXRIS    (0x1<<3)

/* SSP0 Masked Interrupt register */
#define SSPMIS_RORMIS   (0x1<<0)
#define SSPMIS_RTMIS    (0x1<<1)
#define SSPMIS_RXMIS    (0x1<<2)
#define SSPMIS_TXMIS    (0x1<<3)

/* SSP0 Interrupt clear register */
#define SSPICR_RORIC    (0x1<<0)
#define SSPICR_RTIC     (0x1<<1)

/* -------- Pins -------- */
void rf212_reset_enable();
void rf212_reset_disable();
void rf212_slptr_enable();
void rf212_slptr_disable();
void rf212_spi_enable();
void rf212_spi_disable();
/* -------- SPI -------- */
uint8_t rf212_xfer(uint8_t data);
void rf212_spi_init();
/* -------- Initialisation -------- */
void rf212_io_init();
/* -------- Timer -------- */
void rf212_delay_us(uint32_t us);
/* -------- High Priority Interrupt -------- */
void rf212_trigger_interrupt(void);

#endif /* RF212_FUNCTIONS_H */
