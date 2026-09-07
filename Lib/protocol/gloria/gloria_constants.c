/*
 * Copyright (c) 2018 - 2022, ETH Zurich, Computer Engineering Group (TEC)
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

const uint8_t gloria_modulations[]             = { 3,  5,  7, 9 };
const int8_t  gloria_powers[]                  = { 0, 10, 12 };                                 // dBm
const uint8_t gloria_default_power_levels[]    = { 0,  0,  0,  0,  0,  0,  0,  0,  2,  2,  2,  2,  2 };  // see radio_powers; FLRC entries 10-12 use same as GFSK
const uint8_t gloria_default_retransmissions[] = { 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3 };
const uint8_t gloria_default_acks[]            = { 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3 };
const uint8_t gloria_default_data_slots[]      = { 4,  4,  4,  4,  8,  8, 12, 12, 16, 16, 16, 16, 16 };

/*
 * SX1280 theoretical timing values (20260413)
 *
 * Calculated from SX1262 empirical measurements (20200812) using decomposition:
 *   each field = fixed_mcu_overhead + N * symbol_time
 * where symbol_time is recalculated for SX1280 @ BW=203.125 kHz.
 *
 * Decomposition models (verified against SX1262 data, <4% error):
 *   rxOffset:          SF12-SF7: 3.0 * Ts,          SF6-SF5: 5.615 * Ts
 *   floodInitOverhead: SF12-SF7: 9506 + 3.0 * Ts,   SF6-SF5: 9507 + 5.615 * Ts
 *   slotOverhead:      17657 + 15.876 * Ts
 *   slotAckOverhead:   23055 + 18.856 * Ts
 *   txSync:            SF12-SF7: ratio 8/13,         SF6-SF5: 2276 + 26.875 * Ts
 *
 * where Ts = SX1280 symbol time at BW=203.125 kHz (from radio_lora_symb_times[0][]).
 * GFSK entries (8-9) kept at SX1262 values (same bit rates, similar transition times).
 *
 * These are THEORETICAL starting values. Hardware calibration required in R6.
 */
