/****************************************************************************
 * Copyright (C) 2013 Richard Meadows - Header file for an AT86RF212
 * Copyright (C) 2009 FreakLabs - Definition of various constants
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * For definitions of various constants:
 *    Originally written by Christopher Wang aka Akiba.
 *    Please post support questions to the FreakLabs forum.
 *
 *****************************************************************************/

#ifndef RADIO_FUNCTIONS_H
#define RADIO_FUNCTIONS_H

/* Clear Channel Assessment constants */
enum {
  CCA_ED                    = 1,    /* Use energy detection above threshold mode. */
  CCA_CARRIER_SENSE         = 2,    /* Use carrier sense mode. */
  CCA_CARRIER_SENSE_WITH_ED = 3     /* Use a combination of both energy detection and carrier sense. */
};
/* Configuration parameters */
enum {
  RADIO_MAX_FRAME_RETRIES   = 3,
  RADIO_MAX_CSMA_RETRIES    = 4,
  CHB_CCA_MODE            = CCA_ED,
  CHB_MIN_BE              = 3,
  CHB_MAX_BE              = 5,
  CHB_CCA_ED_THRES        = 0x7,
  CHB_CSMA_SEED0          = 0,
  CHB_CSMA_SEED1          = 0,
  CHB_FRM_VER             = 1         /* Accept 802.15.4 version 0 or 1 frames */
};

/* AT86RF Register addresses */
enum {
  TRX_STATUS              = 0x01,
  TRX_STATE               = 0x02,
  TRX_CTRL_0              = 0x03,
  TRX_CTRL_1              = 0x04,
  PHY_TX_PWR              = 0x05,
  PHY_RSSI                = 0x06,
  PHY_ED_LEVEL            = 0x07,
  PHY_CC_CCA              = 0x08,
  CCA_THRES               = 0x09,
  RX_CTRL                 = 0x0a,
  SFD_VALUE               = 0x0b,
  TRX_CTRL_2              = 0x0c,
  ANT_DIV                 = 0x0d,
  IRQ_MASK                = 0x0e,
  IRQ_STATUS              = 0x0f,
  VREG_CTRL               = 0x10,
  BATMON                  = 0x11,
  XOSC_CTRL               = 0x12,
  CC_CTRL_0               = 0x13,
  CC_CTRL_1               = 0x14,
  RX_SYN                  = 0x15,
  RF_CTRL_0               = 0x16,
  XAH_CTRL_1              = 0x17,
  FTN_CTRL                = 0x18,
  RF_CTRL_1               = 0x19,
  PLL_CF                  = 0x1a,
  PLL_DCU                 = 0x1b,
  PART_NUM                = 0x1c,
  VERSION_NUM             = 0x1d,
  MAN_ID_0                = 0x1e,
  MAN_ID_1                = 0x1f,
  SHORT_ADDR_0            = 0x20,
  SHORT_ADDR_1            = 0x21,
  PAN_ID_0                = 0x22,
  PAN_ID_1                = 0x23,
  IEEE_ADDR_0             = 0x24,
  IEEE_ADDR_1             = 0x25,
  IEEE_ADDR_2             = 0x26,
  IEEE_ADDR_3             = 0x27,
  IEEE_ADDR_4             = 0x28,
  IEEE_ADDR_5             = 0x29,
  IEEE_ADDR_6             = 0x2a,
  IEEE_ADDR_7             = 0x2b,
  XAH_CTRL_0              = 0x2c,
  CSMA_SEED_0             = 0x2d,
  CSMA_SEED_1             = 0x2e,
  CSMA_BE                 = 0x2f
};
/* Useful bits */
enum {
  RADIO_AUTO_CRC_GEN		= 0x20,
  RADIO_PROMISCUOUS		= 0x2
};
/* Random defines */
enum {
  CHB_CSMA_SEED1_POS          = 0,
  CHB_CCA_MODE_POS            = 5,
  CHB_TRX_END_POS             = 3,
  CHB_TRAC_STATUS_POS         = 5,
  CHB_FVN_POS                 = 6,
  CHB_OQPSK_TX_OFFSET         = 2,
  CHB_BPSK_TX_OFFSET          = 3,
  CHB_PA_EXT_EN_POS           = 7
};

/* Transceiver timing (in microseconds) */
/* See Table 5-1 (p38) of the AT86RF212 datasheet for more details */

/* Note: Some of these settings are dependent on external capacitors, crystals and so on
 * Hence it might be a good idea to be conservative when doing initial tests. */
enum {
  /* Start-up */
  TIME_RST_PULSE_WIDTH        = 1, 	/* Actually 625ns */
  TIME_P_ON_WAIT				= 400, 	/* As per Table A-5, (p166) */

  /* State Changes */
  TIME_P_ON_TO_CLKM_AVAIL     = 330,	/* Dependent on external setup */
  TIME_SLEEP_TO_TRX_OFF       = 380,	/* Dependent on external setup */
  TIME_TRX_OFF_TO_SLEEP       = 35, 	/* Assuming CLKM is left at 1MHz */
  TIME_TRX_OFF_TO_PLL_ON		= 200,	/* Dependent on external setup */
  TIME_PLL_ON_TO_TRX_OFF      = 1,
  TIME_TRX_OFF_TO_RX_ON       = 200,	/* Dependent on external setup */
  TIME_RX_ON_TO_TRX_OFF       = 1,
  TIME_PLL_ON_TO_RX_ON        = 1,
  TIME_RX_ON_TO_PLL_ON        = 1,
  TIME_BUSY_TX_TO_PLL_ON      = 32,
  TIME_ALL_STATES_TRX_OFF     = 1,
  TIME_RESET_TO_TRX_OFF       = 26,

