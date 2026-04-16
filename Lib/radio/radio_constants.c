/*
 * Copyright (c) 2018 - 2021, ETH Zurich, Computer Engineering Group (TEC)
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

#if RADIO_ENABLE

const radio_config_t radio_modulations[RADIO_NUM_MODULATIONS] =
{
    // NOTE: bandwidth is the Bandwidths[] index (0=203kHz, 1=406kHz, 2=812kHz, 3=1625kHz)
    {   // 0: LoRa SF12
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 12,
        .coderate = 1,
        .preambleLen = 10,
    },
    {   // 1: LoRa SF11
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 11,
        .coderate = 1,
        .preambleLen = 10,
    },
    {   // 2: LoRa SF10
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 10,
        .coderate = 1,
        .preambleLen = 10,
    },
    {   // 3: LoRa SF9
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 9,
        .coderate = 1,
        .preambleLen = 10,
    },
    {   // 4: LoRa SF8
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 8,
        .coderate = 1,
        .preambleLen = 10,
    },
    {   // 5: LoRa SF7
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 7,
        .coderate = 1,
        .preambleLen = 10,
    },
    {   // 6: LoRa SF6
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 6,
        .coderate = 1,
        .preambleLen = 12,
    },
    {   // 7: LoRa SF5
        .modem = MODEM_LORA,
        .bandwidth = 0,
        .datarate = 5,
        .coderate = 1,
        .preambleLen = 12,
    },
    {   // 8: GFSK 125kbit/s
        .modem = MODEM_FSK,
        .bandwidth = 234300,
        .datarate = 125000,
        .fdev = 50000,
        .preambleLen = 2,
    },
    {   // 9: GFSK 200kbit/s
        .modem = MODEM_FSK,
        .bandwidth = 234300,
        .datarate = 200000,
        .fdev = 10000,
        .preambleLen = 2,
    },
    {   // 10: GFSK 250kbit/s
        .modem = MODEM_FSK,
        .bandwidth = 312000,
        .datarate = 250000,
        .fdev = 23500,
        .preambleLen = 4,
    },
};

const radio_band_t radio_bands[RADIO_NUM_BANDS] =
{
    /* 2.4 GHz ISM band, 2 MHz spacing, no duty cycle restriction */
    { .centerFrequency = 2402000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 0
    { .centerFrequency = 2404000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 1
    { .centerFrequency = 2406000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 2
    { .centerFrequency = 2408000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 3
    { .centerFrequency = 2410000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 4
    { .centerFrequency = 2412000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 5
    { .centerFrequency = 2414000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 6
    { .centerFrequency = 2416000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 7
    { .centerFrequency = 2418000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 8
    { .centerFrequency = 2420000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 9
    { .centerFrequency = 2422000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 10
    { .centerFrequency = 2424000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 11
    { .centerFrequency = 2426000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 12
    { .centerFrequency = 2428000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 13
    { .centerFrequency = 2430000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 14
    { .centerFrequency = 2432000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 15
    { .centerFrequency = 2434000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 16
    { .centerFrequency = 2436000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 17
    { .centerFrequency = 2438000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 18
    { .centerFrequency = 2440000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 19
    { .centerFrequency = 2442000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 20
    { .centerFrequency = 2444000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 21
    { .centerFrequency = 2446000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 22
    { .centerFrequency = 2448000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 23
    { .centerFrequency = 2450000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 24
    { .centerFrequency = 2452000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 25
    { .centerFrequency = 2454000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 26
    { .centerFrequency = 2456000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 27
    { .centerFrequency = 2458000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 28
    { .centerFrequency = 2460000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 29
    { .centerFrequency = 2462000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 30
    { .centerFrequency = 2464000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 31
    { .centerFrequency = 2466000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 32
    { .centerFrequency = 2468000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 33
    { .centerFrequency = 2470000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 34
    { .centerFrequency = 2472000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 35
    { .centerFrequency = 2474000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 36
    { .centerFrequency = 2476000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 37
    { .centerFrequency = 2478000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 38
    { .centerFrequency = 2480000000UL, .bandwidth = 812500, .dutyCycle = 1000, .maxPower = 12 }, // 39
};

const radio_band_group_t lora_band_groups[] = {
    { .lower = 0, .upper = 39 },
};

// CAD with 1 Symbol is totally random!
const radio_cad_params_t radio_cad_params[RADIO_NUM_CAD_PARAMS] = {
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 25, .cad_det_min = 10}, // SF12
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 24, .cad_det_min = 10}, // SF11
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 23, .cad_det_min = 10}, // SF10
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 22, .cad_det_min = 10}, // SF9
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 21, .cad_det_min = 10}, // SF8
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 20, .cad_det_min = 10}, // SF7
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 19, .cad_det_min = 10}, // SF6
    {.symb_num = LORA_CAD_04_SYMBOL, .cad_det_peak = 18, .cad_det_min = 10}, // SF5
};

#endif /* RADIO_ENABLE */
