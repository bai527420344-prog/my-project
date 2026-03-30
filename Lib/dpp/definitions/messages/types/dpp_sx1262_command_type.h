/*
 * Copyright (c) 2017 - 2021, ETH Zurich, Computer Engineering Group (TEC)
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
 * command types specific to SX1262
 *
 *
 * typedef enum {
 *   ...
 *   DPP_COMMAND_[device_name]  = [start_id],
 *   [your_defines]
 *   ...
 * } dpp_command_type_t;
 *
 * add a comma after each define!
 */

/* Label ----------------------------- | #No. | Description ----------------------------*/

/* Event-driven approach events */
CMD_SX1262_CONF_WKUP,                 /* 0x01 | Change radio configurations for staggered wakeup */                       /* @param: arg32[0]: LoRa/FSK (1 bit), Spreading Factor (15 bits), Bandwidth (16 bits); arg32[1]: Code Rate (8 bits), Tx Power (8 bits), Preamble (16 bits) */
CMD_SX1262_STAG_RX_MODE,              /* 0x02 | Change staggered wakeup mode */                                           /* @param: ALWAYS_RX = 0, LORA_CAD = 1, LORA_DUTY_CYCLE_CAD_STOPMODE = 2 */
CMD_SX1262_CONF_BS,                   /* 0x03 | Change radio configurations for sending to base station*/                 /* @param: arg32[0]: LoRa/FSK (1 bit), Spreading Factor (15 bits), Bandwidth (16 bits); arg32[1]: Code Rate (8 bits), Tx Power (8 bits), Preamble (16 bits) */
CMD_SX1262_CONF_ELECTION,             /* 0x04 | Change radio configurations for leader election and data aggregation */   /* @param: arg32[0]: LoRa/FSK (1 bit), Spreading Factor (15 bits), Bandwidth (16 bits); arg32[1]: Code Rate (8 bits), Tx Power (8 bits), Preamble (16 bits) */
CMD_SX1262_ELECTION_MODUS,            /* 0x05 | Change the leader election modus */                                       /* @param: leader_election_modi_t */
CMD_SX1262_STOP_MODE,                 /* 0x06 | Change to stop mode 2 for given duration */                               /* @param: Duration after which the node should wake up, in ms */
CMD_SX1262_CONF_EVENT_ADD,            /* 0x07 | Add new event configuration */                                            /* @param: TBD */
CMD_SX1262_CONF_EVENT_DEL,            /* 0x08 | Delete event configuration */                                             /* @param: TBD */
CMD_SX1262_SET_CODETECTION_THRESHOLD, /* 0x09 | Set the codetection threshold for reporting */                            /* @param: number of nodes that need to detect the same events */
CMD_SX1262_SET_CAD_MODE,              /* 0x0A | Set channel activity detection mode */                                    /* @param: mode (0 = disable) */
CMD_SX1262_SET_HEALTH_MSG_PRIORITY,   /* 0x0B | Set the priority for health message generation */                         /* @param: 0 = low priority, 1 = high priority */
CMD_SX1262_SET_STAG_WAKEUP_TIMEOUT,   /* 0x0C | Set the timeout for the staggered wakeup, in ms */                        /* @param: timeout in ms [16-bit] */

CMD_SX1262_RESERVED_0D,
CMD_SX1262_RESERVED_0E,
CMD_SX1262_RESERVED_0F,

CMD_SX1262_SET_NODE_ID,               /* 0x10 | Set the node ID */                     /* @param: 16-bit node ID */
CMD_SX1262_RESERVED_11,
CMD_SX1262_FW_OTA_MODE,               /* 0x12 | enter FW update mode */                /* @param: 16-bit key */
CMD_SX1262_RESET,                     /* 0x13 | perform a software reset */

/* Baseboard commands */
CMD_SX1262_BASEBOARD_ENABLE,          /* 0x14 | Schedule baseboard enable */           /* @param: enable time (32-bit timestamp), time flag (0 = absolute time as UNIX timestamp, 1 = relative time in seconds), wakeup command for baseboard (2 bytes)  */
CMD_SX1262_BASEBOARD_DISABLE,         /* 0x15 | Schedule baseboard disable */          /* @param: disable time (32-bit timestamp), time flag (0 = absolute time as UNIX timestamp, 1 = relative time in seconds) */
CMD_SX1262_BASEBOARD_ENABLE_PERIODIC, /* 0x16 | Schedule periodic baseboard enable */  /* @param: enable time hour [0..23] (1 byte), enable time minute [0..59] (1 byte), period in minutes (2 bytes, set to 0 to disable), wakeup command for baseboard (2 bytes)  */
CMD_SX1262_BASEBOARD_POWER_EXT3,      /* 0x17 | Enable or disable the power on port EXT3 */  /* @param: 1 byte (0 = disable, 1 = enable) */
CMD_SX1262_RESERVED_18,
CMD_SX1262_RESERVED_19,
CMD_SX1262_RESERVED_1A,
CMD_SX1262_RESERVED_1B,
CMD_SX1262_RESERVED_1C,
CMD_SX1262_RESERVED_1D,
CMD_SX1262_RESERVED_1E,
CMD_SX1262_RESERVED_1F,

/* eLWB / network / data collection commands */
CMD_SX1262_SET_COM_PERIOD,            /* 0x20 | Set the communication base period (only on the host node) */               /* @param: period in seconds (16-bit) */
CMD_SX1262_SET_HEALTH_PERIOD,         /* 0x21 | Set the periodicity of the node health messages */                         /* @param: period in seconds (16-bit) */
CMD_SX1262_SET_EVENT_LEVEL,           /* 0x22 | Set event message level */                                                 /* @param: level of type event_msg_level_t (1 byte) */
CMD_SX1262_REGISTER_NODE,             /* 0x23 | Register a node in the network (only a host node accepts this command) */  /* @param: node ID to register */
CMD_SX1262_RESERVED_24,
CMD_SX1262_RESERVED_25,
CMD_SX1262_RESERVED_26,
CMD_SX1262_RESERVED_27,
CMD_SX1262_RESERVED_28,
CMD_SX1262_RESERVED_29,
CMD_SX1262_RESERVED_2A,
CMD_SX1262_RESERVED_2B,
CMD_SX1262_RESERVED_2C,
CMD_SX1262_RESERVED_2D,
CMD_SX1262_RESERVED_2E,
CMD_SX1262_RESERVED_2F,

/* radio commands */
CMD_SX1262_SET_TX_POWER,              /* 0x30 | Set the radio transmit power */   /* @param: TX power in dBm (16-bit, signed) */
CMD_SX1262_SET_MODULATION,            /* 0x31 | Set the radio modulation */       /* @param: modulation table index according to radio_constants.c (1 byte) */

/* Label ----------------------------- | #No. | Description ----------------------------*/
