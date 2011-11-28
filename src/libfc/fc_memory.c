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
/** fc_memory.c:  **/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "fc_memory.h"
#include "fc_com.h"
#include "fc_global.h"
#include "fc_tools.h"


#ifdef HAVE_MALLOC_H
#ifdef HAVE_VALID_HOOKS

/** true if memory is not available **/
int fc_no_memory_hard = 0;

/* initial set of functions pointer for memory */
static __malloc_ptr_t(*old_malloc_hook) (size_t, __const __malloc_ptr_t);
static void (*old_free_hook) (__malloc_ptr_t, __const __malloc_ptr_t);
static __malloc_ptr_t(*old_realloc_hook) (__malloc_ptr_t, size_t, __const __malloc_ptr_t);
static __malloc_ptr_t(*old_memalign_hook) (size_t, size_t, __const __malloc_ptr_t);

/* prototypes for our memory hooks  */
static __malloc_ptr_t my_malloc_hook(size_t, __const __malloc_ptr_t);
static void my_free_hook(__malloc_ptr_t, __const __malloc_ptr_t);
static __malloc_ptr_t my_realloc_hook(__malloc_ptr_t, size_t, __const __malloc_ptr_t);
static __malloc_ptr_t my_memalign_hook(size_t, size_t, __const __malloc_ptr_t);

#else
/** true if memory is not available **/
int fc_no_memory_hard = 1;
#endif

#else
/** true if memory is not available **/
int fc_no_memory_hard = 1;
#endif


/* flag to see if we are running */
static int fc_active_memory = 0;

/* init the memory hooks for memory profiling */
int fc_memory_init(void)
{
    fc_active_memory = 1;
#ifdef HAVE_MALLOC_H
#ifdef HAVE_VALID_HOOKS

    /* store the original pointers */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    old_realloc_hook = __realloc_hook;
    old_memalign_hook = __memalign_hook;

    /* set our own pointers */
    __malloc_hook = my_malloc_hook;
    __free_hook = my_free_hook;
    __realloc_hook = my_realloc_hook;
    __memalign_hook = my_memalign_hook;
#endif
#endif

    return (1);
}

/* stop the memory hooks */
int fc_memory_fini(void)
{
    if (!fc_active_memory)
        return (1);

#ifdef HAVE_MALLOC_H
#ifdef HAVE_VALID_HOOKS
    /* just set the original pointers */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    __realloc_hook = old_realloc_hook;
    __memalign_hook = old_memalign_hook;
#endif
#endif

    fc_active_memory = 0;

    return (1);
}


#ifdef HAVE_MALLOC_H
#ifdef HAVE_VALID_HOOKS

/* my hooks */
__malloc_ptr_t my_malloc_hook(size_t size, __const __malloc_ptr_t where)
{
    __malloc_ptr_t *result;

    /* restore the old hook */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    __realloc_hook = old_realloc_hook;
    __memalign_hook = old_memalign_hook;

    /* real call */
    result = malloc(size);

    /* re-save underlaying hook (cause it may change!) */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    old_realloc_hook = __realloc_hook;
    old_memalign_hook = __memalign_hook;

    /* my treatments (protected, as they may call malloc) */
    fc_com_malloc((void*) result, (unsigned int) size, (void*) where);

    /* restore our own hooks */
    __memalign_hook = my_memalign_hook;
    __realloc_hook = my_realloc_hook;
    __free_hook = my_free_hook;
    __malloc_hook = my_malloc_hook;

    return (result);
}

void my_free_hook(__malloc_ptr_t incoming, __const __malloc_ptr_t where)
{
    /* restore the old hook */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    __realloc_hook = old_realloc_hook;
    __memalign_hook = old_memalign_hook;

    /* real call */
    free(incoming);

    /* re-save underlaying hook (cause it may change!) */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    old_realloc_hook = __realloc_hook;
    old_memalign_hook = __memalign_hook;

    /* my treatments (protected, as they may call free) */
    fc_com_free((void*) incoming, (void*) where);

    /* restore our own hooks */
    __memalign_hook = my_memalign_hook;
    __realloc_hook = my_realloc_hook;
    __free_hook = my_free_hook;
    __malloc_hook = my_malloc_hook;
}

__malloc_ptr_t my_realloc_hook(__malloc_ptr_t incoming, size_t size, __const __malloc_ptr_t where)
{
    __malloc_ptr_t *result;

    /* restore the old hook */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    __realloc_hook = old_realloc_hook;
    __memalign_hook = old_memalign_hook;

    /* real call */
    result = realloc(incoming, size);

    /* re-save underlaying hook (cause it may change!) */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    old_realloc_hook = __realloc_hook;
    old_memalign_hook = __memalign_hook;

    /* my treatments (protected, as they may call realloc) */
    fc_com_realloc((void*) result, (void*) incoming,
                   (unsigned int) size, (void*) where);

    /* restore our own hooks */
    __memalign_hook = my_memalign_hook;
    __realloc_hook = my_realloc_hook;
    __free_hook = my_free_hook;
    __malloc_hook = my_malloc_hook;

    return (result);
}

__malloc_ptr_t my_memalign_hook(size_t size, size_t align, __const __malloc_ptr_t where)
{
    __malloc_ptr_t *result;

    /* restore the old hook */
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;
    __realloc_hook = old_realloc_hook;
    __memalign_hook = old_memalign_hook;

    /* real call */
    result = memalign(align, size);

    /* re-save underlaying hook (cause it may change!) */
    old_malloc_hook = __malloc_hook;
    old_free_hook = __free_hook;
    old_realloc_hook = __realloc_hook;
    old_memalign_hook = __memalign_hook;

    /* my treatments (protected, as they may call memalign) */
    fc_com_memalign((void*) result, (unsigned int) align,
                    (unsigned int) size, (void*) where);

    /* restore our own hooks */
    __memalign_hook = my_memalign_hook;
    __realloc_hook = my_realloc_hook;
    __free_hook = my_free_hook;
    __malloc_hook = my_malloc_hook;

    return (result);
}
#endif
#endif


