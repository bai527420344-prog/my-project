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
 * packet structure and message type definitions
 * MASTER file
 */

#ifndef __DPP_MESSAGE_H__
#define __DPP_MESSAGE_H__


/*
 * D E F I N E S
 */

/* max. message length (header + payload + CRC), limited by the COM module */
#define DPP_MSG_PKT_LEN           126     /* uint8_t (max. 255) */
#define DPP_MSG_HDR_LEN           16
#define DPP_MSG_CRC_LEN           2       /* CRC-16-IBM / ANSI */
/* max. message payload length */
#define DPP_MSG_PAYLOAD_LEN       (DPP_MSG_PKT_LEN - DPP_MSG_HDR_LEN - DPP_MSG_CRC_LEN)
/* actual message length incl. CRC */
#define DPP_MSG_LEN(msg)          \
    ((msg)->header.payload_len + DPP_MSG_HDR_LEN + DPP_MSG_CRC_LEN)

/* minimal message type with smaller header (set last bit of message type to
 * indicate a minimal message) */
#define DPP_MSG_TYPE_MIN          0x80    /* mask (msb) */
#define DPP_MSG_MIN_PKT_LEN       126     /* uint8_t (max. 255) */
#define DPP_MSG_MIN_HDR_LEN       4
#define DPP_MSG_MIN_PAYLOAD_LEN   \
    (DPP_MSG_MIN_PKT_LEN - DPP_MSG_MIN_HDR_LEN - DPP_MSG_CRC_LEN)
#define DPP_MSG_MIN_LEN(msg)      \
    ((msg)->header.payload_len + DPP_MSG_MIN_HDR_LEN + DPP_MSG_CRC_LEN)


/* special (reserved) device IDs */
#define DPP_DEVICE_ID_SINK        0
#define DPP_DEVICE_ID_BROADCAST   0xffff

/* special (reserved) values for data fields */
#define DPP_MSG_INV_UINT8         0xff            /* used to mark a uint8_t data value as invalid */
#define DPP_MSG_INV_INT8          0x80
#define DPP_MSG_INV_UINT16        0xffff
#define DPP_MSG_INV_INT16         0x8000
#define DPP_MSG_INV_UINT32        0xffffffff
#define DPP_MSG_INF_INT32         0x80000000


/* the message type (7 bits available, MSB is reserved) */
typedef enum {
  /* general message types */
  DPP_MSG_TYPE_INVALID      = 0,    /* unknown / invalid message type */
  DPP_MSG_TYPE_TIMESYNC     = 1,    /* timestamp */
  DPP_MSG_TYPE_EVENT        = 2,    /* event notification */
  DPP_MSG_TYPE_NODE_INFO    = 3,    /* node info (sent after a reset) */
  DPP_MSG_TYPE_CMD          = 4,    /* command message */
  DPP_MSG_TYPE_FW           = 5,    /* firmware update message */
  DPP_MSG_TYPE_HEALTH_MIN   = 6,    /* minimal health message (APP + COM combined) */

  /* message types concerning the communication processor */
  DPP_MSG_TYPE_COM_RESPONSE = 10,   /* command response */
  DPP_MSG_TYPE_COM_HEALTH   = 11,   /* com processor periodic health message */
  DPP_MSG_TYPE_LWB_HEALTH   = 12,   /* periodic LWB health message */

  /* message types concerning the application processor */
  DPP_MSG_TYPE_APP_HEALTH   = 21,   /* app processor periodic health message */

  /* application-specific message types (must start from 30U) */
  DPP_MSG_TYPE_AE_EVENT     = 32,   /* acoustic emission event */
  DPP_MSG_TYPE_AE_DATA      = 33,   /* acoustic emission data */
  DPP_MSG_TYPE_GNSS_SV      = 34,   /* GNSS SV message */
  DPP_MSG_TYPE_WGPS_STATUS  = 35,   /* Wireless GPS  */
  DPP_MSG_TYPE_GEOPHONE_ACQ = 36,   /* Geophone acquire data */
  DPP_MSG_TYPE_IMU          = 37,   /* imu data */
  DPP_MSG_TYPE_ADCDATA      = 38,   /* event acquisition (ADC) data */
  DPP_MSG_TYPE_INCLINO      = 39,   /* inclinometer data */
  DPP_MSG_TYPE_GEO_SPEC     = 40,   /* geophone spectrogram (freq data) */

  /* event-driven specific message types */
  DPP_MSG_TYPE_STAG_WAKEUP  = 41,   /* Staggered Wakeup message */
  DPP_MSG_TYPE_ACK_COMMAND  = 42,   /* Acknowledgment from the Base Station combined with a command to the sensor nodes */
  DPP_MSG_TYPE_HEALTH_AGGR  = 43,   /* Aggregated health message */
  DPP_MSG_TYPE_GEO_ACQ_AGGR = 44,   /* Aggregated geophone acquisition event */

  /* no types below this */
  DPP_MSG_TYPE_LASTID       = 127,

  /* --- same message types as above, but with min. header type --- */

  DPP_MSG_TYPE_INVALID_MIN  = 128,
  DPP_MSG_TYPE_TIMESYNC_MIN,
  DPP_MSG_TYPE_EVENT_MIN,
  DPP_MSG_TYPE_NODE_INFO_MIN,
  DPP_MSG_TYPE_CMD_MIN,
  DPP_MSG_TYPE_FW_MIN,
  DPP_MSG_TYPE_HEALTH_MIN_MIN,
} dpp_message_type_t;


