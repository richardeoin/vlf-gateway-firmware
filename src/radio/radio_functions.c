/* 
 * Functions for controlling a AT86RF212
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

#include "radio.h"
#include "radio_functions.h"

#include "ieee_frame.h"

/**
 * Various functions to be used by the radio driver. This must never
 * be called from more than one interrupt level
 */

/* -------- Register Read, Write & Read-Modify-Write -------- */

uint8_t radio_reg_read(uint8_t addr, struct radif* radif) {
  radif->spi_start();

  /* Send Register address and read register content.*/
  radif->spi_xfer(addr | RADIO_SPI_CMD_RR);
  uint8_t val = radif->spi_xfer(BLANK_SPI_CHARACTER);

  radif->spi_stop();

  return val;
}
uint16_t radio_reg_read16(uint8_t addr, struct radif* radif) {

  uint16_t val = radio_reg_read(addr, radif);
  val |= radio_reg_read(addr+1, radif) << 8;

  return val;
}
void radio_reg_write(uint8_t addr, uint8_t val, struct radif* radif) {
  radif->spi_start();

  /* Send Register address and write register content.*/
  radif->spi_xfer(addr | RADIO_SPI_CMD_RW);
  radif->spi_xfer(val);

  radif->spi_stop();
}
void radio_reg_write16(uint8_t addr, uint16_t val, struct radif* radif) {

  radio_reg_write(addr, val, radif);
  radio_reg_write(addr+1, val >> 8, radif);
}
void radio_reg_write64(uint8_t addr, uint8_t *val, struct radif* radif) {
  int i;

  for (i=0; i<8; i++) {
    radio_reg_write(addr + i, *(val + i), radif);
  }
}
void radio_reg_read_mod_write(uint8_t addr, uint8_t val, uint8_t mask, struct radif* radif) {
  uint8_t tmp;

  tmp = radio_reg_read(addr, radif);
  val &= mask;                /* Mask off stray bits from value */
  tmp &= ~mask;               /* Mask off bits in register value */
  tmp |= val;                 /* Copy value into register value */
  radio_reg_write(addr, tmp, radif);   /* Write back to register */
}

/* -------- Frame Read & Write -------- */

uint8_t radio_frame_read(struct rx_frame* rx, struct radif* radif) {
  radif->spi_start();

  /* Send frame read command */
  radif->spi_xfer(RADIO_SPI_CMD_FR);

  /* Read the of the whole frame */
  uint8_t mpdu_len = radif->spi_xfer(BLANK_SPI_CHARACTER);
  /* Read in the header */
  uint8_t hdr_len = read_in_ieee_header(rx, radif);
  /* Work out how long the actual data is */
  rx->length = mpdu_len - (hdr_len + 2);

  /* Read in the MAC Service Data Unit */
  uint8_t i;
  for (i=0; i<(rx->length); i++) {
    rx->data[i] = radif->spi_xfer(BLANK_SPI_CHARACTER);
  }

  /* We don't bother reading in the Frame Check Sequence */

  radif->spi_stop();

  return 0xFF; /* TODO Return 0 on invalid frame */
}
void radio_frame_read_dummy(struct radif* radif) {
  radif->spi_start();

  /* Send frame read command and read the length.*/
  radif->spi_xfer(RADIO_SPI_CMD_FR);
  uint8_t len = radif->spi_xfer(BLANK_SPI_CHARACTER);

  uint8_t i;
  for (i=0; i<len; i++) {
    radif->spi_xfer(BLANK_SPI_CHARACTER);
  }

  radif->spi_stop();
}
/* Writes a frame. The first byte of the frame should be its length */
void radio_frame_write(struct tx_frame* tx, struct radif* radif) {
  radif->spi_start();

  /* Send frame write command */
  radif->spi_xfer(RADIO_SPI_CMD_FW);

  /* Write out the length (MAC Service Data Unit + Header + FCS) */
  uint8_t len = tx->length + ieee_header_len(tx) + 2;
  radif->spi_xfer(len);

  /* If the length is too big, halt! */
  if (len > 127) {
    while(1); // ERROR - Frame too long!
  }

  /* Write out the header */
  write_out_ieee_header(tx, radif);

  /* Write out the MAC Service Data Unit */
  uint8_t i;
  for (i=0; i<(tx->length); i++) {
    radif->spi_xfer(tx->data[i]);
  }

  /* Write two bytes in place of the Frame Check Sequence which will be added by the radio */
  radif->spi_xfer(BLANK_SPI_CHARACTER);
  radif->spi_xfer(BLANK_SPI_CHARACTER);

  radif->spi_stop();
}


