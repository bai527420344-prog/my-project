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
 * processes the packets received from the network
 */

#include "main.h"


extern QueueHandle_t xQueueHandle_rx;


/* Private define ------------------------------------------------------------*/

#ifndef POST_TASK_RESUMED
#define POST_TASK_RESUMED()
#define POST_TASK_SUSPENDED()
#endif /* POST_TASK_IND */


/* Private variables ---------------------------------------------------------*/


/* Functions -----------------------------------------------------------------*/

void vTask_post(void const * argument)
{
  static dpp_message_t msg_buffer;

  LOG_VERBOSE("post task started");

  /* Infinite loop */
  for(;;)
  {
    POST_TASK_SUSPENDED();
    xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
    POST_TASK_RESUMED();

    /* process all packets rcvd from the network (regardless of whether there is space in the BOLT queue) */
    uint16_t rcvd = 0;
    while (xQueueReceive(xQueueHandle_rx, (void*)&msg_buffer, 0)) {
      if (!ps_validate_msg(&msg_buffer)) {
        LOG_WARNING("invalid message received from node %u (type: %u  length: %u)", msg_buffer.header.device_id, msg_buffer.header.type, msg_buffer.header.payload_len);
      }
      //TODO message processing
      rcvd++;
    }
    if (rcvd) {
      LOG_INFO("%u msg rcvd from network", rcvd);
    }

    /* check for critical stack usage or overflow */
    rtos_check_stack_usage();

    /* print some stats */
    LOG_INFO("CPU duty cycle:  %u%%    radio duty cycle (rx/tx):  %uppm/%uppm", (uint16_t)rtos_get_cpu_dc() / 100, radio_get_rx_dc(), radio_get_tx_dc());

    /* flush the log print queue */
#if !LOG_PRINT_IMMEDIATELY
    log_flush();
#endif /* LOG_PRINT_IMMEDIATELY */

    /* before telling the state machine to enter low-power mode, wait for the UART transmission to complete (must be done here, NOT in lpm_prepare) */
#if LOG_USE_DMA
    uart_wait_tx_complete(100);     // 100ms timeout
#endif /* LOG_USE_DMA */

    /* round finished, prepare for low-power mode */
    lpm_update_opmode(OP_MODE_EVT_DONE);
  }
}
