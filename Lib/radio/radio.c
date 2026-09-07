/*
 * Copyright (c) 2018 - 2021, ETH Zurich, Computer Engineering Group (TEC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "flora_lib.h"

#if RADIO_ENABLE

extern void (*RadioOnDioIrqCallback)(void);
extern const struct Radio_s Radio;
extern radio_message_t* last_message_list;

/* shared state */
volatile bool radio_irq_direct = false;

/* internal state */
static volatile radio_sleeping_t  radio_sleeping = RADIO_SLEEPING_FALSE;
static bool                       rx_boosted = true;
static uint64_t                   radio_last_sync_timestamp = 0;
static uint8_t                    preamble_detected_counter = 0;
static uint8_t                    sync_detected_counter = 0;
static uint32_t                   rx_started_counter = 0;       // used to determine the PRR
static uint32_t                   rx_successful_counter = 0;    // used to determine the PRR
static dcstat_t                   radio_dc_rx = { 0 };
static dcstat_t                   radio_dc_tx = { 0 };

/* debug counters for DIO1 interrupt path tracing */
static volatile uint32_t          dbg_execute_cnt = 0;
static volatile uint32_t          dbg_irq_capture_cnt = 0;
static volatile uint32_t          dbg_tx_done_cnt = 0;
static volatile uint32_t          dbg_exti4_cnt = 0;     /* EXTI4 DIO1 hits (PB4 direct) */
static volatile uint8_t           dbg_busy_before = 0;   /* BUSY pin right after NSS rise */
static volatile uint8_t           dbg_busy_after  = 0;   /* BUSY pin after short wait */
static volatile uint8_t           dbg_hw_status   = 0;   /* raw SX1280 status byte post-execute */

/* One producer (radio IRQ) and one consumer (LWB task) TX timing queue. */
#define RADIO_TX_TIMING_QUEUE_LEN  32U
static volatile radio_tx_timing_t tx_timing_queue[RADIO_TX_TIMING_QUEUE_LEN];
static volatile uint8_t           tx_timing_write_idx = 0;
static volatile uint8_t           tx_timing_read_idx  = 0;
static volatile uint32_t          tx_timing_sequence  = 0;
static volatile uint32_t          tx_marker_start_tick = 0;
static volatile uint8_t           tx_marker_payload_len = 0;
static volatile uint8_t           tx_payload_len = 0;
static volatile bool              tx_marker_active = false;

/* function pointers */
static radio_irq_cb_t     radio_irq_callback;
static radio_rx_cb_t      radio_rx_callback      = 0;
static radio_cad_cb_t     radio_cad_callback     = 0;
static radio_timeout_cb_t radio_timeout_callback = 0;
static radio_tx_cb_t      radio_tx_callback      = 0;

/* private callback functions */
void radio_irq_capture_cb(void);
void radio_timeout_cb(void);
void radio_cad_done_cb(_Bool detected);
void radio_rx_done_cb(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr, bool crc_error);
void radio_rx_error_cb(void);
void radio_rx_timeout_cb(void);
void radio_tx_done_cb(void);
void radio_tx_timeout_cb(void);
void radio_rx_sync_cb(void);
void radio_rx_preamble_cb(void);


static inline void radio_tx_marker_start(void)
{
  RADIO_TX_START_IND();
  tx_marker_start_tick   = hs_timer_get_counter();
  tx_marker_payload_len  = tx_payload_len;
  tx_marker_active       = true;
}


static inline void radio_tx_marker_stop(uint32_t end_tick, bool store)
{
  if (!tx_marker_active) {
    RADIO_TX_STOP_IND();
    return;
  }

  RADIO_TX_STOP_IND();
  tx_marker_active = false;

  if (store) {
    uint8_t next = (uint8_t)((tx_timing_write_idx + 1U) % RADIO_TX_TIMING_QUEUE_LEN);
    if (next != tx_timing_read_idx) {
      tx_timing_queue[tx_timing_write_idx].sequence   = ++tx_timing_sequence;
      tx_timing_queue[tx_timing_write_idx].ticks      = end_tick - tx_marker_start_tick;
      tx_timing_queue[tx_timing_write_idx].payload_len = tx_marker_payload_len;
      tx_timing_write_idx = next;
    }
  }
}