/* -------- SRAM Read & Write -------- */

void radio_sram_read(uint8_t addr, uint8_t len, uint8_t* data, struct radif* radif) {
  int i;

  radif->spi_start();

  /* Send SRAM read command.*/
  radif->spi_xfer(RADIO_SPI_CMD_SR);

  /* Send address where to start reading.*/
  radif->spi_xfer(addr);

  for (i=0; i<len; i++) {
    *data++ = radif->spi_xfer(BLANK_SPI_CHARACTER);
  }

  radif->spi_stop();
}
void radio_sram_write(uint8_t addr, uint8_t len, uint8_t* data, struct radif* radif) {
  int i;

  radif->spi_start();

  /* Send SRAM write command.*/
  radif->spi_xfer(RADIO_SPI_CMD_SW);

  /* Send address where to start writing to.*/
  radif->spi_xfer(addr);

  for (i=0; i<len; i++) {
    radif->spi_xfer(*data++);
  }

  radif->spi_stop();
}

/* -------- Radio State -------- */

uint8_t radio_get_state(struct radif* radif) {
  return radio_reg_read(TRX_STATUS, radif) & 0x1f;
}
uint8_t radio_is_state_busy(struct radif* radif) {
  uint8_t state = radio_get_state(radif);
  if (state == BUSY_RX || state == BUSY_RX_AACK || state == BUSY_RX_AACK_NOCLK ||
      state == BUSY_TX || state == BUSY_TX_ARET) {
    return 1;
  } else {
    return 0;
  }
}
uint8_t radio_get_trac(struct radif* radif) {
  return radio_reg_read(TRX_STATE, radif) >> CHB_TRAC_STATUS_POS;
}

/* -------- Set Radio Properties -------- */

void radio_set_modulation(struct radif* radif) {
  radio_set_state(TRX_OFF, radif); /* The radio must be in TRX_OFF to change the modulation */

  radio_reg_read_mod_write(TRX_CTRL_2, radif->modulation, 0x3F, radif);

  if (radif->modulation & RADIF_OQPSK) { /* According to table 7-16 in at86rf212 datasheet */
    radio_reg_read_mod_write(RF_CTRL_0, CHB_OQPSK_TX_OFFSET, 0x3, radif);
  } else {
    radio_reg_read_mod_write(RF_CTRL_0, CHB_BPSK_TX_OFFSET, 0x3, radif);
  }
}
uint8_t radio_set_freq(struct radif* radif) {
  uint16_t freq = radif->freq;
  uint8_t band, number;

  /* Translate the frequency (given in MHz or 100s of kHz) into a band and number
   * See Table 7-35 in the AT86RF212 datasheet */
  if (7690 <= freq && freq <= 7945) { /* 769.0 MHz - 794.5 MHz: Chinese Band */
    band = 1; number = freq-7690;
  } else if (8570 <= freq && freq <= 8825) { /* 857.0 MHz - 882.5 MHz: European Band */
    band = 2; number = freq-8570;
  } else if (9030 <= freq && freq <= 9285) { /* 903.0 MHz - 928.5 MHz: North American Band */
    band = 3; number = freq-9030;
  } else if (769 <= freq && freq <= 863) { /* 769 MHz - 863 MHz: General 1 */
    band = 4; number = freq-769;
  } else if (833 <= freq && freq <= 935) { /* 833 MHz - 935 MHz: General 2 */
    band = 5; number = freq-833;
  } else { /* Unknown frequency */
    return RADIO_INVALID_ARGUMENT;
  }

  /* Write these values to the control register */
  radio_reg_read_mod_write(CC_CTRL_1, band, 0x7, radif);
  radio_reg_write(CC_CTRL_0, number, radif);

  /* Add a delay to allow the PLL to lock if in active mode. */
  uint8_t state = radio_get_state(radif);
  if ((state == RX_ON) || (state == PLL_ON)) {
    radif->delay_us(TIME_PLL_LOCK_TIME);
  }

  /* TODO: Wait for the PLL to lock instead */

  return RADIO_SUCCESS;
}
void radio_set_pwr(struct radif* radif) {
  radio_reg_write(PHY_TX_PWR, radif->power, radif);
}
void radio_set_address(struct radif* radif) {
  /* Set PAN ID */
  radio_reg_write16(PAN_ID_0, radif->pan_id, radif);

  /* Set the Short Address */
  radio_reg_write16(SHORT_ADDR_0, radif->short_address, radif);
}

