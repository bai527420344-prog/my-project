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
 * event types specific to the BaseBoard (Colibri application processor)
 *
 * add a comma after each line, don't assign values/numbers to the enums
 */

/* Label ------------------------------ | #No. | Description ---------------------- | Value ---*/

/* system notifications */
EVENT_BASEBOARD_RST,                   /* 0x01 | a reset has occurred               | reset cause  */
EVENT_BASEBOARD_SUSPEND,               /* 0x02 | board suspending                   |  */
EVENT_BASEBOARD_WAKEUP,                /* 0x03 | board wakeup from suspend          |  */
EVENT_BASEBOARD_MSG_ROUTING_CHANGED,   /* 0x04 | DPP message routing changed        | routing mode */

EVENT_BASEBOARD_RESERVED_05,
EVENT_BASEBOARD_RESERVED_06,
EVENT_BASEBOARD_RESERVED_07,
EVENT_BASEBOARD_RESERVED_08,
EVENT_BASEBOARD_RESERVED_09,
EVENT_BASEBOARD_RESERVED_0A,
EVENT_BASEBOARD_RESERVED_0B,
EVENT_BASEBOARD_RESERVED_0C,
EVENT_BASEBOARD_RESERVED_0D,
EVENT_BASEBOARD_RESERVED_0E,
EVENT_BASEBOARD_RESERVED_0F,

/* BackLog notifications */
EVENT_BASEBOARD_BACKLOG_START,         /* 0x10 | BackLog started                    | start unixtime  */
EVENT_BASEBOARD_BACKLOG_STOP,          /* 0x11 | BackLog stopped                    | stop unixtime */
EVENT_BASEBOARD_BACKLOG_CFG_CHANGED,   /* 0x12 | config has changed                 | config file md5 hash */

EVENT_BASEBOARD_RESERVED_13,
EVENT_BASEBOARD_RESERVED_14,
EVENT_BASEBOARD_RESERVED_15,
EVENT_BASEBOARD_RESERVED_16,
EVENT_BASEBOARD_RESERVED_17,
EVENT_BASEBOARD_RESERVED_18,
EVENT_BASEBOARD_RESERVED_19,
EVENT_BASEBOARD_RESERVED_1A,
EVENT_BASEBOARD_RESERVED_1B,
EVENT_BASEBOARD_RESERVED_1C,
EVENT_BASEBOARD_RESERVED_1D,
EVENT_BASEBOARD_RESERVED_1E,
EVENT_BASEBOARD_RESERVED_1F,

/* errors and warnings */
EVENT_BASEBOARD_BACKLOG_ERROR,         /* 0x20 | radio protocol error (glossy, LWB) | error code */
EVENT_BASEBOARD_BOLT_ERROR,            /* 0x21 | bolt communication error           | error code */
EVENT_BASEBOARD_QUEUE_FULL,            /* 0x22 | queue full, message dropped        |  */
EVENT_BASEBOARD_INV_MSG,               /* 0x23 | invalid message received           | error code */
EVENT_BASEBOARD_INV_CMD,               /* 0x24 | invalid command received           | cmd type and value */
EVENT_BASEBOARD_MSG_IGNORED,           /* 0x25 | unknown message type, ignored      | msg type */

/* last symbol in this file must be a comma */

