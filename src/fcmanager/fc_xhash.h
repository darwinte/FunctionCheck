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
/** fc_xhash.h: all functions for hash-tables **/

#ifndef __fc_xhash_h_
#define __fc_xhash_h_

/*
   Notes:
     this is a particular kind of hash-table. when removing
     elements the size of the hash-table is never reduced.
     
     access to an element cost between 1 and 8 memory access
     (regardless the overcost in case of hash-table resizing)
*/

#include <stdio.h>
#include <stdlib.h>


/* structure of entries in a hash-table */
#define FC_HASH_DEPTH 8
typedef struct _FC_HNode FC_HNode;
struct _FC_HNode
{
  /* storage for the used key (for resize) */
  void *key;

  /* local data stored for the user */
  int  value;
  void *ptr1;
  void *ptr2;
  
  FC_HNode *next;
  FC_HNode *fnext;
};


/* structure of a hash-table */
#define FC_HASH_MAX_ADD 32
typedef struct
{
  int size;  /* current size of the hash-table */
  int nb;    /* number of elements in the hash-table */
  FC_HNode **nodes; /* list of entries */
  FC_HNode *rnodes; /* list of nodes */
  FC_HNode *add_rnodes[FC_HASH_MAX_ADD]; /* additionnal list of nodes */
  int nb_add_rnodes;/* position in additionnal list */
  int nb_rnodes;    /* used entries */
  int max_rnodes;   /* size */
  FC_HNode *free_entry;   /* next free entry */
  int frozen;       /* maw size reached, do not resize */
}FC_Hash;


/* a function to apply to each elements */
typedef void (*FC_HFunc)(void *key, int val, void *ptr1, void *ptr2, void *data);


/* create a hash-table */
FC_Hash *fc_hash_new(void);

/* destroy a hash-table */
void fc_hash_destroy(FC_Hash *hash);

/* insert an element in a hash-table */
void fc_hash_insert(FC_Hash *hash, void *key,
                    int val, void *ptr1, void *ptr2);

/* search an element in a hash-table */
int fc_hash_lookup(FC_Hash *hash, void *key,
                   int *val, void **ptr1, void **ptr2);

/* (!) search an element in a hash-table and return pointers
   on the entries (in order to modify their values without
   having to remove/modify/insert.
   WARNING: these pointers are only valid until you perfom
            an other action on this hash-table. after any
	    operation they may become invalid */
int fc_hash_lookup_modify(FC_Hash *hash, void *key,
                          int **val, void ***ptr1, void ***ptr2);

/* remove an element from a hash-table */
void fc_hash_remove(FC_Hash *hash, void *key);

/* apply a function to each elements of the hash-table */
void fc_hash_foreach(FC_Hash *hash, FC_HFunc func, void *data);

/* get the number of elements in the hash-table */
int fc_hash_size(FC_Hash *hash);

/* for debug purpose. prints the internal data inside the hash-table */
void fc_hash_debug(FC_Hash *hash);

#endif /* __fc_xhash_h_ */