// restore basic radio configuration (e.g. after cold sleep)
static void radio_restore_config(bool reset)
{
  static RadioEvents_t radio_events;

  // perform a full radio reset?
  if (reset) {
    // assign callback functions for radio driver
    radio_events.CadDone    = radio_cad_done_cb;
    radio_events.RxDone     = radio_rx_done_cb;
    radio_events.RxError    = radio_rx_error_cb;
    radio_events.RxTimeout  = radio_rx_timeout_cb;
    radio_events.TxDone     = radio_tx_done_cb;
    radio_events.TxTimeout  = radio_tx_timeout_cb;
    radio_events.RxSync     = radio_rx_sync_cb;
    radio_events.RxPreamble = radio_rx_preamble_cb;

    // NOTE: Radio.Init() performs a hard reset of the SX1280 chip
    Radio.Init(&radio_events);

    // Set the SX1280 center frequency in the 2.4 GHz ISM band.
    Radio.SetChannel(radio_bands[RADIO_DEFAULT_BAND].centerFrequency);
  }

  // max LNA gain, increase current by ~2mA for around ~3dB in sensitivity
  if (rx_boosted) {
    radio_set_rx_gain(true);
  }
}

void radio_init(void)
{
  if (RADIO_READ_DIO1_PIN()) {
    LOG_WARNING("radio DIO1 pin is high at init");
  }

  radio_restore_config(true);

  hs_timer_capture(&radio_irq_capture_cb);
  radio_set_irq_direct(true);

  dcstat_reset(&radio_dc_rx);
  dcstat_reset(&radio_dc_tx);
  tx_timing_write_idx = 0;
  tx_timing_read_idx  = 0;
  tx_timing_sequence  = 0;
  tx_marker_active    = false;
  RADIO_TX_STOP_IND();
  RADIO_RX_STOP_IND();

  LOG_INFO("initialized");
}


void radio_set_irq_callback(void (*callback)())
{
  radio_irq_callback = callback;
}


void radio_set_irq_mode(lora_irq_mode_t mode)
{
  uint16_t radio_irq_mask = IRQ_RADIO_ALL;

  switch (mode)
  {
  case IRQ_MODE_ALL:
    radio_irq_mask = IRQ_RADIO_ALL;
    break;
  case IRQ_MODE_TX:
    radio_irq_mask = IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT;
    break;
  case IRQ_MODE_RX:
    radio_irq_mask = IRQ_HEADER_ERROR | IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID | IRQ_RX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0);
    break;
  case IRQ_MODE_RX_CRC:
    radio_irq_mask = IRQ_HEADER_ERROR | IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID | IRQ_RX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0) | IRQ_CRC_ERROR;
    break;
  case IRQ_MODE_RX_CRC_PREAMBLE:
    radio_irq_mask = IRQ_PREAMBLE_DETECTED | IRQ_HEADER_ERROR | IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID | IRQ_RX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0) | IRQ_CRC_ERROR;
    break;
  case IRQ_MODE_RX_PREAMBLE:
    radio_irq_mask = IRQ_PREAMBLE_DETECTED | IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID | IRQ_RX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0);
    break;
  case IRQ_MODE_RX_ONLY:
    radio_irq_mask = IRQ_RX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0);
    break;
  case IRQ_MODE_SYNC_RX_VALID:
    radio_irq_mask = IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID | IRQ_RX_DONE;
    break;
  case IRQ_MODE_SYNC_ONLY:
    radio_irq_mask = IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID;
    break;
  case IRQ_MODE_CAD:
    radio_irq_mask = IRQ_CAD_ACTIVITY_DETECTED | IRQ_CAD_DONE;
    break;
  case IRQ_MODE_CAD_RX:
    radio_irq_mask = IRQ_HEADER_ERROR | IRQ_HEADER_VALID | IRQ_RX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0);
    break;
  case IRQ_MODE_RX_TX:
    radio_irq_mask = IRQ_HEADER_ERROR | IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID | IRQ_RX_DONE | IRQ_TX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0);
    break;
  case IRQ_MODE_RX_TX_CRC:
    radio_irq_mask = IRQ_HEADER_ERROR | IRQ_HEADER_VALID | IRQ_SYNCWORD_VALID | IRQ_RX_DONE | IRQ_TX_DONE | (RADIO_USE_HW_TIMEOUT ? IRQ_RX_TX_TIMEOUT : 0) | IRQ_CRC_ERROR;
    break;
  default:
    break;
  }

  SX1280SetDioIrqParams(radio_irq_mask,
                        radio_irq_mask,
                        IRQ_RADIO_NONE,
                        IRQ_RADIO_NONE);
}


