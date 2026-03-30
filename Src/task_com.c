/*
 * Copyright (c) 2021, ETH Zurich, Computer Engineering Group (TEC)
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

/*
 * communication task, runs the LWB protocol and controls the execution of the pre and post tasks
 */

#include "main.h"


/* Private variables ---------------------------------------------------------*/

extern TaskHandle_t  xTaskHandle_pre;
extern TaskHandle_t  xTaskHandle_post;
extern QueueHandle_t xQueueHandle_rx;
extern QueueHandle_t xQueueHandle_tx;
extern QueueHandle_t xQueueHandle_retx;

/* global variables for binary patching the config */
volatile uint16_t         host_id           = HOST_ID;
static volatile int8_t    gloria_power      = GLORIA_INTERFACE_POWER;
static volatile uint8_t   gloria_modulation = GLORIA_INTERFACE_MODULATION;
static volatile uint8_t   gloria_band       = GLORIA_INTERFACE_RF_BAND;
static volatile uint8_t   lwb_n_tx          = LWB_N_TX;
static volatile uint8_t   lwb_num_hops      = LWB_NUM_HOPS;
static volatile uint32_t  lwb_period        = LWB_SCHED_PERIOD;

extern uint32_t data_period;


void listen_timeout(void)
{
  /* nothing to do */
}


#if COLLECT_FLOODING_DATA

void collect_radio_stats(uint16_t initiator_id, lwb_phases_t lwb_phase, lwb_packet_t* packet)
{
  if (initiator_id != NODE_ID) {
    /* check if schedule packet is valid (there are sporadic cases of SCHED1 packets receptions which have a valid PHY CRC but are not valid LWB packets) */
    if ((lwb_phase == LWB_PHASE_SCHED1) && !LWB_IS_SCHEDULE_PACKET(packet)) {
      return;
    }

    uint8_t  rx_cnt        = gloria_get_rx_cnt();
    uint8_t  rx_started    = gloria_get_rx_started_cnt();
    uint8_t  rx_idx        = 0;
    int8_t   snr           = -99;
    int16_t  rssi          = -99;
    uint8_t  payload_len   = 0;
    uint8_t  t_ref_updated = 0;
    uint64_t network_time  = 0;
    uint64_t t_ref         = 0;

    if (rx_cnt > 0 && LWB_IS_PKT_HEADER_VALID(packet)) {
      rx_idx         = gloria_get_rx_index();
      snr            = gloria_get_snr();
      rssi           = gloria_get_rssi();
      payload_len    = gloria_get_payload_len();
      t_ref_updated  = gloria_is_t_ref_updated();
      if (t_ref_updated) {
        lwb_get_last_syncpoint(&network_time, &t_ref);
      }
    }

    /* print in json format */
    LOG_INFO("{"
             "\"initiator_id\":%d,"
             "\"lwb_phase\":%d,"
             "\"rx_cnt\":%d,"
             "\"rx_idx\":%d,"
             "\"rx_started\":%d,"
             "\"rssi\":%d,"
             "\"snr\":%d,"
             "\"payload_len\":%d,"
             "\"t_ref_updated\":%llu,"
             "\"network_time\":%llu,"
             "\"t_ref\":%llu"
             "}",
      initiator_id,
      lwb_phase,
      rx_cnt,
      rx_idx,
      rx_started,
      rssi,
      snr,
      payload_len,
      t_ref_updated,
      network_time,
      t_ref
    );
  }
}

#endif /* COLLECT_FLOODING_DATA */


/* communication task */
void vTask_com(void const * argument)
{
  LOG_VERBOSE("com task started");

  /* make sure the radio is awake */
  radio_wakeup();

  /* set gloria config values */
  gloria_set_tx_power(gloria_power);
  gloria_set_modulation(gloria_modulation);
  gloria_set_band(gloria_band);

  /* set LWB config values */
  if (lwb_sched_set_period(lwb_period)) { // Note: period needs to be larger than max round duration (based on current values of )
    LOG_INFO("LWB successfully set period to %lus", lwb_period);
  } else {
    LOG_WARNING("LWB rejects setting period to %lus", lwb_period);
  }
  if (lwb_set_n_tx(lwb_n_tx)) {  // Note: Configured period needs to be large enough!
    LOG_INFO("LWB successfully set n_tx to %u", lwb_n_tx);
  } else {
    LOG_WARNING("LWB rejects setting n_tx to %u", lwb_n_tx);
  }
  if (lwb_set_num_hops(lwb_num_hops)) {  // Note: Configured period needs to be large enough!
    LOG_INFO("LWB successfully set num_hops to %u", lwb_num_hops);
  } else {
    LOG_WARNING("LWB rejects setting num_hops to %u", lwb_num_hops);
  }

  /* init LWB */
  if (!lwb_init(xTaskGetCurrentTaskHandle(), xTaskHandle_pre, xTaskHandle_post, xQueueHandle_rx, xQueueHandle_tx, xQueueHandle_retx, listen_timeout, IS_HOST)) {
    FATAL_ERROR("LWB init failed");
  }
  //lwb_set_ipi(data_period);

#if COLLECT_FLOODING_DATA
  lwb_register_slot_callback(collect_radio_stats);

  /* print config in json format */
  LOG_INFO("{"
           "\"node_id\":%d,"
           "\"host_id\":%d,"
           "\"tx_power\":%d,"
           "\"modulation\":%d,"
           "\"rf_band\":%d,"
           "\"n_tx\":%d,"
           "\"num_hops\":%d,"
           "\"lwb_pkt_len\":%d,"
           "\"lwb_num_slots\":%d,"
           "\"lwb_period\":%lu,"
           "\"health_msg_period\":%lu"
           "}",
    NODE_ID,
    host_id,
    gloria_power,
    gloria_modulation,
    gloria_band,
    lwb_get_n_tx(),
    lwb_get_num_hops(),
    LWB_MAX_PAYLOAD_LEN,
    LWB_MAX_DATA_SLOTS,
    lwb_sched_get_period(),
    data_period
  );
#endif /* COLLECT_FLOODING_DATA */

  while (lptimer_now() < LPTIMER_SECOND);   /* wait until 1s has elapsed since MCU startup */

  /* start LWB (blocking call, should never return) */
  lwb_start();

  FATAL_ERROR("LWB task terminated");
}