/* timestamp in microseconds */
typedef uint64_t dpp_timestamp_t;


/*
 * I N C L U D E S
 */

#include "dpp_component_id.h"
#include "dpp_type.h"
#include "dpp_com_message.h"
#include "dpp_app_message.h"

/*
 * S T R U C T S
 */

/* for all following structs, force alignment to 1 byte */
#pragma pack(1)


#define DPP_NODE_INFO_LEN   46          /* bytes */
typedef struct {
  uint8_t             component_id;     /* application specific component id */
  uint8_t             rst_flag;         /* reset cause / source */
  uint16_t            rst_cnt;          /* reset counter */
  uint8_t             mcu_desc[12];     /* MCU description, i.g. 'CC430F5147' */
  uint8_t             compiler_desc[4]; /* compiler abbreviation (e.g. 'GCC') */
  uint32_t            compiler_ver;     /* compiler version: human-readable (e.g. AAABBBCCC where A: major, B: minor, C: patch */
  uint32_t            compile_date;     /* compilation time (seconds since 1970) */
  uint8_t             fw_name[8];       /* name of the firmware/application */
  uint16_t            fw_ver;           /* firmware version: human-readable (e.g. ABBCC where A: major, B: minor, C: patch */
  uint32_t            sw_rev_id;        /* repository revision number (GIT or SVN) */
  uint32_t            config;           /* bitfield for custom (application specific) compile-time configuration settings */
} dpp_node_info_t;


#define DPP_EVENT_LEN       6
typedef struct {
  dpp_event_type_t    type;       /* event type / id */
  uint32_t            value;      /* event value / subtype */
} dpp_event_t;


#define DPP_COMMAND_HDR_LEN 2
typedef struct {
  dpp_command_type_t  type;         /* command ID */
  union {
    uint8_t           arg[DPP_MSG_PAYLOAD_LEN - DPP_COMMAND_HDR_LEN];   /* arguments (length depends on command) */
    uint16_t          arg16[(DPP_MSG_PAYLOAD_LEN - DPP_COMMAND_HDR_LEN) / 2];
    uint32_t          arg32[(DPP_MSG_PAYLOAD_LEN - DPP_COMMAND_HDR_LEN) / 4];
  };
} dpp_command_t;


/* helper struct for dpp_ack_cmd_t */
#define DPP_CMD_MIN_LEN   6
typedef struct {
  uint16_t                target_id;    /* target node ID */
  union {
    struct {
      dpp_command_type_t  type;         /* command */
      uint16_t            arg;          /* argument */
    } cmd;
    uint32_t              cmd32;
  };
} dpp_cmd_min_t;

/* combined acknowledgement + commands */
#define DPP_ACK_CMD_LEN(ack_cmd)  ((ack_cmd)->num_cmds * DPP_CMD_MIN_LEN + 4)
#define DPP_ACK_MAX_CMDS          ((DPP_MSG_PAYLOAD_LEN - 4) / DPP_CMD_MIN_LEN)
typedef struct {
  uint16_t            ack_seq_no;     /* sequence no. of the acknowledged packet */
  uint16_t            num_cmds;       /* number of node ID + command tuples (note: use 16 bits for better alignment) */
  dpp_cmd_min_t       commands[DPP_ACK_MAX_CMDS];    /* list of commands */
} dpp_ack_cmd_t;