void radio_set_irq_direct(bool direct)
{
  radio_irq_direct = direct;
}


void radio_set_cad_callback(radio_cad_cb_t callback)
{
  radio_cad_callback = callback;
}


void radio_set_rx_callback(radio_rx_cb_t callback)
{
  radio_rx_callback = callback;
}


void radio_set_timeout_callback(radio_timeout_cb_t callback)
{
  radio_timeout_callback = callback;
}


void radio_set_tx_callback(radio_tx_cb_t callback)
{
  radio_tx_callback = callback;
}


void radio_sleep(bool warm)
{
  if (!radio_sleeping) {
    if (warm) {
      Radio.Sleep();
      radio_sleeping = RADIO_SLEEPING_WARM;
    }
    else {
      Radio.ColdSleep();
      radio_sleeping = RADIO_SLEEPING_COLD;
    }
    RADIO_TX_STOP_IND();
    RADIO_RX_STOP_IND();
    dcstat_stop(&radio_dc_rx);
    dcstat_stop(&radio_dc_tx);
  }
}


void radio_reset(void)
{
  RADIO_RX_STOP_IND();
  RADIO_TX_STOP_IND();
  dcstat_stop(&radio_dc_rx);
  dcstat_stop(&radio_dc_tx);
  radio_restore_config(true);   // calls Radio.Init() and performs a radio chip reset
}


bool radio_wakeup(void)
{
  if (radio_sleeping) {
    if (radio_sleeping == RADIO_SLEEPING_COLD) {
      /* radio config is lost and must be restored */
      radio_restore_config(true);
    } else {
      SX1280Wakeup();
      radio_restore_config(false);
    }
    radio_sleeping = RADIO_SLEEPING_FALSE;
    return true;
  }
  return false;
}


/* puts the radio into idle mode */
void radio_standby(void)
{
  if (radio_sleeping) {
    radio_wakeup();       // wake radio if it is still in sleep mode
  }

  // Temporarily force STDBY_RC before returning to the driver's standby path.
  SX1280SetStandby( STDBY_RC );
  Radio.Standby();

  RADIO_RX_STOP_IND();
  RADIO_TX_STOP_IND();
  dcstat_stop(&radio_dc_rx);
  dcstat_stop(&radio_dc_tx);
}


void radio_irq_capture_cb(void)
{
  dbg_irq_capture_cnt++;
  /* In TX mode DIO1 marks TX_DONE. CCR4 is captured by hardware when the
   * PB4-to-PB11 jumper is fitted, or snapshotted by the EXTI fallback. */
  if (tx_marker_active) {
    radio_tx_marker_stop((uint32_t)hs_timer_get_capture_timestamp(), true);
  }
  if (radio_irq_callback) {
    radio_irq_callback();
  }
  else {
    (*RadioOnDioIrqCallback)();

    if (radio_irq_direct) {
      Radio.IrqProcess();
    }
  }

#ifdef FLORA_DEBUG
  led_set_event_blink(0, 0);
#endif
}


/* EXTI4 fallback: SX1280 DIO1 connects to PB4 on DLP-RFS1280 module.
 * A jumper PB4→PB11 feeds TIM2_CH4 for hardware-captured timestamps,
 * but we ALSO process DIO1 from EXTI4 so that TxDone/RxDone callbacks
 * fire even if the jumper is missing or the TIM2 capture path fails.
 * When both paths fire, the second invocation finds IRQ already cleared
 * and returns safely (irqRegs == 0 in RadioIrqProcess). */
void GPIO_Radio_Callback(void)
{
  dbg_exti4_cnt++;
  hs_timer_trigger_capture_from_exti();
}


void radio_timeout_cb(void)
{
  radio_rx_callback = 0;

  if (radio_timeout_callback) {
    radio_timeout_cb_t tmp = radio_timeout_callback;
    radio_timeout_callback = 0;
    radio_standby();
    if(tmp) {
      tmp(false);
    }
  }

#ifdef FLORA_DEBUG
  CLI_LOG("MCU interrupt was triggered!", CLI_LOG_LEVEL_WARNING);
#endif
}


/**
 * SX1280 Callbacks
 */

