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

#define LIST_ELEM_DATA(elem)    (void*)((uint32_t)elem + sizeof(list_elem_t))

/*---------------------------------------------------------------------------*/
void list_init(list_t list)
{
  if (list) {
    list_meta_t* l = (list_meta_t*)list;
    memb_init(l->list_mem);
    l->head = NULL;
  }
}
/*---------------------------------------------------------------------------*/
void list_link(list_t list, list_elem_t* elem)
{
  if (!list || !elem) {
    return;
  }
  list_elem_t* curr = ((list_meta_t*)list)->head;
  list_elem_t* prev = NULL;
  /* look for suitable position to insert this element */
  while (curr) {
    if (curr->key >= elem->key) {
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  elem->next = curr;
  if (!prev) {
    /* becomes new head */
    ((list_meta_t*)list)->head = elem;
  } else {
    prev->next = elem;
  }
}
/*---------------------------------------------------------------------------*/
bool list_insert(list_t list, uint32_t key, const void* data)
{
  if (!list || !data) {
    return false;
  }
  list_meta_t* l = (list_meta_t*)list;
  /* find a free spot */
  list_elem_t* elem = memb_allocate_block(l->list_mem);
  if (elem) {
    elem->key = key;
    /* copy data */
    uint32_t data_len = memb_get_block_size(l->list_mem) - sizeof(list_elem_t);
    memcpy(LIST_ELEM_DATA(elem), data, data_len);
    list_link(list, elem);
    return true;
  }
  return false;
}
/*---------------------------------------------------------------------------*/
bool list_remove(list_t list, const void* elem)
{
  if (!list || !elem) {
    return false;
  }
  list_meta_t* l    = (list_meta_t*)list;
  list_elem_t* curr = l->head;
  list_elem_t* prev = NULL;
  while (curr) {
    if (curr == elem) {
      /* adjust link */
      if (prev) {
        prev->next = curr->next;
      } else {
        /* it's the head */
        l->head = curr->next;
      }
      /* free the memory block */
      if (!memb_release_block(l->list_mem, curr)) {
        return false;
      }
      return true;
    }
    prev = curr;
    curr = curr->next;
  }
  return false;
}
/*---------------------------------------------------------------------------*/
/* will remove all elements with a given key (can be multiple) */
bool list_remove_by_key(list_t list, uint32_t key)
{
  if (!list) {
    return false;
  }
  list_meta_t* l    = (list_meta_t*)list;
  list_elem_t* curr = l->head;
  list_elem_t* prev = NULL;
  bool elem_found   = false;
  /* elements are sorted with their keys in ascending order */
  while (curr && curr->key <= key) {
    if (curr->key == key) {
      /* adjust link */
      if (prev) {
        prev->next = curr->next;
      } else {
        /* it's the head */
        l->head = curr->next;
      }
      /* free the memory block */
      if (!memb_release_block(l->list_mem, curr)) {
        return false;
      }
      elem_found = true;
      /* note: memb_release_block will not erase the memory, so the pointer is still valid at this point */
      curr = curr->next;
      continue;
    }
    prev = curr;
    curr = curr->next;
  }
  return elem_found;
}
/*---------------------------------------------------------------------------*/
bool list_remove_head(list_t list, void* data)
{
  if (!list) {
    return false;
  }
  list_meta_t* l = (list_meta_t*)list;
  if (l->head) {
    if (data) {
      /* copy into output buffer */
      uint32_t data_len = memb_get_block_size(l->list_mem) - sizeof(list_elem_t);
      memcpy(data, LIST_ELEM_DATA(l->head), data_len);
    }
    /* free the memory block */
    if (!memb_release_block(l->list_mem, l->head)) {
      return false;
    }
    /* note: memb_release_block will not erase the memory, so the pointer is still valid at this point */
    l->head = l->head->next;
    return true;
  }
  return false;
}
/*---------------------------------------------------------------------------*/
const void* list_get_head(const list_t list)
{
  if (!list || !((list_meta_t*)list)->head) {
    return NULL;
  }
  return LIST_ELEM_DATA(((list_meta_t*)list)->head);
}
/*---------------------------------------------------------------------------*/
uint32_t list_get_head_key(const list_t list)
{
  if (!list || !((list_meta_t*)list)->head) {
    return 0;
  }
  return ((list_meta_t*)list)->head->key;
}
/*---------------------------------------------------------------------------*/
