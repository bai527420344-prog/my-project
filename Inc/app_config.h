/*
 * Copyright (c) 2021 - 2022, ETH Zurich, Computer Engineering Group (TEC)
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
 * Flora "Low-power Wireless Bus" config
 */

#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H


/* --- adjustable parameters --- */

/* general */
#define FLOCKLAB                        0           /* set to 1 to run on FlockLab */
#define FLOCKLAB_SWD                    0           /* set to 1 to reserve SWDIO / SWDCLK pins for debugging (GPIOs not available for tracing) */
#define SWO_ENABLE                      0           /* set to 1 to enable data tracing or serial printing via SWO pin */

/* network parameters */
#define HOST_ID                         1           /* note: host ID is only used to determine whether a node is a host node (irrelevant for source nodes); config will be overwritten by binary patching! */
#if !FLOCKLAB
  #define NODE_ID                       HOST_ID
#endif /* FLOCKLAB */
#define IS_HOST                         (NODE_ID == host_id)

/* energy (low-power mode) */
#if SWO_ENABLE
  #define LOW_POWER_MODE                LP_MODE_SLEEP  /* low-power mode to use between rounds during periods of inactivity */
#else /* SWO_ENABLE */
  #define LOW_POWER_MODE                LP_MODE_STOP2  /* low-power mode to use between rounds during periods of inactivity */
#endif /* SWO_ENABLE */
#define LPM_DISABLE_GPIO_CLOCKS         0

/* data collection / generation */
#define DATA_GENERATION_PERIOD          15          /* in seconds */
#define COLLECT_FLOODING_DATA           0

/* memory */
#define PRE_TASK_STACK_SIZE             256                             /* in # words of 4 bytes */
#define COM_TASK_STACK_SIZE             320                             /* in # words of 4 bytes */
#define POST_TASK_STACK_SIZE            256                             /* in # words of 4 bytes */
#define STACK_WARNING_THRESHOLD         80                              /* a warning will be generated once the stack usage of a task exceeds this value (in percent) */
#define TRANSMIT_QUEUE_SIZE             20                              /* #messages */
#define RECEIVE_QUEUE_SIZE              LWB_MAX_DATA_SLOTS              /* #messages */

/* Gloria config */
#define GLORIA_INTERFACE_POWER          1    /* transmit power in dBm (max. value is 14 for most RF bands); keep non-zero init for binary patching!; config will be overwritten by binary patching! */
#if FLOCKLAB
  #define GLORIA_INTERFACE_MODULATION   10   /* 7 = LoRa SF5, 10 = FSK 250kbit/s (see radio_constants.c for details); config will be overwritten by binary patching! */
  #define GLORIA_INTERFACE_RF_BAND      46   /* 869.01 MHz (see table in radio_constants.c for options); config will be overwritten by binary patching! */
#else
  #define GLORIA_INTERFACE_MODULATION   10   /* 7 = LoRa SF5, 10 = FSK 250kbit/s (see radio_constants.c for details); config will be overwritten by binary patching! */
  #define GLORIA_INTERFACE_RF_BAND      48   /* 869.46 MHz (see table in radio_constants.c for options); config will be overwritten by binary patching! */
#endif /* FLOCKLAB */

/* LWB config */
#define LWB_ENABLE                      1
#define LWB_NETWORK_ID                  0x4444
#define LWB_MIN_NODE_ID                 1
#define LWB_MAX_NODE_ID                 32
#define LWB_N_TX                        2
#define LWB_NUM_HOPS                    6
#define LWB_T_GAP                       LWB_MS_TO_TICKS(10)
#define LWB_SCHED_PERIOD                15
#define LWB_CONT_USE_HSTIMER            1
#define LWB_MAX_PAYLOAD_LEN             80
#define LWB_MAX_DATA_SLOTS              LWB_MAX_NUM_NODES
//#define LWB_DATA_ACK                    1
#define LWB_ON_WAKEUP()                 lpm_update_opmode(OP_MODE_EVT_WAKEUP)
#define LWB_T_PREPROCESS                LWB_MS_TO_TICKS(20)
#define LWB_SCHED_NODE_LIST             1, 2, 3//5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 19, 20, 21, 22, 23, 24, 26, 27, 28, 29, 31, 32  /* nodes to pre-register in the scheduler */