void radio_cad_done_cb(bool detected)
{
  if (radio_cad_callback) {
    radio_set_timeout_callback(NULL);
    radio_cad_cb_t tmp = radio_cad_callback;
    radio_cad_callback = 0;
    if(tmp) {
      tmp(detected);
    }
  }
  else if (radio_timeout_callback && !detected) {
    radio_timeout_cb_t tmp = radio_timeout_callback;
    radio_timeout_callback = 0;
    if(tmp) {
      tmp(false);
    }
  }

#ifdef FLORA_DEBUG
  if (detected) {
    CLI_LOG("CAD detected signal!", CLI_LOG_LEVEL_INFO);
  }
  else {
    CLI_LOG("CAD detected NO signal!", CLI_LOG_LEVEL_INFO);
  }
#endif
}


void radio_rx_done_cb(uint8_t* payload, uint16_t size,  int16_t rssi, int8_t snr, bool crc_error)
{
  RADIO_RX_STOP_IND();
  dcstat_stop(&radio_dc_rx);
  if (!crc_error) {
    rx_successful_counter++;
  }
#if !RADIO_USE_HW_TIMEOUT
  hs_timer_timeout_stop();
#endif /* RADIO_USE_HW_TIMEOUT */

  if (radio_rx_callback) {
    radio_set_timeout_callback(NULL);

    radio_rx_cb_t tmp = radio_rx_callback;
    if (SX1280GetOperatingMode() != MODE_RX_CONTINUOUS) {
      radio_rx_callback = 0;
    }

    if (tmp) {
      tmp(payload, size, rssi, snr, crc_error);
    }
  }

#ifdef FLORA_DEBUG
  if (!crc_error) {
    radio_message_t* message = malloc(sizeof(radio_message_t));

    if (message != NULL) {
      uint8_t* message_payload = malloc(size);
      if (message_payload != NULL) {
        memcpy(message_payload, payload, size);

        message->payload = message_payload;
        message->size = size;
        message->rssi = rssi;
        message->snr = snr;
        message->next = NULL;

        if (last_message_list == NULL) {
          last_message_list = message;
        }
        else {
          radio_message_t* tmp = last_message_list;

          while (tmp->next != NULL) {
            tmp = tmp->next;
          }

          tmp->next = message;
        }
      }
      else {
        free(message);
      }
    }
  }
  else {
    LOG_ERROR("CRC Error on message reception");
  }
#endif
}


void radio_rx_error_cb(void)
{
  RADIO_RX_STOP_IND();
  dcstat_stop(&radio_dc_rx);

#if !RADIO_USE_HW_TIMEOUT
  hs_timer_timeout_stop();
#endif /* RADIO_USE_HW_TIMEOUT */

  radio_timeout_cb_t tmp = radio_timeout_callback;
  if (SX1280GetOperatingMode() != MODE_RX_CONTINUOUS) {
    radio_timeout_callback = 0;
  }
  if(tmp) {
    tmp(true);
  }

#ifdef FLORA_DEBUG
  LOG_WARNING("CRC Error Timeout");
#endif
}


void radio_rx_timeout_cb(void)
{
  RADIO_RX_STOP_IND();
  dcstat_stop(&radio_dc_rx);

#if !RADIO_USE_HW_TIMEOUT
  hs_timer_timeout_stop();
#endif /* RADIO_USE_HW_TIMEOUT */

  radio_timeout_cb_t tmp = radio_timeout_callback;
  radio_timeout_callback = 0;
  if(tmp) {
    tmp(false);
  }

#ifdef FLORA_DEBUG
  LOG_WARNING("Rx Timeout");
#endif
}


void radio_rx_sync_cb(void)
{
  radio_last_sync_timestamp = hs_timer_get_capture_timestamp();
  if (sync_detected_counter < 255) {
    sync_detected_counter++;
  }
  rx_started_counter++;
}


void radio_rx_preamble_cb(void)
{
  if (preamble_detected_counter < 255) {
    preamble_detected_counter++;
  }
}


void radio_tx_done_cb(void)
{
  dbg_tx_done_cnt++;
  radio_tx_marker_stop(hs_timer_get_counter(), false);
  dcstat_stop(&radio_dc_tx);

  radio_set_timeout_callback(NULL);

  radio_tx_cb_t tmp = radio_tx_callback;
  radio_tx_callback = 0;
  if (tmp) {
    tmp();
  }
}


