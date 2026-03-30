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
 * command types specific to BaseBoard (Colibri application processor)
 *
 * add a comma after each line, don't assign values/numbers to the enums
 */

/* Label ------------------------- | #No. | Description ---------------------*/

/* system commands */
CMD_BASEBOARD_POWEROFF,           /* 0x01 | perform soft shutdown and power off */ /* @param 0: immediate, 1: wait if busy (1 byte) */
CMD_BASEBOARD_SUSPEND,            /* 0x02 | suspend system */                      /* @param none */
CMD_BASEBOARD_WAKEUP_MODE,        /* 0x03 | wakeup mode */                         /* @param wakeup mode (2 bytes) */
CMD_BASEBOARD_MSG_ROUTING,        /* 0x04 | DPP message routing */                 /* @param 0: to GSN, 1: to LWB network, 2: both (to GSN and LWB network) */

CMD_BASEBOARD_RESERVED_05,
CMD_BASEBOARD_RESERVED_06,
CMD_BASEBOARD_RESERVED_07,
CMD_BASEBOARD_RESERVED_08,
CMD_BASEBOARD_RESERVED_09,
CMD_BASEBOARD_RESERVED_0A,
CMD_BASEBOARD_RESERVED_0B,
CMD_BASEBOARD_RESERVED_0C,
CMD_BASEBOARD_RESERVED_0D,
CMD_BASEBOARD_RESERVED_0E,
CMD_BASEBOARD_RESERVED_0F,

/* power commands */
CMD_BASEBOARD_PWR_WLAN,           /* 0x10 | wlan en/disable */                 /* @param 0: off, 1: on */
CMD_BASEBOARD_PWR_EXT1,           /* 0x11 | ext1 port en/disable */            /* @param 0: off, 1: on */
CMD_BASEBOARD_PWR_EXT2,           /* 0x12 | ext2 port en/disable */            /* @param 0: off, 1: on */
CMD_BASEBOARD_PWR_EXT3,           /* 0x13 | ext3 port en/disable */            /* @param 0: off, 1: on */
CMD_BASEBOARD_PWR_POE,            /* 0x14 | PoE en/disable */                  /* @param 0: off, 1: on */
CMD_BASEBOARD_PWR_USB,            /* 0x15 | USB port en/disable */             /* @param 0: off, 1: on */
CMD_BASEBOARD_PWR_GNSS,           /* 0x16 | GNSS receiver en/disable */        /* @param 0: off, 1: on */
CMD_BASEBOARD_PWR_MODEM,          /* 0x17 | GPRS modem en/disable */           /* @param 0: off, 1: on */

/* last symbol in this file must be a comma */

