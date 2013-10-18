/* 
 * High level functions for operating the radio
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

#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>

#define NUM_TXFRAMES		8
#define NUM_RXFRAMES		8

/* ---- Function type definitions for the hardware functions we need ---- */
typedef void (*pin_set_func) (void);
typedef uint8_t (*spi_xfer_func) (uint8_t);
typedef void (*delay_us_func) (uint32_t);
typedef void (*interrupt_trigger_func) (void);

/* ---- Data transfer structures ---- */
struct rx_frame {
  uint8_t data[0x7F];
  uint8_t length;
  uint8_t crc_status;
  uint8_t energy_detect;
  uint16_t source_address;
};
struct tx_frame {
  uint8_t data[0x7F];
  uint8_t length;
  uint8_t ack;
  uint16_t destination_address;
};

/* ---- Function type definition for the received callback ---- */
typedef void (*rx_callback_func) (struct rx_frame*);

/* ---- Represents an interface to a radio ---- */
struct radif {
  /* ---- Data ---- */
  volatile uint8_t RxProduceIndex, RxConsumeIndex;
  volatile uint8_t TxConsumeIndex, TxProduceIndex;

  struct rx_frame RxFrames[NUM_RXFRAMES];
  struct tx_frame TxFrames[NUM_TXFRAMES];

  volatile uint8_t Command, CommandOutput;

  /* ---- Configuration ---- */
  uint8_t clkm_config; /* See §7.7.6 (p120) of the AT86RF212 datasheet */
  uint8_t auto_crc_gen; /* 0: Don't automatically CRC  Otherwise: Replace the last two bytes of outgoing frames with a CRC */
  uint8_t promiscuous; /* 0: Only frames matching our address are received.  Otherwise: All frames are received regardless of destination */
  uint16_t freq; /* The frequency of the radio */
  uint8_t power; /* The power of the radio */
  uint8_t modulation; /* The modulation of the radio */
  uint16_t pan_id; /* Addressing */
  uint16_t short_address; /* 16 bit address */

  /* A flag to signify if the radio is operational */
  uint8_t up;

  /* The current sequence ID we're transmitting at */
  uint8_t seq;

  /* The callback function for when data is received */
  rx_callback_func rx_callback;

  /* ---- Pointers to the hardware functions we need for the radio interface ---- */
  pin_set_func spi_start;
  pin_set_func spi_stop;
  spi_xfer_func spi_xfer;
  pin_set_func slptr_set;
  pin_set_func slptr_clear;
  pin_set_func reset_set;
  pin_set_func reset_clear;
  delay_us_func delay_us;
  interrupt_trigger_func interrupt_trigger;

  /* ---- Statistics ---- */
  uint16_t rx_success_count;
  uint16_t rx_overflow;

  uint16_t tx_success_count;
  uint16_t tx_channel_fail;
  uint16_t tx_noack;
  uint16_t tx_invalid;

  uint16_t last_trac_status;
};

/* This is what's sent when the radio doesn't care what we send */
enum {
  BLANK_SPI_CHARACTER		= 0
};
/* -------- Radio Commands -------- */
enum {
  RADIF_NO_COMMAND = 0,
  RADIF_RESET,
  RADIF_STARTUP,
  RADIF_GET_RANDOM_NUMBER,
  RADIF_SET_MODULATION,
  RADIF_SET_FREQ,
  RADIF_SET_POWER,
  RADIF_SET_ADDRESS,
  RADIF_ENERGY,
  RADIF_WAKE,
  RADIF_SLEEP
};
/* -------- Radio Modulation Modes -------- */
enum {
  RADIF_1000KBITS_S_SCRAMBLER		= 0x20,
  RADIF_1000KCHIPS_SIN			= 0x10,
  RADIF_1000KCHIPS_RC			= 0,
  RADIF_OQPSK_1000KCHIPS_1000KBITS_S	= 0x0E,
  RADIF_OQPSK_1000KCHIPS_500KBITS_S	= 0x0D,
  RADIF_OQPSK_1000KCHIPS_250KBITS_S	= 0x0C,
  RADIF_OQPSK_400KCHIPS_400KBITS_S	= 0x0A,
  RADIF_OQPSK_400KCHIPS_200KBITS_S	= 0x09,
  RADIF_OQPSK_400KCHIPS_100KBITS_S	= 0x08,
  RADIF_BPSK_600KCHIPS_40KBITS_S	= 0x4,
  RADIF_BPSK_300KCHIPS_20KBITS_S	= 0,
  RADIF_OQPSK				= 0x08,
  RADIP_BPSK				= 0
};
/* -------- Radio statuses -------- */
enum {
  RADIO_SUCCESS = 0x40,                       /* The requested service was performed successfully. */
  RADIO_UNSUPPORTED_DEVICE,                   /* The connected device is not an Atmel AT86RF212. */
  RADIO_INVALID_ARGUMENT,                     /* One or more of the supplied function arguments are invalid. */
  RADIO_TIMED_OUT,                            /* The requested service timed out. */
  RADIO_WRONG_STATE,                          /* The end-user tried to do an invalid state transition. */
  RADIO_BUSY_STATE,                           /* The radio transceiver is busy receiving or transmitting. */
  RADIO_STATE_TRANSITION_FAILED,              /* The requested state transition could not be completed. */
  RADIO_CCA_IDLE,                             /* Channel is clear, available to transmit a new frame. */
  RADIO_CCA_BUSY,                             /* Channel busy. */
  RADIO_TRX_BUSY,                             /* Transceiver is busy receiving or transmitting data. */
  RADIO_BAT_LOW,                              /* Measured battery voltage is lower than voltage threshold. */
  RADIO_BAT_OK,                               /* Measured battery voltage is above the voltage threshold. */
  RADIO_CRC_FAILED,                           /* The CRC failed for the actual frame. */
  RADIO_CHANNEL_ACCESS_FAILURE,               /* The channel access failed during the auto mode. */
  RADIO_NO_ACK,                               /* No acknowledge frame was received. */
};
/* -------- TRAC statuses -------- */
enum {
  TRAC_SUCCESS               = 0,
  TRAC_SUCCESS_DATA_PENDING  = 1,
  TRAC_WAIT_FOR_ACK          = 2,
  TRAC_CHANNEL_ACCESS_FAIL   = 3,
  TRAC_NO_ACK                = 5,
  TRAC_INVALID               = 7
};

uint8_t radif_command(uint8_t command, struct radif* radif); /* Do a command */
void radif_service(struct radif* radif); /* Calls the receive callback on all pending frames */
void radif_send(uint8_t* frame, uint8_t len, uint16_t dest_addr, uint8_t ack, struct radif* radif); /* Transmits a frame over the radio interface */
void radif_init_struct(struct radif* radif); /* Initialises the radio interface */

#endif /* RADIO_H */