void radio_tx_timeout_cb(void)
{
  radio_tx_marker_stop(hs_timer_get_counter(), false);
  dcstat_stop(&radio_dc_tx);

  radio_timeout_cb_t tmp = radio_timeout_callback;
  radio_timeout_callback = 0;
  if (tmp) {
    tmp(false);
  }
}


/**
 * RX / TX functions
 */

static void radio_execute(void)
{
  dbg_execute_cnt++;
  RADIO_SET_NSS_PIN();

  /* NOTE: Radio.GetStatus() only reads cached operating mode — no SPI access.
   * Do NOT call SX1280GetStatus() or any SPI function here!
   * This runs in TIM2 ISR context; SPI from ISR corrupts the bus. */
  switch (Radio.GetStatus()) {
    case RF_RX_RUNNING:
      RADIO_RX_START_IND();
      dcstat_start(&radio_dc_rx);
      break;
    case RF_TX_RUNNING:
      RADIO_RX_STOP_IND();
      radio_tx_marker_start();
      dcstat_start(&radio_dc_tx);
      break;
    default:
      break;
  }

  hs_timer_schedule_stop();
}


void radio_transmit(uint8_t* buffer, uint8_t size)
{
  if (radio_sleeping) return;      // abort if radio is still in sleep mode

  if (buffer) {
    radio_set_payload(buffer, size);
  }
  Radio.Tx(0, false);
  hs_timer_set_schedule_timestamp(hs_timer_get_counter());
  RADIO_RX_STOP_IND();
  radio_tx_marker_start();
  dcstat_start(&radio_dc_tx);
}


void radio_transmit_scheduled(uint8_t* buffer, uint8_t size, uint64_t schedule_timestamp_hs)
{
  if (radio_sleeping) return;      // abort if radio is still in sleep mode

  if (buffer) {
    radio_set_payload(buffer, size);
  }
  Radio.Tx(0, true);
  hs_timer_schedule_start(schedule_timestamp_hs, &radio_execute);
}


void radio_execute_manually(int64_t timestamp_hs)
{
  if (RADIO_READ_NSS_PIN() == 0) {    // command scheduled?
    if (timestamp_hs < 0) {
      hs_timer_set_schedule_timestamp(hs_timer_get_counter());
      radio_execute();
    }
    else {
      hs_timer_schedule_start(timestamp_hs, &radio_execute);
    }
  }
}


void radio_receive_scheduled(uint64_t schedule_timestamp_hs, uint32_t timeout_hs)
{
  uint32_t timeout_ms = 0;

  if (radio_sleeping) return;      // abort if radio is still in sleep mode

#if RADIO_USE_HW_TIMEOUT
  // convert to milliseconds
  timeout_ms = HS_TIMER_TICKS_TO_MS(timeout_hs);
  if (timeout_ms > RADIO_TIMER_MAX_TIMEOUT_MS) {
    timeout_ms = RADIO_TIMER_MAX_TIMEOUT_MS;        // set to max. allowed value
  }
#else  /* RADIO_USE_HW_TIMEOUT */
  if (timeout_hs) {
    hs_timer_timeout_start(schedule_timestamp_hs + timeout_hs, &radio_timeout_cb);
  }
#endif /* RADIO_USE_HW_TIMEOUT */

  Radio.Rx(timeout_ms, false, true);

  hs_timer_schedule_start(schedule_timestamp_hs, &radio_execute);
}


void radio_receive(uint32_t timeout_hs)
{
  uint32_t timeout_ms = 0;

  if (radio_sleeping) return;      // abort if radio is still in sleep mode

#if RADIO_USE_HW_TIMEOUT
  // convert to milliseconds
  timeout_ms = HS_TIMER_TICKS_TO_MS(timeout_hs);
  if (timeout_ms > RADIO_TIMER_MAX_TIMEOUT_MS) {
    timeout_ms = RADIO_TIMER_MAX_TIMEOUT_MS;        // set to max. allowed value
  }
#else  /* RADIO_USE_HW_TIMEOUT */
  if (timeout_hs) {
    hs_timer_timeout_start(hs_timer_get_current_timestamp() + timeout_hs, &radio_timeout_cb);
  }
#endif /* RADIO_USE_HW_TIMEOUT */

  Radio.Rx(timeout_ms, false, false);
  RADIO_RX_START_IND();
  dcstat_start(&radio_dc_rx);
}


