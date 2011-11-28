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
/** fc_functions.h: treat enter/exit actions during profile **/

#ifndef __fc_functions_h_
#define __fc_functions_h_


/** structures **/
/* function descriptor */
typedef struct
{
  void *symbol;       /* symbol address */
  unsigned int calls; /* number of calls */
  unsigned long long local_time;   /* time spend in the function */
  unsigned long long total_time;  /* total time of execution */
  unsigned long long last_time;    /* the 'time' at last treatment */
  unsigned long long max_time;    /* the max total time spend in this fnc */
  unsigned long long min_time;    /* the min total time spend in this fnc */
  unsigned long long max_ltime;    /* the max local time spend in this fnc */
  unsigned long long min_ltime;    /* the min local time spend in this fnc */
  unsigned long long temp_ltime;   /* temporay local time */
  /* used for management during profile */
  int recursive_state;   /* number of time in the stack */
  int faked;             /* true if special fill-up fnc */
}FC_Function;



/** functions to manage list of functions **/

/* initialize a function (pre-allocated) */
void fc_functions_init(FC_Function *fnc);

/* treat entering in a function */
void fc_functions_enter(void *fnc, void *call_site, unsigned long long time);

/* treat exiting a function */
void fc_functions_exit(void *fnc, void *call_site, unsigned long long time);



#endif /* __fc_functions_h_ */