  /* Misc */
  TIME_PLL_LOCK_TIME          = 200,
  TIME_TRX_IRQ_DELAY          = 9,
  TIME_IRQ_PROCESSING_DLY     = 32
};
/* Transceiver commands */
enum {
  CMD_NOP                 = 0,
  CMD_TX_START            = 2,
  CMD_FORCE_TRX_OFF       = 3,
  CMD_FORCE_PLL_ON        = 4,
  CMD_RX_ON               = 6,
  CMD_TRX_OFF             = 8,
  CMD_PLL_ON              = 9,
  CMD_RX_AACK_ON          = 22,
  CMD_TX_ARET_ON          = 25
};
/* Transceiver states */
enum {
  P_ON               = 0,
  BUSY_RX            = 1,
  BUSY_TX            = 2,
  RX_ON              = 6,
  TRX_OFF            = 8,
  PLL_ON             = 9,
  SLEEP              = 15,
  BUSY_RX_AACK       = 17,
  BUSY_TX_ARET       = 18,
  RX_AACK_ON         = 22,
  TX_ARET_ON         = 25,
  RX_ON_NOCLK        = 28,
  RX_AACK_ON_NOCLK   = 29,
  BUSY_RX_AACK_NOCLK = 30,
  TRANS_IN_PROG      = 31
};
enum {
  AT86RF212_VER_NUM		= 0x01,		/* AT86RF212 version number */
  AT86RF212_PART_NUM		= 0x07		/* AT86RF212 part number */
};
/* Commands */
enum {
  RADIO_SPI_CMD_RW				= 0xC0,		/* Register Write (short mode). */
  RADIO_SPI_CMD_RR				= 0x80,		/* Register Read (short mode). */
  RADIO_SPI_CMD_FW				= 0x60,		/* Frame Transmit Mode (long mode). */
  RADIO_SPI_CMD_FR				= 0x20,		/* Frame Receive Mode (long mode). */
  RADIO_SPI_CMD_SW				= 0x40,		/* SRAM Write. */
  RADIO_SPI_CMD_SR				= 0x00,		/* SRAM Read. */
  RADIO_SPI_CMD_RADDRM			= 0x7F		/* Register Address Mask. */
};
/* Interrupt Masks */
enum {
  RADIO_IRQ_BAT_LOW			= 0x80,		/* Mask for the BAT_LOW interrupt. */
  RADIO_IRQ_TRX_UR			= 0x40,		/* Mask for the TRX_UR interrupt. */
  RADIO_IRQ_CCA_ED_DONE		= 0x10,		/* Mask for the CCA_ED_DONE interrupt */
  RADIO_IRQ_TRX_END			= 0x08,		/* Mask for the TRX_END interrupt. */
  RADIO_IRQ_RX_START			= 0x04,		/* Mask for the RX_START interrupt. */
  RADIO_IRQ_PLL_UNLOCK		= 0x02,		/* Mask for the PLL_UNLOCK interrupt. */
  RADIO_IRQ_PLL_LOCK			= 0x01  	/* Mask for the PLL_LOCK interrupt. */
};

/* -------- Register Read, Write & Read-Modify-Write -------- */
uint8_t radio_reg_read(uint8_t addr, struct radif* radif);
uint16_t radio_reg_read16(uint8_t addr, struct radif* radif);
void radio_reg_write(uint8_t addr, uint8_t val, struct radif* radif);
void radio_reg_write16(uint8_t addr, uint16_t val, struct radif* radif);
void radio_reg_write64(uint8_t addr, uint8_t *val, struct radif* radif);
void radio_reg_read_mod_write(uint8_t addr, uint8_t val, uint8_t mask, struct radif* radif);
/* -------- Frame Read & Write -------- */
uint8_t radio_frame_read(struct rx_frame* rx, struct radif* radif);
void radio_frame_read_dummy(struct radif* radif);
void radio_frame_write(struct tx_frame* tx, struct radif* radif);
/* -------- SRAM Read & Write -------- */
void radio_sram_read(uint8_t addr, uint8_t len, uint8_t* data, struct radif* radif);
void radio_sram_write(uint8_t addr, uint8_t len, uint8_t* data, struct radif* radif);
/* -------- Radio State -------- */
uint8_t radio_get_state(struct radif* radif);
uint8_t radio_is_state_busy(struct radif* radif);
uint8_t radio_get_trac(struct radif* radif);
/* -------- Set Radio Properties -------- */
void radio_set_modulation(struct radif* radif);
uint8_t radio_set_freq(struct radif* radif);
void radio_set_pwr(struct radif* radif);
void radio_set_address(struct radif* radif);
/* -------- Set State  -------- */
uint8_t radio_set_state(uint8_t state, struct radif* radif);
/* -------- Wake & Sleep -------- */
void radio_sleep(struct radif* radif);
void radio_wake(struct radif* radif);
/* -------- Misc -------- */
uint8_t radio_get_random(struct radif* radif);
uint8_t radio_measure_energy(struct radif* radif);
/* -------- Reset -------- */
uint8_t radio_reset(struct radif* radif);
void radio_config(struct radif* radif);

#endif /* RADIO_FUNCTIONS_H */