void radio_receive_continuously(void)
{
  if (radio_sleeping) return;      // abort if radio is still in sleep mode

  Radio.Rx(0, true, false);
  RADIO_RX_START_IND();
  dcstat_start(&radio_dc_rx);
}


void radio_receive_duty_cycle(uint32_t rx, uint32_t sleep, bool schedule)
{
  if (radio_sleeping) return;      // abort if radio is still in sleep mode

  Radio.SetRxDutyCycle(rx, sleep, schedule);
}


void radio_set_rx_gain(bool rx_boost)
{
  rx_boosted = rx_boost;
  if (rx_boosted) {
    SX1280WriteRegister( REG_RX_GAIN, 0x96 ); // max LNA gain, increase current by ~2mA for around ~3dB in sensitivity
  } else {
    SX1280WriteRegister( REG_RX_GAIN, 0x94 ); // default gain
  }
}


/**
 * functions to query / reset stats
 */

uint64_t radio_get_last_sync_timestamp(void)
{
  return radio_last_sync_timestamp;
}


void radio_reset_preamble_counter(void)
{
  preamble_detected_counter = 0;
}


uint8_t radio_get_preamble_counter(void)
{
  return preamble_detected_counter;
}


void radio_reset_sync_counter(void)
{
  sync_detected_counter = 0;
}


uint8_t radio_get_sync_counter(void)
{
  return sync_detected_counter;
}


uint32_t radio_get_rx_dc(void)
{
  return dcstat_get_dc(&radio_dc_rx);
}


uint64_t radio_get_rx_time(void)
{
  return dcstat_get_active_time(&radio_dc_rx);
}


uint32_t radio_get_tx_dc(void)
{
  return dcstat_get_dc(&radio_dc_tx);
}


uint64_t radio_get_tx_time(void)
{
  return dcstat_get_active_time(&radio_dc_tx);
}


void radio_dc_counter_reset(void)
{
  dcstat_reset(&radio_dc_rx);
  dcstat_reset(&radio_dc_tx);
}


uint32_t radio_get_prr(bool reset)
{
  uint32_t prr = 0;
  if (rx_started_counter) {
    prr = 10000 * rx_successful_counter / rx_started_counter;
  }
  if (reset) {
    rx_successful_counter = rx_started_counter = 0;
  }
  return prr;
}


void radio_tx_timing_set_payload_len(uint8_t payload_len)
{
  tx_payload_len = payload_len;
}


bool radio_tx_timing_pop(radio_tx_timing_t* timing)
{
  if (!timing || tx_timing_read_idx == tx_timing_write_idx) {
    return false;
  }

  ENTER_CRITICAL_SECTION();
  if (tx_timing_read_idx == tx_timing_write_idx) {
    LEAVE_CRITICAL_SECTION();
    return false;
  }
  timing->sequence    = tx_timing_queue[tx_timing_read_idx].sequence;
  timing->ticks       = tx_timing_queue[tx_timing_read_idx].ticks;
  timing->payload_len = tx_timing_queue[tx_timing_read_idx].payload_len;
  tx_timing_read_idx  = (uint8_t)((tx_timing_read_idx + 1U) % RADIO_TX_TIMING_QUEUE_LEN);
  LEAVE_CRITICAL_SECTION();
  return true;
}


void radio_dbg_get_counters(uint32_t* execute, uint32_t* irq_capture, uint32_t* tx_done)
{
  if (execute)     *execute     = dbg_execute_cnt;
  if (irq_capture) *irq_capture = dbg_irq_capture_cnt;
  if (tx_done)     *tx_done     = dbg_tx_done_cnt;
}

uint32_t radio_dbg_get_exti4_cnt(void)
{
  return dbg_exti4_cnt;
}

void radio_dbg_get_hw_state(uint8_t* busy_before, uint8_t* busy_after, uint8_t* hw_status)
{
  if (busy_before) *busy_before = dbg_busy_before;
  if (busy_after)  *busy_after  = dbg_busy_after;
  if (hw_status)   *hw_status   = dbg_hw_status;
}

void radio_dbg_reset_counters(void)
{
  dbg_execute_cnt     = 0;
  dbg_irq_capture_cnt = 0;
  dbg_tx_done_cnt     = 0;
  dbg_exti4_cnt       = 0;
}

#endif /* RADIO_ENABLE */
