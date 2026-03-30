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

#include "dpp_lib.h"

/*---------------------------------------------------------------------------*/
void* memb_allocate_block(memb_t memb)
{
  if (!memb) {
    return NULL;
  }
  memb_meta_t* m = (memb_meta_t*)memb;
  if (!m->valid_bits) {
    return NULL;
  }
  uint32_t i;
  /* linear search */
  for (i = 0; i < m->num_blocks; i++) {
    uint32_t bitmask = (1 << (i & 31));
    if ((m->valid_bits[i >> 5] & bitmask) == 0) {
      m->valid_bits[i >> 5] |= bitmask;                         /* set bit (mark as used) */
      return (void*)((uint32_t)m->blocks + i * m->block_size);  /* return block address */
    }
  }
  return NULL;  /* no free block found */
}
/*---------------------------------------------------------------------------*/
bool memb_release_block(memb_t memb, void* block_addr)
{
  if (!memb) {
    return false;
  }
  memb_meta_t* m = (memb_meta_t*)memb;
  if (!m->block_size) {
    return false;
  }
  uint32_t block_idx = ((uint32_t)block_addr - (uint32_t)m->blocks) / m->block_size;
  if (block_idx < m->num_blocks) {
    uint32_t bitmask = (1 << (block_idx & 31));
    if (m->valid_bits[block_idx >> 5] & bitmask) {
      m->valid_bits[block_idx >> 5] &= ~bitmask;
      return true;
    }
  }
  return false;   /* block not found */
}
/*---------------------------------------------------------------------------*/
void memb_init(memb_t memb)
{
  if (memb) {
    memb_meta_t* m = (memb_meta_t*)memb;
    if (m->valid_bits) {
      memset(m->valid_bits, 0, (m->num_blocks + 31) / 32 + (m->num_blocks * m->block_size));
    }
  }
}
/*---------------------------------------------------------------------------*/
uint32_t memb_get_block_size(memb_t memb)
{
  if (memb) {
    return ((memb_meta_t*)memb)->block_size;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
uint32_t memb_get_block_count(memb_t memb)
{
  if (memb) {
    return ((memb_meta_t*)memb)->num_blocks;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
uint32_t memb_get_free_block_count(memb_t memb)
{
  if (!memb) {
    return 0;
  }
  memb_meta_t* m = (memb_meta_t*)memb;
  if (!m->valid_bits) {
    return 0;
  }
  uint32_t i, cnt = 0;
  for (i = 0; i < m->num_blocks; i++) {
    uint32_t bitmask = (1 << (i & 31));
    if ((m->valid_bits[i >> 5] & bitmask) == 0) {
      cnt++;
    }
  }
  return cnt;
}
/*---------------------------------------------------------------------------*/
