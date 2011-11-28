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
/** fc_stack.h: manage stack operations **/

#ifndef __fc_stack_h_
#define __fc_stack_h_

#include <stdio.h>
#include <stdlib.h>

#include "fc_tools.h"
#include "fc_functions.h"


/** structures for stack **/
/* element of a stack */
typedef struct
{
  FC_Function *function;
  void *call_site;
  unsigned long long time;
}FC_SEl;
typedef struct
{
  FC_SEl *stack;
  unsigned long long *stack_time;
  int nb_elements;
  int real_size;
}FC_Stack;


/** functions **/
/* create an epmty stack */
FC_Stack *fc_stack_create(int size);

/* destroy a stack */
void fc_stack_delete(FC_Stack *stack);

/* add an element in the stack */
int fc_stack_push(FC_Stack *stack, FC_Function *el, unsigned long long time, void *call_site);

/* remove the upper element */
int fc_stack_pop(FC_Stack *stack);

/* get and remove the upper element */
int fc_stack_get_and_pop(FC_Stack *stack, FC_Function **el, unsigned long long *time);

/* get the upper element */
int fc_stack_get(FC_Stack *stack, FC_Function **el, unsigned long long *time, void **call_site);

/* get the upper-1 element */
int fc_stack_getp(FC_Stack *stack, FC_Function **el, unsigned long long *time);

/* get the number of elements in the stack */
int fc_stack_size(FC_Stack *stack);

/* true if the stack is empty */
int fc_stack_empty(FC_Stack *stack);

/* true if the element is still present */
int fc_stack_here(FC_Stack *stack, void *el);

/* set the given void* list (NULL termined) with the current
     call-stack (real call-site, not fnc address) */
int fc_get_top_stack(FC_Stack *stack, int nb_max, void **lst);




#endif /* __fc_stack_h_ */
