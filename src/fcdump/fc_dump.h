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
/** fc_dump.h:  **/

#ifndef __fc_dump_h_
#define __fc_dump_h_

#include <stdio.h>
#include <stdlib.h>

#include "fc_global.h"
#include "fc_tools.h"


/** structures **/
/* names of a symbol */
typedef struct
{
  char *name;
  char *object;
  int line;
  int ok;
}FC_Name;
/* values for the 'ok' field */
#define FC_OK_NDEF 0  /* empty */
#define FC_OK_OK   1  /* all is done */
#define FC_OK_BAD  2  /* not ok, but don't touch it anymore */

/* functions */
typedef struct
{
  void *symbol;    /* symbol address */
  FC_Name name;    /* name structure */
  int calls;       /* number of calls */
  long long int local_time;   /* local time */
  long long int total_time;   /* total time */
  long long int max_time;
  long long int min_time;
  long long int max_ltime;
  long long int min_ltime;
  int hide;    /* do not use this function */
  void *node;  /* the node for this function */
  int my_index;  /* cross reference */
}FC_Function;

/* arcs */
typedef struct
{
  void *from, *to;
  int number;
  FC_Function *ffrom, *fto;
}FC_Arc;

/* dynamic libraries */
typedef struct
{
  void *address;
  char name[256];
}FC_LDyn;

/* structures for list of functions (name+addr) */
typedef struct
{
  char *name;  /* function name */
  char *file;  /* corresp. file (not implemented) */
  void *addr;  /* corresp. symbol */
}FC_NSym;


/* max size for a stack */
#define FC_MAX_STACK_SIZE 16
/* structure for memory leaks */
typedef struct
{
  void *pointer;  /* the memory block */
  void *alloc_place;  /* the allocation place */
  FC_Name alloc_name; /* name for the malloc */
  unsigned int size;  /* the initial size of the block */
  void *realloc_place;/* last reallocation place */
  FC_Name realloc_name;/* name for the realloc */
  void *alloc_stack[FC_MAX_STACK_SIZE]; /* call-stack for the allocation */
  FC_Name *stack_name;/* names for the call-stack */
}FC_MLeak;

/* structure for an invalid free */
typedef struct
{
  void *pointer;  /* the memory block given */
  void *free_place;   /* the free place */
  FC_Name free_name;  /* name for the free */
  void *free_stack[FC_MAX_STACK_SIZE]; /* call-stack for the free */
  FC_Name *stack_name;/* names for the call-stack */
}FC_MFree;

/* structure for an invalid realloc */
typedef struct
{
  void *pointer;  /* the memory block given */
  void *realloc_place;/* the realloc place */
  FC_Name realloc_name;/* name for the realloc */
  void *realloc_stack[FC_MAX_STACK_SIZE]; /* call-stack for the realloc */
  FC_Name *stack_name;/* names for the call-stack */
}FC_MRealloc;




#endif /* __fc_dump_h_ */