/* misc */
#define HS_TIMER_COMPENSATE_DRIFT       0
#define HS_TIMER_INIT_FROM_RTC          0
#define LPTIMER_RESET_WDG_ON_OVF        0
#define LPTIMER_RESET_WDG_ON_EXP        0
#define LPTIMER_CHECK_EXP_TIME          1
#define CLI_ENABLE                      0           /* command line interface */

/* logging */
#define LOG_ENABLE                      1
#define LOG_LEVEL                       LOG_LEVEL_INFO
#define LOG_USE_DMA                     0
#define LOG_BUFFER_SIZE                 4096
#if LOG_USE_DMA
  #define UART_FIFO_BUFFER_SIZE         LOG_BUFFER_SIZE
#endif /* LOG_USE_DMA */
#if BASEBOARD
  #define LOG_ADD_TIMESTAMP             0       /* don't print the timestamp on the baseboard */
  #define LOG_USE_COLORS                0
  #define LOG_LEVEL_ERROR_STR           "<3>"   /* use syslog severity level number instead of strings */
  #define LOG_LEVEL_WARNING_STR         "<4>"
  #define LOG_LEVEL_INFO_STR            "<6>"
  #define LOG_LEVEL_VERBOSE_STR         "<7>"
#endif /* BASEBOARD */
#if FLOCKLAB
  #define LOG_PRINT_IMMEDIATELY         1       /* enable immediate printing to get accurate timestamps on FlockLab */
#endif /* FLOCKLAB */

/* debugging */
#if FLOCKLAB
  #define ISR_ON_IND()                  bool nested = PIN_STATE(FLOCKLAB_INT1); (void)nested; PIN_SET(FLOCKLAB_INT1)
  #define ISR_OFF_IND()                 if (!nested) PIN_CLR(FLOCKLAB_INT1)
  #define CPU_ON_IND()                  //PIN_SET(FLOCKLAB_INT2)
  #define CPU_OFF_IND()                 //PIN_CLR(FLOCKLAB_INT2)
  #define LWB_RESUMED()                 PIN_SET(FLOCKLAB_INT2)
  #define LWB_SUSPENDED()               PIN_CLR(FLOCKLAB_INT2)
  #define POST_TASK_RESUMED()           PIN_SET(FLOCKLAB_INT2)
  #define POST_TASK_SUSPENDED()         PIN_CLR(FLOCKLAB_INT2)
  #define PRE_TASK_RESUMED()            PIN_SET(FLOCKLAB_INT2)
  #define PRE_TASK_SUSPENDED()          PIN_CLR(FLOCKLAB_INT2)
  #define GLORIA_START_IND()            led_on(LED_SYSTEM); PIN_SET(FLOCKLAB_INT2)
  #define GLORIA_STOP_IND()             led_off(LED_SYSTEM); PIN_CLR(FLOCKLAB_INT2)
  #define RADIO_TX_START_IND()          PIN_SET(FLOCKLAB_LED2)
  #define RADIO_TX_STOP_IND()           PIN_CLR(FLOCKLAB_LED2)
  #define RADIO_RX_START_IND()          PIN_SET(FLOCKLAB_LED3)
  #define RADIO_RX_STOP_IND()           PIN_CLR(FLOCKLAB_LED3)
#else /* FLOCKLAB */
  #define GLORIA_START_IND()            led_on(LED_SYSTEM); PIN_SET(COM_GPIO1)
  #define GLORIA_STOP_IND()             led_off(LED_SYSTEM); PIN_CLR(COM_GPIO1)
  #define RADIO_TX_START_IND()          PIN_SET(COM_GPIO2)
  #define RADIO_TX_STOP_IND()           PIN_CLR(COM_GPIO2)
  #define RADIO_RX_START_IND()          PIN_SET(COM_PROG2)
  #define RADIO_RX_STOP_IND()           PIN_CLR(COM_PROG2)
#endif /* FLOCKLAB */

#endif /* __APP_CONFIG_H */
