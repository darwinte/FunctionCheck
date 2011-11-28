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
/* fc_context.h: manage contexts for profile stats */


/**
   description of the dump file format.
   {...} and empty lines are comments
**/
/*

{ general informations }

functioncheck_v3.0	{ header and version control }
unique_ID		{ unique string for a particular execution }
time_mode		{ unit used for time }
profile_mode		{ do fork/thread allowed during profile ? }
PID			{ PID of the profiled program }
PPID			{ PID of the father of the profiled program }
exec_time		{ time elapsed between 1st and last action }

{ call graph }

nb_elements			{ number of call arcs }
0xfrom 0xto nb			{ call arc (symbol address) }
0xfrom 0xto nb
...

{ functions stats }

nb_elements			{ number of functions }
0xfunction nb_calls self_time total_time MIN MAX lMIN lMAX
0xfunction nb_calls self_time total_time MIN MAX lMIN lMAX
...

{ list of dynamic library used }

nb_elements		{ number of libnames }
0xaddr_base libname
0xaddr_base libname
...

{ memory leaks }

nb_elements
0xblock_addr initial_size 0xlocation 0xrelocation Oxstack 0xstack ... NULL
0xblock_addr initial_size 0xlocation 0xrelocation Oxstack 0xstack ... NULL
...

{ invalid free }

nb_elements
0xlocation 0xpointer 0xstack 0xstack ... NULL
0xlocation 0xpointer 0xstack 0xstack ... NULL
...

{ invalid realloc }

nb_elements
0xlocation 0xpointer 0xstack 0xstack ... NULL
0xlocation 0xpointer 0xstack 0xstack ... NULL
...

*/


#ifndef __fc_context_h_
#define __fc_context_h_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fc_global.h"
#include "fc_com.h"
#include "fc_tools.h"
#include "fc_stack.h"
#include "fc_graph.h"
#include "fc_xhash.h"
#include "fc_xlhash.h"
#include "fc_hash.h"
#include "fc_time.h"
#include "fc_memory_manager.h"


/** strings **/


/** structure of a context **/
typedef struct
{
  int id;    /* ID of the current context */
  int pid;   /* ID of the parent context  */
  unsigned long long first_time; /* time for the first event */
  unsigned long long last_time;  /* time for the last event */
  unsigned long long ulast_time;  /* time for the last event (ID uint) */
  unsigned long long time_pad;   /* time to add the 'unsigned int' times */

  /* effective data about profile */
  FC_LHash *graph;  /* associated call-graph */
  FC_Stack *stack;    /* associated call stack */
  FC_FHash *functions; /* associated list of functions */
  FC_Memory *memory;  /* list of memory leaks */
}FC_Context;

/* the active context */
extern FC_Context *fc_current_context;


/** functions **/
/* create a new context */
FC_Context *fc_context_create(int id, unsigned int first, int stack_size,
                       int func_size, int graph_size, int memory_size);

/* delete a context */
void fc_context_delete(FC_Context *ctx);

/* set the current context regards to the ID (create it if needed) */
void fc_context_set(int id, unsigned int first);

/* set default values */
void fc_context_set_functions(int n);
void fc_context_set_graph(int n);
void fc_context_set_stack(int n);
void fc_context_set_memory(int n);
void fc_context_set_name(char *n);
void fc_context_set_path(char *n);
void fc_context_set_usepid(int n);
void fc_context_set_starttime(time_t start);
void fc_context_set_stoptime(time_t stop);
void fc_context_set_mode(int mode);


/* save a given context */
int fc_context_save(FC_Context *ctx);

/* save all existing contexts */
int fc_context_save_all();

/* loop on contexts */
FC_Context *fc_context_first();
FC_Context *fc_context_next();

/* to manage list of dynamic libraries */
int fc_ldyn_add(FC_LDYN *ldyn);


#endif /* __fc_context_h_ */
