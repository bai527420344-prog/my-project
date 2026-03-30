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
 * generates data packets
 */

#include "main.h"


extern QueueHandle_t xQueueHandle_tx;

uint32_t data_period = DATA_GENERATION_PERIOD;


/* Private define ------------------------------------------------------------*/

#ifndef PRE_TASK_RESUMED
#define PRE_TASK_RESUMED()
#define PRE_TASK_SUSPENDED()
#endif /* PRE_TASK_RESUMED */


/* Private variables ---------------------------------------------------------*/


/* Functions -----------------------------------------------------------------*/

void generate_data_pkt(void)
{
  static dpp_message_t msg_buffer;

  /* generate a dummy packet (header only, no data) */
  if (ps_compose_msg(DPP_DEVICE_ID_SINK, DPP_MSG_TYPE_INVALID, 0, 0, &msg_buffer) &&
      xQueueSend(xQueueHandle_tx, &msg_buffer, 0)) {
    LOG_INFO("data packet generated");
  } else {
    LOG_WARNING("failed to insert message into transmit queue");
  }
}


void vTask_pre(void const * argument)
{
  static uint32_t last_pkt = 0;

  LOG_VERBOSE("pre task started");

  generate_data_pkt();

  /* Infinite loop */
  for(;;)
  {
    PRE_TASK_SUSPENDED();
    xTaskNotifyWait(0, ULONG_MAX, NULL, portMAX_DELAY);
    PRE_TASK_RESUMED();

    /* generate a node info message if necessary (must be here) */
    if (data_period) {
      /* only send other messages once the node info msg has been sent! */
      uint64_t network_time = 0;
      lwb_get_last_syncpoint(&network_time, 0);
      uint32_t div = (network_time / (1000000 * data_period));
      if (div != last_pkt) {
        /* generate a dummy packet (header only, no data) */
        generate_data_pkt();
        last_pkt = div;
      }
    }
  }
}
