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


#define FIFO_GET_ELEM_BY_INDEX(idx)   (void*)((uint32_t)((fifo_meta_t*)fifo)->buffer_addr + (idx * ((fifo_meta_t*)fifo)->elem_size))

/*---------------------------------------------------------------------------*/
void fifo_clear(fifo_t fifo)
{
  if (fifo) {
    ((fifo_meta_t*)fifo)->write_idx = 0;
    ((fifo_meta_t*)fifo)->read_idx = 0;
  }
}
/*---------------------------------------------------------------------------*/
bool fifo_is_empty(const fifo_t fifo)
{
  if (fifo) {
    return (((fifo_meta_t*)fifo)->read_idx == ((fifo_meta_t*)fifo)->write_idx);
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool fifo_is_full(const fifo_t fifo)
{
  if (fifo) {
    fifo_meta_t* f = (fifo_meta_t*)fifo;
    uint32_t w = ((f->write_idx + 1) == f->num_elem) ? 0 : (f->write_idx + 1);
    return (w == f->read_idx);
  }
  return true;
}
/*---------------------------------------------------------------------------*/
/* get current fill level */
uint32_t fifo_get_used_space(const fifo_t fifo)
{
  if (fifo) {
    fifo_meta_t* f = (fifo_meta_t*)fifo;
    if (f->write_idx < f->read_idx) {
      return (f->num_elem - f->read_idx + f->write_idx);
    }
    return (f->write_idx - f->read_idx);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
uint32_t fifo_get_free_space(const fifo_t fifo)
{
  return (((fifo_meta_t*)fifo)->num_elem - fifo_get_used_space(fifo));
}
/*---------------------------------------------------------------------------*/
bool fifo_push(fifo_t fifo, const void* elem)
{
  if (!fifo || !elem || fifo_is_full(fifo)) {
    return false;
  }
  fifo_meta_t* f = (fifo_meta_t*)fifo;
  memcpy(FIFO_GET_ELEM_BY_INDEX(f->write_idx), elem, f->elem_size);
  f->write_idx++;
  if (f->write_idx == f->num_elem) {
    f->write_idx = 0;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool fifo_pop(fifo_t fifo, void* elem)
{
  if (!fifo || !elem || !fifo_is_empty(fifo)) {
    return false;
  }
  fifo_meta_t* f = (fifo_meta_t*)fifo;
  memcpy(elem, FIFO_GET_ELEM_BY_INDEX(f->read_idx), f->elem_size);
  ((fifo_meta_t*)fifo)->read_idx++;
  if (f->read_idx == f->num_elem) {
    f->read_idx = 0;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
const void* fifo_peek(const fifo_t fifo)
{
  if (fifo && !fifo_is_empty(fifo)) {
    return FIFO_GET_ELEM_BY_INDEX(((fifo_meta_t*)fifo)->read_idx);
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
