/*
 * FunctionCheck profiler
 * (C) Copyright 2000-2002 Yannick Perret 
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the FC_NU FC_eneral Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the FC_NU
 *  FC_eneral Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/** fc_memory.c:  **/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "fc_memory_manager.h"
#include "fc_com.h"
#include "fc_global.h"
#include "fc_hash.h"
#include "fc_tools.h"


/* indicate the call-stack requested for memory actions */
int fc_memory_stack_size = 4;

/* set the call-stack size requested */
int fc_memory_set_stack_size(int size)
{
    if (size < FC_MAX_CALLSTACK)
        fc_memory_stack_size = size;
    else
        fc_memory_stack_size = FC_MAX_CALLSTACK;

    return (1);
}

/* create a memory */
FC_Memory *fc_memory_create(int entry_size)
{
    FC_Memory *tmp;
    int i;

    tmp = malloc(sizeof (FC_Memory));
    if (tmp == NULL)
    {
        fc_message("cannot allocate %d bytes (memory)", sizeof (FC_Memory));
        return (NULL);
    }
    /* allocate the list of entries */
    tmp->max_elements = entry_size;
    tmp->nb_elements = 0;
    tmp->entry = 0;
    tmp->list = malloc(sizeof (FC_MEl) * entry_size);
    if (tmp->list == NULL)
    {
        fc_message("cannot allocate %d bytes (memory/list)", sizeof (FC_MEl) * entry_size);
        free(tmp);
        return (NULL);
    }
    /* initialize the entry list */
    for (i = 0; i < entry_size; i++)
    {/* link for free entries */
        tmp->list[i].next = i + 1;
        tmp->list[i].self = i;
        tmp->list[i].set = 0;
        tmp->list[i].alloc_stack[0] = NULL;
        tmp->list[i].pointer = NULL;
    }
    /* create the hash-table for access */
    tmp->hash = fc_hash_new();
    /* I need to check the NULL result ? */

    return (tmp);
}

/* destroy a memory */
int fc_memory_delete(FC_Memory *mem)
{
    if (mem == NULL)
        return (0);

    /* free the list of elements */
    if (mem->list != NULL)
        free(mem->list);
    /* remove the hash-table */
    if (mem->hash != NULL)
        fc_hash_destroy(mem->hash);
    free(mem);

    return (1);
}

/* add an element in a memory/list (with reallocation if needed) and
     return the address of the element for the hash-table */
FC_MEl *fc_memory_list_add(FC_Memory *mem)
{
    int i;

    /* with have to reallocate */
    if (mem->nb_elements == mem->max_elements)
    {
        mem->list = realloc(mem->list, sizeof (FC_MEl)*2 * mem->max_elements);
        if (mem->list == NULL)
        {/* argl! */
            mem->nb_elements = 0;
            mem->max_elements = 0;
            mem->entry = 0;
            fc_message("cannot reallocate entry list for memory managment");
            fc_message("memory profile data lost.");
            return (NULL);
        }
        /* init the available links */
        for (i = mem->max_elements; i < mem->max_elements * 2; i++)
        {
            mem->list[i].next = i + 1;
            mem->list[i].self = i;
            mem->list[i].set = 0;
            mem->list[i].alloc_stack[0] = NULL;
            mem->list[i].pointer = NULL;
        }
        mem->max_elements *= 2;
    }

    /* now the entry is 'entry' */
    i = mem->entry;
    /* next available */
    mem->entry = mem->list[mem->entry].next;
    mem->nb_elements++;

    mem->list[i].set = 1;

    return (&(mem->list[i]));
}

/* remove an element from the memory/list */
int fc_memory_list_remove(FC_Memory *mem, FC_MEl *el)
{
    /* still removed */
    if (el->pointer == NULL)
        return (0);

    /* remove the element */
    el->pointer = NULL;
    el->set = 0;
    el->next = mem->entry; /* next free is the actual entry */
    mem->entry = el->self; /* new entry */

    return (1);
}