/* combined (APP + COM) minimal health message */
#define DPP_HEALTH_MIN_LEN  22
typedef struct {
  uint16_t            uptime;         /* system uptime (either COM or APP) [h] */
  uint16_t            cpu_dc_app;     /* CPU duty cycle (APP) [10^-2 %] */
  uint16_t            cpu_dc_com;     /* CPU duty cycle (COM) [10^-2 %] */
  uint8_t             supply_vcc;     /* system supply voltage [10^-1 V] */
  uint8_t             nv_mem;         /* Non-volatile memory usage (typically SD card of APP) [%] */
  uint8_t             stack_wm;       /* Stack watermark (max value of APP and COM) [%] (note: MSB = 1 for AP, MSB = 0 for CP) */
  int8_t              temperature;    /* Ambient temperature [°C] */
  uint8_t             humidity;       /* Ambient humidity [%] */
  uint8_t             radio_prr;      /* Average packet reception rate [%] */
  int8_t              radio_rssi;     /* Average receive signal strength indicator [dBm] */
  uint8_t             radio_hop_cnt;  /* Average hop count on first reception [10^-1 hops] */
  uint16_t            radio_rx_dc;    /* RX duty cycle (COM) [10^-2 %] */
  uint16_t            radio_tx_dc;    /* TX duty cycle (COM) [10^-2 %] */
  uint16_t            events;         /* bit field to indicate events that occurred */
  uint16_t            config;         /* contains a part of the current node configuration (bit field) */
} dpp_health_min_t;


/* helper structs for dpp_data_aggr_t to create tuples of node ID and some other struct */
#define DPP_DATA_AGGR_HEALTH_MIN_LEN    (2 + DPP_HEALTH_MIN_LEN)
typedef struct {
  uint16_t            node_id;
  dpp_health_min_t    health_min;
} dpp_data_aggr_health_min_t;

#define DPP_DATA_AGGR_GEO_ACQ_MIN_LEN   (2 + DPP_GEOPHONE_ACQ_MIN_LEN)
typedef struct {
  uint16_t                node_id;
  dpp_geophone_acq_min_t  geo_acq_min;
} dpp_data_aggr_geo_acq_min_t;

/* for general data aggregation (several 'blocks' of the same type of data sequentialized) */
#define DPP_DATA_AGGR_HDR_LEN     2
#define DPP_DATA_AGGR_LEN(aggr)   (((aggr)->block_cnt * (aggr)->block_size) + 2)
typedef struct {
  uint8_t                       block_cnt;
  uint8_t                       block_size;
  union {
    dpp_data_aggr_health_min_t  health_blocks[(DPP_MSG_PAYLOAD_LEN - DPP_DATA_AGGR_HDR_LEN) / DPP_DATA_AGGR_HEALTH_MIN_LEN];
    dpp_data_aggr_geo_acq_min_t geo_acq_blocks[(DPP_MSG_PAYLOAD_LEN - DPP_DATA_AGGR_HDR_LEN) / DPP_DATA_AGGR_GEO_ACQ_MIN_LEN];
  };
} dpp_data_aggr_t;


#define DPP_FW_HDR_LEN        4
#define DPP_FW_BLOCK_SIZE     96    /* must be a multiple of 8 */
#define DPP_FW_INFO_LEN       (DPP_FW_HDR_LEN + 8)
#define DPP_FW_DATA_LEN       (DPP_FW_HDR_LEN + 2 + DPP_FW_BLOCK_SIZE)
#define DPP_FW_REQ_LEN(n)     (DPP_FW_HDR_LEN + 2 + 2 * (n))
#define DPP_FW_REQ_MAX_BLOCKS ((DPP_MSG_PAYLOAD_LEN - 2 - DPP_FW_HDR_LEN) / 2)

typedef struct {
  dpp_fw_type_t type : 8;
  uint8_t       component_id;
  uint16_t      version;
  union {
    struct {
      uint32_t  len;    /* total length in bytes */
      uint32_t  crc;    /* 32bit CRC over complete FW data ('len' bytes) */
    } info;
    struct {
      uint16_t  ofs;    /* offset = block ID (starting from 0) */
      uint8_t   data[DPP_FW_BLOCK_SIZE];
    } data;
    struct {
      uint16_t  num;    /* 16-bit to avoid alignment issues */
      uint16_t  block_ids[DPP_FW_REQ_MAX_BLOCKS];
    } req;
  };
} dpp_fw_t;


/* header of a DPP message */
typedef struct {
  uint16_t            device_id;        /* sender node ID */
  dpp_message_type_t  type : 8;         /* force 1 byte */
  uint8_t             payload_len;      /* payload length [bytes] */
  uint16_t            target_id;        /* recipient device ID */
  uint16_t            seqnr;            /* sequence number */
  uint64_t            generation_time;  /* packet generation time (us) */
} dpp_header_t;


