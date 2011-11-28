/*
 * FunctionCheck profiler
 * (C) Copyright 2000-2002 Yannick Perret 
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/** fc_memory_manager.h:  **/

#ifndef __fc_memory_manager_h_
#define __fc_memory_manager_h_

#include <stdio.h>
#include <stdlib.h>
#include "fc_xhash.h"


/** structure of memory list entries **/
/* a such entry is created for each 'malloc/memalign'.
   for each realloc 'pointer' and 'realloc_place' are updated
   in case of free the entry is removed
   
   in case of free on a unknown entry (i.e. previously freed
     pointer) the entry is removed from the access table (hash-table)
     but not from the list (pointer/alloc place are used, and
     size is set to MAX_UINT)
*/
#define FC_MAX_CALLSTACK 8
typedef struct
{
  void *pointer;      /* the pointer on the memory block */
  unsigned int size;  /* orig. size of the block */
  void *alloc_place;  /* addr in prog. space of the allocation */
  void *realloc_place;/* addr in prog. space of the last reallocation */
  int next;           /* next entry */
  int self;           /* our own index for optimization */
  void *alloc_stack[FC_MAX_CALLSTACK]; /* call-stack for alloc place (max 8) */
  char set;
}FC_MEl;
typedef struct
{
  int nb_elements;
  int max_elements;
  int entry;
  FC_MEl *list;
  FC_Hash *hash;
}FC_Memory;


/* hum... this include MUST be here because some functions below
     use FC_Context structure, BUT FC_Context uses FC_Memory...
     so FC_Memory MUST be defined BEFORE including fc_context.h...
   In fact my .h have to be reorganized... */
#include "fc_context.h"


/* indicate the call-stack requested for memory actions */
extern int fc_memory_stack_size;



/* set the call-stack size requested */
int fc_memory_set_stack_size(int size);

/* create a memory */
FC_Memory *fc_memory_create(int entry_size);
/* destroy a memory */
int fc_memory_delete(FC_Memory *mem);
/* add an element in a memory/list (with reallocation if needed) and
     return the address of the element for the hash-table */
FC_MEl *fc_memory_list_add(FC_Memory *mem);
/* remove an element from the memory/list */
int fc_memory_list_remove(FC_Memory *mem, FC_MEl *el);

/* record a 'malloc' entry */
int fc_memory_add_malloc(void *ctx, void *ptr, unsigned int size, void *where);
/* record a 'memalign' entry */
int fc_memory_add_memalign(void *ctx, void *ptr, unsigned int align, unsigned int size, void *where);
/* record a 'free' action */
int fc_memory_add_free(void *ctx, void *ptr, void *where);
/* record a 'realloc' entry */
int fc_memory_add_realloc(void *ctx, void *ptr, void *inc, unsigned int size, void *where);


#endif /* __fc_memory_manager_h_ */