/* record a 'malloc' entry */
int fc_memory_add_malloc(void *ctx, void *ptr, unsigned int size, void *where)
{
    FC_MEl *el;
    FC_Memory *mem;
    void *tmp, *b;
    int ret, a;

    mem = ((FC_Context*) ctx)->memory;

    /* allocation failed */
    if (ptr == NULL)
    {/* nothing to record */
        return (1);
    }

    /* first check if this pointer is available */
    ret = fc_hash_lookup(mem->hash, ptr, &a, &tmp, &b);
    if (ret)
    {
        fc_message("warning: pointer %p is still present as an active block!");
        fc_message("         overwriting previous entry.");
        el = (FC_MEl*) tmp;
    }
    else
    {/* prepare a new entry */
        el = fc_memory_list_add(mem);
        /* add the reference in the hash-table */
        fc_hash_insert(mem->hash, ptr, 0, (void*) el, NULL);
    }

    /* now set the fields */
    el->pointer = ptr;
    el->size = size;
    el->alloc_place = where;
    fc_get_top_stack(((FC_Context*) ctx)->stack, fc_memory_stack_size, el->alloc_stack);
    el->realloc_place = NULL;

    return (1);
}

/* record a 'memalign' entry */
int fc_memory_add_memalign(void *ctx, void *ptr, unsigned int align, unsigned int size, void *where)
{
    /* we treat it as a malloc action (align info is dropped) */
    return (fc_memory_add_malloc(ctx, ptr, size, where));
}

/* record a 'free' action */
int fc_memory_add_free(void *ctx, void *ptr, void *where)
{
    FC_MEl *el;
    FC_Memory *mem;
    int ret, a;
    void *tmp, *b;

    mem = ((FC_Context*) ctx)->memory;

    /* search the entry */
    ret = fc_hash_lookup(mem->hash, ptr, &a, &tmp, &b);
    if (!ret)
    {/* free on a unreferenced pointer */
        /* prepare a new entry */
        el = fc_memory_list_add(mem);
        el->pointer = ptr;
        el->alloc_place = where;
        el->realloc_place = NULL;
        fc_get_top_stack(((FC_Context*) ctx)->stack, fc_memory_stack_size, el->alloc_stack);
        el->size = UINT_MAX;
        /* remove the original from the hash-table */
        fc_hash_remove(mem->hash, ptr);
        return (1);
    }

    /* no more needs of this entry. remove */
    el = (FC_MEl*) tmp;
    fc_hash_remove(mem->hash, ptr);
    el->pointer = NULL;
    el->size = 0;
    el->set = 0;
    el->alloc_place = NULL;
    el->realloc_place = NULL;
    fc_memory_list_remove(mem, el);

    return (1);
}

/* record a 'realloc' entry */
int fc_memory_add_realloc(void *ctx, void *ptr, void *inc, unsigned int size, void *where)
{
    FC_MEl *el;
    FC_Memory *mem;
    void *tmp, *b;
    int a, ret;

    mem = ((FC_Context*) ctx)->memory;

    /* search the entry */
    ret = fc_hash_lookup(mem->hash, inc, &a, &tmp, &b);
    if (!ret)
    {/* realloc on a unreferenced pointer */
        /* prepare a new entry */
        el = fc_memory_list_add(mem);
        el->pointer = inc;
        el->realloc_place = where;
        fc_get_top_stack(((FC_Context*) ctx)->stack, fc_memory_stack_size, el->alloc_stack);
        el->alloc_place = NULL;
        el->size = UINT_MAX;
        return (1);
    }

    /* update the data */
    el = (FC_MEl*) tmp;
    el->realloc_place = where;
    el->pointer = ptr;
    /* just modify the hash-table entry */
    if (inc != ptr)
    {
        fc_hash_remove(mem->hash, inc);
        fc_hash_insert(mem->hash, ptr, 0, (void *) el, NULL);
    }

    return (1);
}

