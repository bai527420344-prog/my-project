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
 * event types specific to SX1262
 */

/* Label ----------------------- | #No. | Description ---------------------- | Value ---*/

EVENT_SX1262_CMD_EXECUTED,      /* 0x01 | command executed (ACK)             | cmd type without component ID (16-bit) + argument (16-bit) */
EVENT_SX1262_TIME_UPDATED,      /* 0x02 | local time has been adjusted       | time delta / jump in ms (32-bit signed) */
EVENT_SX1262_QUEUE_FULL,        /* 0x03 | queue full, message dropped        | queue ID (32-bit) */
EVENT_SX1262_INV_MSG,           /* 0x04 | invalid message received           | sender device ID (16-bit) + message length in bytes (16-bit) */
EVENT_SX1262_INV_CMD,           /* 0x05 | invalid command received           | cmd type (16-bit) + argument (16-bit) */
EVENT_SX1262_MSG_IGNORED,       /* 0x06 | unknown message type, ignored      | sender device ID (16-bit) + msg type (16-bit) */
EVENT_SX1262_RADIO_ERROR,       /* 0x07 | radio error                        | error code (32-bit) */
EVENT_SX1262_BOLT_ERROR,        /* 0x08 | bolt communication error           | error code (32-bit) */
EVENT_SX1262_STACK_WM,          /* 0x09 | stack watermark notification       | watermark (16-bit) + task ID (16-bit) */
EVENT_SX1262_HOST_ID_ERROR,     /* 0x0a | error related to the host ID       | error code (32-bit) */
EVENT_SX1262_SCHED_MISSED,      /* 0x0b | schedule packet missed             | error code (32-bit) */
EVENT_SX1262_MUTEX_ERROR,       /* 0x0c | error related to mutual exclusion (semaphores, critical sections) | error code (32-bit) */
EVENT_SX1262_TSYNC_ERROR,       /* 0x0d | error or warning related to time synchronization                  | error code (32-bit) */
EVENT_SX1262_TSYNC_DRIFT,       /* 0x0e | high drift warning                                                | drift in ppm (signed 32-bit) */
EVENT_SX1262_RTC_ERROR,         /* 0x0f | error related to the RTC                                          | error code (32-bit) */
EVENT_SX1262_TIMEOUT,           /* 0x10 | generic timeout error                                             | error code (32-bit) */
EVENT_SX1262_RESET,             /* 0x11 | device reset occurred              | reset cause (16-bit) + counter (16-bit) */
EVENT_SX1262_TX_FAILED,         /* 0x12 | transmission failed (e.g. due to channel busy or no ACK received) | error code (32-bit) */
EVENT_SX1262_FW_OTA_STATUS,     /* 0x13 | firmware update status             | progress in percent (32-bit) */
EVENT_SX1262_CMD_FORWARDED,     /* 0x14 | command forwarded                  | target ID (16-bit) + cmd type (16-bit) */