/* application layer packet format (a packet is called 'message') */
/* crc is calculated over the whole structure and appended to the message */
typedef struct {
  /* header: DPP_MSG_HDR_LEN bytes */
  dpp_header_t          header;
  /* none of the union members may be larger than DPP_MSG_PAYLOAD_LEN bytes
   * except for 'payload' */
  union {
    dpp_com_health_t    com_health;
    dpp_app_health_t    app_health;
    dpp_health_min_t    health_min;
    dpp_command_t       cmd;
    dpp_node_info_t     node_info;
    dpp_event_t         evt;              /* do not rename to 'event', compiler usage in TinyOS */
    dpp_imu_t           imu;
    dpp_inclino_t       inclino;
    dpp_gnss_sv_t       gnss_sv;
    dpp_wgps_status_t   wgps_status;
    dpp_timestamp_t     timestamp;
    dpp_fw_t            firmware;
    dpp_lwb_health_t    lwb_health;
    dpp_geophone_acq_t  geo_acq;
    dpp_geophone_adc_t  geo_adc;
    dpp_geophone_spec_t geo_spec;
    dpp_data_aggr_t     data_aggr;
    dpp_ack_cmd_t       ack_cmd;
    uint8_t             payload[DPP_MSG_PAYLOAD_LEN + DPP_MSG_CRC_LEN];
    uint16_t            payload16[DPP_MSG_PAYLOAD_LEN / 2]; /* rounded down! */
  };
} dpp_message_t;


/* application layer packet format (minimal type) */
typedef struct {
  struct {
    uint16_t            device_id;        /* sender node ID */
    dpp_message_type_t  type : 8;         /* message type (MSB must be set!) */
    uint8_t             payload_len;      /* payload length [bytes] */
  } header;
  union {
    dpp_com_health_t    com_health;
    dpp_app_health_t    app_health;
    dpp_health_min_t    health_min;
    dpp_command_t       cmd;
    dpp_node_info_t     node_info;
    dpp_event_t         evt;              /* do not rename to 'event', compiler usage in TinyOS */
    dpp_imu_t           imu;
    dpp_inclino_t       inclino;
    dpp_gnss_sv_t       gnss_sv;
    dpp_wgps_status_t   wgps_status;
    dpp_timestamp_t     timestamp;
    dpp_fw_t            firmware;
    dpp_lwb_health_t    lwb_health;
    dpp_geophone_acq_t  geo_acq;
    dpp_geophone_adc_t  geo_adc;
    dpp_geophone_spec_t geo_spec;
    dpp_data_aggr_t     data_aggr;
    dpp_ack_cmd_t       ack_cmd;
    uint8_t             payload[DPP_MSG_MIN_PAYLOAD_LEN + DPP_MSG_CRC_LEN];
    uint16_t            payload16[DPP_MSG_MIN_PAYLOAD_LEN / 2];
  };
} dpp_message_min_t;


#pragma pack()    /* restore default structure packing rule */


/*
 * M A C R O S
 */

#define DPP_MSG_SET_CRC16(msg, crc) \
  (msg)->payload[(msg)->header.payload_len] = crc & 0xff; \
  (msg)->payload[(msg)->header.payload_len + 1] = (crc >> 8) & 0xff;
#define DPP_MSG_GET_CRC16(msg)      \
  ((uint16_t)(msg)->payload[(msg)->header.payload_len] | \
   (uint16_t)(msg)->payload[(msg)->header.payload_len + 1] << 8)


/*
 * E R R O R   C H E C K S
 */

#ifdef BOLT_CONF_MAX_MSG_LEN
  #if BOLT_CONF_MAX_MSG_LEN < MSG_PKT_LEN
    #error "BOLT max msg length is too small"
  #endif
#endif /* BOLT_CONF_MAX_MSG_LEN */

#if DPP_FW_BLOCK_SIZE > (DPP_MSG_PAYLOAD_LEN - 2 - DPP_FW_HDR_LEN)
  #error "DPP_FW_BLOCK_SIZE is too big"
#endif /* DPP_FW_BLOCK_SIZE */

_Static_assert(sizeof(dpp_header_t) == DPP_MSG_HDR_LEN, "dpp_header_t is not DPP_MSG_HDR_LEN bytes in size!");
_Static_assert(sizeof(dpp_message_t) == DPP_MSG_PKT_LEN, "dpp_message_t is not DPP_MSG_PKT_LEN bytes in size!");
_Static_assert(sizeof(dpp_message_min_t) == DPP_MSG_PKT_LEN, "dpp_message_min_t is not DPP_MSG_PKT_LEN bytes in size!");


#endif /* __DPP_MESSAGE_H__ */
