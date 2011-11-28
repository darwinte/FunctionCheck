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
/** fc_stack.c: manage stack operations  **/

#include <stdio.h>
#include <stdlib.h>
#include "fc_stack.h"


/* number of elements to add in the stack */
#define FC_STACK_STEP  256

/* create an epmty stack */
FC_Stack *fc_stack_create(int size)
{
    FC_Stack *tmp;

    if (size <= 0)
    {
        fc_message("cannot create an empty stack.");
        return (NULL);
    }

    tmp = malloc(sizeof (FC_Stack));
    if (tmp == NULL)
    {
        fc_message("cannot allocate %d bytes for a stack.", sizeof (FC_Stack));
        return (NULL);
    }

    /* create a first set of elements */
    tmp->stack = malloc(sizeof (FC_SEl) * size);
    if (tmp->stack == NULL)
    {
        fc_message("cannot allocate %d bytes for a stack.", sizeof (FC_SEl) * size);
        free(tmp);
        return (NULL);
    }
    tmp->nb_elements = 0;
    tmp->real_size = size;

    return (tmp);
}

/* destroy a stack */
void fc_stack_delete(FC_Stack *stack)
{
    if (stack == NULL)
        return;

    if (stack->stack != NULL)
        free(stack->stack);
    free(stack);
}

/* add an element in the stack */
int fc_stack_push(FC_Stack *stack, FC_Function *el, unsigned long long time, void *call_site)
{
    if (stack->nb_elements >= stack->real_size)
    {
        stack->stack = realloc(stack->stack, sizeof (FC_SEl)*(stack->real_size + FC_STACK_STEP));
        if (stack->stack == NULL)
        {
            fc_message("cannot reallocate %d bytes for a stack.", sizeof (FC_SEl)*(stack->real_size + FC_STACK_STEP));
            stack->nb_elements = 0;
            stack->real_size = 0;
            return (0);
        }
        stack->real_size += FC_STACK_STEP;
    }

    /* insert the element */
    fc_debug("stack: insert %p at %d", el->symbol, stack->nb_elements);
    stack->stack[stack->nb_elements].function = el;
    stack->stack[stack->nb_elements].call_site = call_site;
    stack->stack[stack->nb_elements++].time = time;

    return (1);
}

/* remove the upper element */
int fc_stack_pop(FC_Stack *stack)
{
    if (stack->nb_elements == 0)
    {
        fc_message("pop action on an empty stack! ignored.");
        return (0);
    }
    stack->nb_elements--;
    return (1);
}

/* get and remove the upper element */
int fc_stack_get_and_pop(FC_Stack *stack, FC_Function **el, unsigned long long *time)
{
    if (stack->nb_elements == 0)
    {
        fc_message("pop/get action on an empty stack! ignored.");
        return (0);
    }
    stack->nb_elements--;
    *el = stack->stack[stack->nb_elements].function;
    *time = stack->stack[stack->nb_elements].time;
    return (1);
}

/* get the upper element */
int fc_stack_get(FC_Stack *stack, FC_Function **el, unsigned long long *time, void **call_site)
{
    if (stack->nb_elements == 0)
    {
        fc_message("get action on an empty stack! ignored.");
        *el = NULL;
        return (0);
    }
    *el = stack->stack[stack->nb_elements - 1].function;
    *time = stack->stack[stack->nb_elements - 1].time;
    *call_site = stack->stack[stack->nb_elements - 1].call_site;
    return (1);
}

/* get the upper-1 element */
int fc_stack_getp(FC_Stack *stack, FC_Function **el, unsigned long long *time)
{
    if (stack->nb_elements == 1)
    {
        fc_message("getp action on a single-element stack! ignored.");
        *el = NULL;
        return (0);
    }
    *el = stack->stack[stack->nb_elements - 2].function;
    *time = stack->stack[stack->nb_elements - 2].time;

    return (1);
}

/* get the number of elements in the stack */
int fc_stack_size(FC_Stack *stack)
{
    return (stack->nb_elements);
}

/* true if the stack is empty */
int fc_stack_empty(FC_Stack *stack)
{
    return (stack->nb_elements == 0);
}

/* true if the element is still present */
int fc_stack_here(FC_Stack *stack, void *el)
{
    int i;

    if (stack->nb_elements == 0)
        return (0);

    for (i = stack->nb_elements - 1; i >= 0; i--)
    {
        if (stack->stack[i].function == el)
            return (1);
    }
    return (0);
}

/* set the given void* list (NULL termined) with the current
     call-stack (real call-site, not fnc address)
   Note: lst must have at least nb_max elements */
int fc_get_top_stack(FC_Stack *stack, int nb_max, void **lst)
{
    int i, pos;

    pos = 0;
    for (i = stack->nb_elements - 1; i >= 0; i--)
    {
        lst[pos++] = stack->stack[i].call_site;
        if (pos >= nb_max)
            break; /* no more needed */
    }
    /* NULL terminated (only if not full) */
    if (pos < nb_max)
        lst[pos] = NULL;

    return (1);
}

