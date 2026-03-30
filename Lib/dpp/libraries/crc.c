/*
 * Copyright (c) 2017 - 2022, ETH Zurich, Computer Engineering Group (TEC)
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


#include "dpp_lib.h"

/*---------------------------------------------------------------------------*/
/* CRC-8-Dallas/Maxim, based on: http://stackoverflow.com/questions/29214301/ios-how-to-calculate-crc-8-dallas-maxim-of-nsdata */
uint8_t crc8(const uint8_t* data, uint32_t num_bytes, uint8_t init_val)
{
  const uint16_t poly = 0x131;
  uint32_t       crc  = init_val;
  while (num_bytes) {
    crc         ^= *data;
    uint32_t bit = 8;
    while (bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ poly;
      } else {
        crc <<= 1;
      }
      bit--;
    }
    data++;
    num_bytes--;
  }
  return (uint8_t)crc;
}
/*---------------------------------------------------------------------------*/
/* CRC-16-IBM, CRC-16-ANSI */
uint16_t crc16(const uint8_t* data, uint16_t num_bytes, uint16_t init_val)
{
  const uint16_t poly = 0xa001;
  uint16_t       crc  = init_val;
  while (num_bytes) {
    uint16_t ch  = *data;
    uint16_t bit = 8;
    while (bit) {
      if ((crc & 1) ^ (ch & 1)) {
        crc = (crc >> 1) ^ poly;
      } else {
        crc >>= 1;
      }
      ch >>= 1;
      bit--;
    }
    data++;
    num_bytes--;
  }
  return crc;
}
/*---------------------------------------------------------------------------*/
uint32_t crc32(const uint8_t* data, uint32_t num_bytes, uint32_t init_val)
{
  const uint32_t poly = 0xedb88320;
  uint32_t       crc  = ~init_val;
  while (num_bytes) {
    crc         ^= (*data);
    uint32_t bit = 8;
    while (bit) {
      uint32_t mask = -(crc & 1);
      crc           = (crc >> 1) ^ (poly & mask);
      bit--;
    }
    data++;
    num_bytes--;
  }
  return ~crc;
}
/*---------------------------------------------------------------------------*/
