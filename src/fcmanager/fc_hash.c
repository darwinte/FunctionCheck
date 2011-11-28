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
/** fc_hash.c: manage hash tables for functions **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fc_global.h"
#include "fc_hash.h"
#include "fc_tools.h"

/* create a hash table with initial size */

/* size MUST be a power of 2 (not checked) */
FC_FHash *fc_fhash_create(int size)
{
    FC_FHash *tmp;
    int i;

    tmp = malloc(sizeof (FC_FHash));
    if (tmp == NULL)
    {
        fc_message("cannot allocate %d bytes for a hash table.", sizeof (FC_FHash));
        return (NULL);
    }
    /* create the elements */
    tmp->functions = malloc(sizeof (FC_Function) * size);
    if (tmp->functions == NULL)
    {
        fc_message("cannot allocate %d bytes for a hash table.", sizeof (FC_Function) * size);
        free(tmp);
    }
    memset((void*) tmp->functions, 0, sizeof (FC_Function) * size);

    tmp->current_size = size;
    tmp->current_mask = size - 1;
    tmp->current_nb = 0;

    for (i = 0; i < size; i++)
    {
        fc_functions_init(&(tmp->functions[i]));
    }

    return (tmp);
}

/* destroy a hash table */
void fc_fhash_delete(FC_FHash *hash)
{
    if (hash->functions != NULL)
        free(hash->functions);
    free(hash);
}

/* add an element in the hash table */
FC_Function *fc_fhash_insert(FC_FHash *hash, FC_Function *fnc)
{
    unsigned int pos;

    /* compute the position */
    pos = ((unsigned int) (fnc->symbol)) & hash->current_mask;

    /* hash table is full ? */
    if (((float) hash->current_nb / hash->current_size) > FC_HASH_MAX_RATIO)
    {
        fc_message_fatal(FC_ERR_HASH, "too many colisions in the hash table.");
    }

    /* search the next free entry */
    hash->current_nb++;
    while ((hash->functions[pos].symbol != NULL) || (hash->functions[pos].faked))
    {
        pos = (pos + 1) & hash->current_mask;
    }

    /* set the value */
    hash->functions[pos] = *fnc;
    return (&(hash->functions[pos]));
}

/* find an element in the hash table */
FC_Function *fc_fhash_find(FC_FHash *hash, void *fnc)
{
    unsigned int pos;

    /* compute the position */
    pos = ((unsigned int) (fnc)) & hash->current_mask;

    /* search the entry */
    while (hash->functions[pos].symbol != fnc)
    {
        /* the entry does not exist */
        if (hash->functions[pos].symbol == NULL)
            return (NULL);
        pos = pos + 1;
    }
    return (&(hash->functions[pos]));
}