const gloria_timings_t gloria_timings[] = {
    { // 0 (SF12)  Ts=161320
        .slotOverhead       = 2578773,  // 322.347 ms
        .slotAckOverhead    = 3064905,  // 383.113 ms
        .floodInitOverhead  = 493466,   // 61.683 ms
        .rxOffset           = 483960,   // 60.495 ms
        .txSync             = 3740830,  // 467.604 ms (theoretical 20260413)
    },
    { // 1 (SF11)  Ts=80656
        .slotOverhead       = 1298152,  // 162.269 ms
        .slotAckOverhead    = 1543905,  // 192.988 ms
        .floodInitOverhead  = 251474,   // 31.434 ms
        .rxOffset           = 241968,   // 30.246 ms
        .txSync             = 1866154,  // 233.269 ms (theoretical 20260413)
    },
    { // 2 (SF10)  Ts=40328
        .slotOverhead       = 657904,   // 82.238 ms
        .slotAckOverhead    = 783480,   // 97.935 ms
        .floodInitOverhead  = 130490,   // 16.311 ms
        .rxOffset           = 120984,   // 15.123 ms
        .txSync             = 931330,   // 116.416 ms (theoretical 20260413)
    },
    { // 3 (SF9)  Ts=20168
        .slotOverhead       = 337844,   // 42.231 ms
        .slotAckOverhead    = 403343,   // 50.418 ms
        .floodInitOverhead  = 70010,    // 8.751 ms
        .rxOffset           = 60504,    // 7.563 ms
        .txSync             = 465186,   // 58.148 ms (theoretical 20260413)
    },
    { // 4 (SF8)  Ts=10080
        .slotOverhead       = 177687,   // 22.211 ms
        .slotAckOverhead    = 213123,   // 26.640 ms
        .floodInitOverhead  = 39746,    // 4.968 ms
        .rxOffset           = 30240,    // 3.780 ms
        .txSync             = 232748,   // 29.094 ms (theoretical 20260413)
    },
    { // 5 (SF7)  Ts=5040
        .slotOverhead       = 97672,    // 12.209 ms
        .slotAckOverhead    = 118089,   // 14.761 ms
        .floodInitOverhead  = 24626,    // 3.078 ms
        .rxOffset           = 15120,    // 1.890 ms
        .txSync             = 116839,   // 14.605 ms (theoretical 20260413)
    },
    { // 6 (SF6)  Ts=2520
        .slotOverhead       = 57665,    // 7.208 ms
        .slotAckOverhead    = 70572,    // 8.822 ms
        .floodInitOverhead  = 23657,    // 2.957 ms
        .rxOffset           = 14151,    // 1.769 ms
        .txSync             = 70001,    // 8.750 ms (theoretical 20260413)
    },
    { // 7 (SF5)  Ts=1264
        .slotOverhead       = 37724,    // 4.716 ms
        .slotAckOverhead    = 46889,    // 5.861 ms
        .floodInitOverhead  = 16604,    // 2.076 ms
        .rxOffset           = 7097,     // 0.887 ms
        .txSync             = 36246,    // 4.531 ms (theoretical 20260413)
    },
    { // 8 (FSK 125k)
        .slotOverhead       = 28000,    // 3.5 ms
        .slotAckOverhead    = 28000,    // 3.5 ms
        .floodInitOverhead  = 18000,    // 2.25 ms
        .rxOffset           = 4096,     // 512.000 us
        .txSync             = 4137,     // 517.125 us (SX1262 value, same bitrate)
    },
    { // 9 (FSK 250k)
        .slotOverhead       = 14000,    // 1.75 ms
        .slotAckOverhead    = 14000,    // 1.75 ms
        .floodInitOverhead  = 18000,    // 2.25 ms
        .rxOffset           = 2560,     // 320.000 us
        .txSync             = 3140,     // 392.5 us (SX1262 value, same bitrate)
    },
    /* FLRC entries 10-12: conservative starting values per MIGRATION_PLAN
     * R6 rule "宁可浪费时间，不可打断 flood". FLRC physical bitrate is high
     * (260k..1.3M) but the chip-side state-machine overhead is similar to GFSK,
     * so we reuse GFSK overhead numbers and scale by bitrate ratio.
     * Hardware calibration deferred to F8 / R6 follow-up. */
    { // 10 (FLRC 260k CR=1/2)  -> effective payload bitrate ~130 kbit/s
        .slotOverhead       = 32000,    // 4.0 ms  (a bit more than GFSK 125k due to FLRC header)
        .slotAckOverhead    = 32000,
        .floodInitOverhead  = 18000,    // 2.25 ms
        .rxOffset           = 4096,     // 512 us
        .txSync             = 4500,     // ~562 us (placeholder)
    },
    { // 11 (FLRC 650k CR=1/2)  -> effective ~325 kbit/s
        .slotOverhead       = 20000,    // 2.5 ms
        .slotAckOverhead    = 20000,
        .floodInitOverhead  = 18000,
        .rxOffset           = 2560,
        .txSync             = 3200,
    },
    { // 12 (FLRC 1300k CR=3/4) -> effective ~975 kbit/s
        .slotOverhead       = 12000,    // 1.5 ms
        .slotAckOverhead    = 12000,
        .floodInitOverhead  = 18000,
        .rxOffset           = 2000,
        .txSync             = 2500,
    },
};

_Static_assert(sizeof(gloria_timings) / sizeof(gloria_timings[0]) == RADIO_NUM_MODULATIONS,
               "gloria_timings must match radio_modulations");
_Static_assert(sizeof(gloria_default_power_levels) / sizeof(gloria_default_power_levels[0]) == RADIO_NUM_MODULATIONS,
               "gloria_default_power_levels must match radio_modulations");
_Static_assert(sizeof(gloria_default_retransmissions) / sizeof(gloria_default_retransmissions[0]) == RADIO_NUM_MODULATIONS,
               "gloria_default_retransmissions must match radio_modulations");
_Static_assert(sizeof(gloria_default_acks) / sizeof(gloria_default_acks[0]) == RADIO_NUM_MODULATIONS,
               "gloria_default_acks must match radio_modulations");
_Static_assert(sizeof(gloria_default_data_slots) / sizeof(gloria_default_data_slots[0]) == RADIO_NUM_MODULATIONS,
               "gloria_default_data_slots must match radio_modulations");
