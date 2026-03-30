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

#ifndef __DPP_LIB_H__
#define __DPP_LIB_H__

/* includes */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


/* --- config --- */



/* --- macros --- */

/* statically allocate memory blocks on the heap */
#define MEMB_CREATE(memb_name, block_size, num_blocks)      static uint8_t memb_name##_memb[((num_blocks + 31) / 32) + block_size * num_blocks] = { 0 }; \
                                                            static memb_meta_t memb_name##_memb_info = { block_size, num_blocks, (uint32_t*)memb_name##_memb, (void*)((uint32_t)memb_name##_memb + ((num_blocks + 31) / 32)) }; \
                                                            memb_t memb_name = &memb_name##_memb_info

#define LIST_CREATE(list_name, element_size, num_elements)  static uint8_t list_name##_memb[((num_elements + 31) / 32) + (sizeof(list_elem_t) + element_size) * num_elements] = { 0 }; \
                                                            static memb_meta_t list_name##_memb_info = { (sizeof(list_elem_t) + element_size), num_elements, (uint32_t*)list_name##_memb, (void*)((uint32_t)list_name##_memb + ((num_elements + 31) / 32)) }; \
                                                            static list_meta_t list_name##_list = { 0, &list_name##_memb_info }; \
                                                            list_t list_name = &list_name##_list

#define FIFO_CREATE(fifo_name, element_size, num_elements)  static uint8_t fifo_name##_fifo_buffer[element_size * num_elements] = { 0 }; \
                                                            static fifo_meta_t fifo_name##_fifo_info = { 0, 0, element_size, num_elements, fifo_name##_fifo_buffer }; \
                                                            fifo_t fifo_name = &fifo_name##_fifo_info

#define SWAP_VALUES(a, b, tmp)  tmp = a; a = b; b = tmp

#ifndef ABS
#define ABS(x)                  (((x) < 0) ? (-(x)) : (x))
#endif
#ifndef MIN
#define MIN(x, y)               (((x) < (y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y)               (((x) > (y)) ? (x) : (y))
#endif
#ifndef CLAMP
#define CLAMP(x, lower, upper)  if ((x) > upper) { x = upper; } else if ((x) < lower) { x = lower; }
#endif
#ifndef CLAMP_UINT16
#define CLAMP_UINT16(x)         if ((x) > UINT16_MAX) { x = UINT16_MAX; }
#endif
#ifndef CLAMP_UINT8
#define CLAMP_UINT8(x)          if ((x) > UINT8_MAX) { x = UINT8_MAX; }
#endif


/* --- typedefs --- */

typedef void* memb_t;     /* just an alias for memb_meta_t to hide the contents of the struct from the user */

typedef struct memb {
  uint32_t  block_size;
  uint32_t  num_blocks;
  uint32_t* valid_bits;
  void*     blocks;
} memb_meta_t;

typedef void* fifo_t;     /* just an alias for fifo_meta_t to hide the contents of the struct from the user */

typedef struct fifo {
  uint32_t write_idx;
  uint32_t read_idx;
  uint32_t elem_size;
  uint32_t num_elem;
  void*    buffer_addr;
} fifo_meta_t;

typedef void* list_t;     /* just an alias for list_meta_t to hide the contents of the struct from the user */

typedef struct list_elem {
  uint32_t key;
  void*    next;
} list_elem_t;

typedef struct list_meta {
  list_elem_t* head;
  memb_t       list_mem;
} list_meta_t;


/* --- function prototypes --- */

uint8_t  crc8(const uint8_t* data, uint32_t num_bytes, uint8_t init_val);
uint16_t crc16(const uint8_t* data, uint16_t num_bytes, uint16_t init_val);
uint32_t crc32(const uint8_t* data, uint32_t num_bytes, uint32_t init_val);

uint32_t hexstr_to_uint32(const char* str);
uint32_t uint16_to_str(uint16_t val, char* out_buffer);
uint32_t bytes_to_hexstr(const uint8_t* bytes, uint32_t num_bytes, char* out_buffer, uint32_t buffer_size);
int32_t  str_to_int32(const char* str);
void     base64_encode(const uint8_t* input, uint32_t len, char* out_buf, uint32_t buf_len);
uint32_t base64_decode_char(unsigned char c);
void     base64_decode(const char* input, uint32_t len, uint8_t* out_buf, uint32_t buf_len);

void     memb_init(memb_t memb);
void*    memb_allocate_block(memb_t memb);
bool     memb_release_block(memb_t memb, void* block_addr);
uint32_t memb_get_block_size(memb_t memb);
uint32_t memb_get_block_count(memb_t memb);
uint32_t memb_get_free_block_count(memb_t memb);

void     list_init(list_t list);
bool     list_insert(list_t list, uint32_t key, const void* data);
bool     list_remove(list_t list, const void* elem);
bool     list_remove_by_key(list_t list, uint32_t key);
bool     list_remove_head(list_t list, void* data);
const void* list_get_head(const list_t list);
uint32_t list_get_head_key(const list_t list);

void     fifo_clear(fifo_t fifo);
bool     fifo_push(fifo_t fifo, const void* elem);
bool     fifo_pop(fifo_t fifo, void* elem);
const void* fifo_peek(const fifo_t fifo);
bool     fifo_is_empty(const fifo_t fifo);
bool     fifo_is_full(const fifo_t fifo);
uint32_t fifo_get_used_space(const fifo_t fifo);
uint32_t fifo_get_free_space(const fifo_t fifo);

void     bubble_sort_uint8(uint8_t* a, uint16_t len);

uint32_t gcd(uint32_t a, uint32_t b);
uint32_t lcm(uint32_t a, uint32_t b);

#endif /* __DPP_LIB_H__ */
