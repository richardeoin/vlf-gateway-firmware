/* 
 * Handles the radio interrupt event
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

#include <stdlib.h>

#include "radio.h"
#include "radio_functions.h"

/* -------- Command -------- */

void radio_command(struct radif* radif) {
  if (radif->Command != RADIF_NO_COMMAND) { /* If there's a command waiting */
    switch (radif->Command) {
      case RADIF_RESET:
	radif->CommandOutput = radio_reset(radif);
	break;
      case RADIF_STARTUP: radio_config(radif); /* Apply the configuration values from the radif struct to the radio */
	radif->up = 0xFF; /* Start the interface */
	break;
      case RADIF_GET_RANDOM_NUMBER: radif->CommandOutput = radio_get_random(radif);
	break;
      case RADIF_SET_MODULATION: radio_set_modulation(radif);
	break;
      case RADIF_SET_FREQ: radif->CommandOutput = radio_set_freq(radif);
	break;
      case RADIF_SET_POWER: radio_set_pwr(radif);
	break;
      case RADIF_SET_ADDRESS: radio_set_address(radif);
	break;
      case RADIF_ENERGY: radif->CommandOutput = radio_measure_energy(radif);
	break;
      case RADIF_WAKE: radio_wake(radif); radif->up = 0xFF;
	break;
      case RADIF_SLEEP: radio_sleep(radif); radif->up = 0;
	break;
      default: break; /* Invalid Command */
    }
  }

  radif->Command = RADIF_NO_COMMAND; /* Increment our consume index */
}

/* -------- Tx -------- */

void radio_tx(struct radif* radif) {
  uint16_t index = radif->TxConsumeIndex;

  if (index != radif->TxProduceIndex) { /* If there's data to be output */
    if (!radio_is_state_busy(radif)) { /* If we're not currently busy */
      struct tx_frame* tx = &radif->TxFrames[index];

      /* Stop receiving */
      radio_set_state(TRX_OFF, radif);
      /* Get ready to transmit */
      radio_set_state(TX_ARET_ON, radif);

      /* Write the frame into a buffer */
      radio_frame_write(tx, radif);

      /* Actually start the transmission */
      radio_reg_read_mod_write(TRX_STATE, CMD_TX_START, 0x1F, radif);

      /* Increment our consume index - We're all done with this frame */
      radif->TxConsumeIndex = (index+1) % NUM_TXFRAMES;
    }
  }
}
void radio_tx_end(struct radif* radif) {
  /* See how the transmission went */
  uint8_t trac_status = radio_get_trac(radif);

  radif->last_trac_status = trac_status;

  if (trac_status == TRAC_SUCCESS || trac_status == TRAC_SUCCESS_DATA_PENDING) { /* We've successfully consumed a frame */
    radif->tx_success_count++; /* Update the statistics */
  } else if (trac_status == TRAC_CHANNEL_ACCESS_FAIL) {
    radif->tx_channel_fail++;
  } else if (trac_status == TRAC_NO_ACK) {
    radif->tx_noack++;
  } else {
    radif->tx_invalid++; /* This should never happen. Was radio_tx_end() called too early? */
  }
}

/* ------- Rx --------- */

void radio_rx_end(struct radif* radif) {
  /* Find the next index */
  uint16_t index = radif->RxProduceIndex;
  uint16_t next = (index+1) % NUM_RXFRAMES;

  if (next != radif->RxConsumeIndex) { /* This isn't going to collide with the consume index */
    struct rx_frame* rx = &radif->RxFrames[index];

    /* Get the ED measurement */
    rx->energy_detect = radio_reg_read(PHY_ED_LEVEL, radif);

    /* Find out if the CRC on the last received packet was valid */
    rx->crc_status = (radio_reg_read(PHY_RSSI, radif) & (1<<7)) ? 1 : 0;

    /* Read in the frame */
    radio_frame_read(rx, radif);

    /* Move along the produce index */
    radif->RxProduceIndex = next;
    /* Increment the statistics */
    radif->rx_success_count++;
  } else { /* No space in our internal buffers */
    /* Increment the overflow statistics */
    radif->rx_overflow++;
    /* Read the frame in to clear it */
    radio_frame_read_dummy(radif);
  }

  while (radio_get_state(radif) == BUSY_RX_AACK);
}

void radio_trx_end(struct radif* radif) {
  uint8_t state = radio_get_state(radif);

  /* See what state we've been in */
  if (state == RX_ON || state == RX_AACK_ON || state == BUSY_RX_AACK) { /* We've been receiving */
    radio_rx_end(radif);
  } else { /* We've been transmitting */
    radio_tx_end(radif);
  }
}

void radio_irq(struct radif* radif) {
  if (radif->up) {
    /* Get the flags for the currently active interrupts */
    uint8_t intp_src = radio_reg_read(IRQ_STATUS, radif);

    /* Deal with each of the current interrupts in turn */
    if (intp_src & RADIO_IRQ_RX_START) {
      /* We could start to read in frames here, but then we'd have to stagger the SPI read */
    }
    if (intp_src & RADIO_IRQ_TRX_END) {
      radio_trx_end(radif);
    }
    if (intp_src & RADIO_IRQ_TRX_UR) {
      /* We shouldn't get any problems here as long as the SPI clock is higher than the radio link bitrate */
    }
    if (intp_src & RADIO_IRQ_PLL_UNLOCK) {
    }
    if (intp_src & RADIO_IRQ_PLL_LOCK) {
    }
    if (intp_src & RADIO_IRQ_BAT_LOW) {
    }

    /* Do any outstanding frame transmissions */
    radio_tx(radif);
  }

  /* Do any outstanding commands */
  radio_command(radif);

  if (radif->up && !radio_is_state_busy(radif)) { /* If we're not currently busy */
    /* Put ourselves into receiving mode */
    while (radio_set_state(RX_AACK_ON, radif) != RADIO_SUCCESS);
  }
}
