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
/** fc_hash.h: manage hash tables for functions **/

#ifndef __fc_hash_h_
#define __fc_hash_h_

#include <stdio.h>
#include <stdlib.h>
#include "fc_functions.h"


/* max fill-ratio in the hash table */
#define FC_HASH_MAX_RATIO   0.80


/* structure of a hash table */
typedef struct
{
  int current_size;
  int current_nb;
  int current_mask;
  FC_Function *functions;
}FC_FHash;


/** functions **/
/* create a hash table with initial size */
/* size MUST be a power of 2 (not checked) */
FC_FHash *fc_fhash_create(int size);

/* destroy a hash table */
void fc_fhash_delete(FC_FHash *hash);

/* add an element in the hash table */
FC_Function *fc_fhash_insert(FC_FHash *hash, FC_Function *fnc);

/* find an element in the hash table */
FC_Function *fc_fhash_find(FC_FHash *hash, void *fnc);

#endif /* __fc_hash_h_ */