/* -------- Set State  -------- */

uint8_t radio_set_state(uint8_t state, struct radif* radif) {
  uint8_t curr_state = radio_get_state(radif);

  /* If we're already in the correct state it's not a problem */
  if (curr_state == state) { return RADIO_SUCCESS; }

  /* If we're in a transition state, wait for the state to become stable */
  if ((curr_state == BUSY_TX_ARET) || (curr_state == BUSY_RX_AACK) || (curr_state == BUSY_RX) || (curr_state == BUSY_TX)) {
    while (radio_get_state(radif) == curr_state);
  }

  /* At this point it is clear that the requested new_state is one of */
  /* TRX_OFF, RX_ON, PLL_ON, RX_AACK_ON or TX_ARET_ON. */
  /* we need to handle some special cases before we transition to the new state */
  switch (state) {
    case TRX_OFF:
      /* Go to TRX_OFF from any state. */
      radif->slptr_clear();
      radio_reg_read_mod_write(TRX_STATE, CMD_FORCE_TRX_OFF, 0x1f, radif);
      radif->delay_us(TIME_ALL_STATES_TRX_OFF);
      break;

    case TX_ARET_ON:
      if (curr_state == RX_AACK_ON) {
	/* First do intermediate state transition to PLL_ON, then to TX_ARET_ON. */
	radio_reg_read_mod_write(TRX_STATE, CMD_PLL_ON, 0x1f, radif);
	radif->delay_us(TIME_RX_ON_TO_PLL_ON);
      }
      break;

    case RX_AACK_ON:
      if (curr_state == TX_ARET_ON) {
	/* First do intermediate state transition to RX_ON, then to RX_AACK_ON. */
	radio_reg_read_mod_write(TRX_STATE, CMD_PLL_ON, 0x1f, radif);
	radif->delay_us(TIME_RX_ON_TO_PLL_ON);
      }
      break;
  }

  /* Now we're okay to transition to any new state. */
  radio_reg_read_mod_write(TRX_STATE, state, 0x1f, radif);

  /* When the PLL is active most states can be reached in 1us. However, from */
  /* TRX_OFF the PLL needs time to activate. */
  uint32_t delay = (curr_state == TRX_OFF) ? TIME_TRX_OFF_TO_PLL_ON : TIME_RX_ON_TO_PLL_ON;
  radif->delay_us(delay);

  if (radio_get_state(radif) == state) {
    return RADIO_SUCCESS;
  }

  return RADIO_TIMED_OUT;
}

/* -------- Wake & Sleep -------- */

void radio_sleep(struct radif* radif) {
  /* First we need to go to TRX OFF state */
  radio_set_state(TRX_OFF, radif);

  /* Set the SLPTR pin */
  radif->slptr_set();
}
void radio_wake(struct radif* radif) {
  /* Clear the SLPTR pin */
  radif->slptr_clear();

  /* We need to allow some time for the PLL to lock */
  radif->delay_us(TIME_SLEEP_TO_TRX_OFF);

  /* Turn the transceiver back on */
  radio_set_state(RX_AACK_ON, radif);
}

/* -------- Misc -------- */

uint8_t radio_get_random(struct radif* radif) {
  /* Set the radio in the standard operating mode to do this */
  radio_set_state(RX_ON, radif);

  uint8_t i, rand = 0;
  for (i = 0; i < 8; i+=2) {
    rand |= ((radio_reg_read(PHY_RSSI, radif) << 1) & 0xC0) >> i;
  }

  return rand;
}
uint8_t radio_measure_energy(struct radif* radif) { /* TODO: Make this more efficient */
  /* Set the radio in the standard operating mode to do this */
  radio_set_state(RX_ON, radif);

  /* Write to PHY_ED_LEVEL to trigger off the measurement */
  radio_reg_write(PHY_ED_LEVEL, BLANK_SPI_CHARACTER, radif);

  /* Enable the CCA_ED_DONE interrupt */
  radio_reg_read_mod_write(IRQ_MASK, RADIO_IRQ_CCA_ED_DONE, RADIO_IRQ_CCA_ED_DONE, radif);

  /* Wait for interrupt CCA_ED_DONE */
  while ((radio_reg_read(IRQ_STATUS, radif) & RADIO_IRQ_CCA_ED_DONE) == 0);

  /* Disable the CCA_ED_DONE interrupt */
  radio_reg_read_mod_write(IRQ_MASK, 0, RADIO_IRQ_CCA_ED_DONE, radif);

  /* Return the result */
  return radio_reg_read(PHY_ED_LEVEL, radif);
}

/* -------- Reset -------- */

uint8_t radio_reset(struct radif* radif) {
  /* This is the reset procedure as per Table A-5 (p166) of the AT86RF212 datasheet */

  /* Set input pins to their default operating values */
  radif->reset_clear();
  radif->slptr_clear();
  radif->spi_stop();

  /* Wait while transceiver wakes up */
  radif->delay_us(TIME_P_ON_WAIT);

  /* Reset the device */
  radif->reset_set();
  radif->delay_us(TIME_RST_PULSE_WIDTH);
  radif->reset_clear();

  uint8_t i = 0;
  /* Check that we have the part number that we're expecting */
  while ((radio_reg_read(VERSION_NUM, radif) != AT86RF212_VER_NUM) || (radio_reg_read(PART_NUM, radif) != AT86RF212_PART_NUM)) {
    if (i++ > 100) { /* This is never going to work, we've got the wrong part number */
      return RADIO_UNSUPPORTED_DEVICE;
    }
  }

  /* Set the CLKM output to 1MHz, 4mA driver strength */
  radio_reg_read_mod_write(TRX_CTRL_0, 0x19, 0x3F, radif);

  /* Force transceiver into TRX_OFF state */
  radio_reg_read_mod_write(TRX_STATE, CMD_FORCE_TRX_OFF, 0x1F, radif);
  radif->delay_us(TIME_ALL_STATES_TRX_OFF);

  i = 0;
  /* Make sure the transceiver is in the off state before proceeding */
  while ((radio_reg_read(TRX_STATUS, radif) & 0x1f) != TRX_OFF) {
    if (i++ > 100) { /* Nope, it's never going to change state */
      return RADIO_WRONG_STATE;
    }
  }

  radio_reg_read(IRQ_STATUS, radif); /* Clear any outstanding interrupts */
  radio_reg_write(IRQ_MASK, 0, radif); /* Disable interrupts */

  return RADIO_SUCCESS;
}
/* Setup various configuration parameters from the radif structure */
void radio_config(struct radif* radif) {
  /* Set the number of retries if no ACK is received */
  radio_reg_write(XAH_CTRL_0, (RADIO_MAX_FRAME_RETRIES << 4) | (RADIO_MAX_CSMA_RETRIES << 1), radif);

  //chb_reg_read_mod_write(CSMA_SEED_1, CHB_CSMA_SEED1 << CHB_CSMA_SEED1_POS, 0x7 << CHB_CSMA_SEED1_POS);
  //chb_ret_write(CSMA_SEED0, CHB_CSMA_SEED0);
  //chb_reg_read_mod_write(PHY_CC_CCA, CHB_CCA_MODE << CHB_CCA_MODE_POS,0x3 << CHB_CCA_MODE_POS);
  //chb_reg_write(CCA_THRES, CHB_CCA_ED_THRES);

  //radio_reg_write(RF_CTRL_1, 0xF0, radif);

  // Set frame version that we'll accept
  radio_reg_read_mod_write(CSMA_SEED_1, CHB_FRM_VER << CHB_FVN_POS, 3 << CHB_FVN_POS, radif);

  /* Enable interrupts */
  radio_reg_write(IRQ_MASK, RADIO_IRQ_RX_START | RADIO_IRQ_TRX_END, radif);

  /* Enable Automatic CRC */
  if (radif->auto_crc_gen) {
    radio_reg_read_mod_write(TRX_CTRL_1, RADIO_AUTO_CRC_GEN, RADIO_AUTO_CRC_GEN, radif);
  }
  /* Enable promiscuous mode */
  if (radif->promiscuous) {
    radio_reg_read_mod_write(XAH_CTRL_1, RADIO_PROMISCUOUS, RADIO_PROMISCUOUS, radif);
  }

  /* Take a random sequence number to start with */
  radif->seq = radio_get_random(radif);
}
