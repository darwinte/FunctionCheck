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
/** fc_functions.c: treat enter/exit actions during profile **/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "fc_global.h"
#include "fc_com_manager.h"
#include "fc_functions.h"
#include "fc_context.h"
#include "fc_hash.h"
#include "fc_tools.h"

#define     LLONG_MAX       9223372036854775807LL

/* initialize a function (pre-allocated) */
void fc_functions_init(FC_Function *fnc)
{
    fnc->symbol = NULL;
    fnc->calls = 0;
    fnc->local_time = fnc->total_time = 0;
    fnc->recursive_state = 0;
    fnc->min_time = (LLONG_MAX * 2ULL + 1);
    fnc->max_time = 0;
    fnc->min_ltime = (LLONG_MAX * 2ULL + 1);
    fnc->max_ltime = 0;
    fnc->temp_ltime = 0;
}

/* treat entering in a function */
void fc_functions_enter(void *fnc, void *call_site, unsigned long long time)
{
    FC_Function tmp;
    FC_Function *func, *pfunc;
    unsigned long long ptime;

    /* 1st check if the function exists */
    if (fnc == NULL)
        func = NULL;
    else
        func = fc_fhash_find(fc_current_context->functions, fnc);
    if (func == NULL)
    {/* if not create one and insert it in the table */
        fc_debug("enter: creating function %p", fnc);
        tmp.symbol = fnc;
        tmp.calls = 0;
        tmp.local_time = 0;
        tmp.max_time = 0;
        tmp.min_time = (LLONG_MAX * 2ULL + 1);
        tmp.min_ltime = (LLONG_MAX * 2ULL + 1);
        tmp.max_ltime = 0;
        tmp.total_time = 0;
        tmp.recursive_state = 0;
        if (fnc == NULL)
            tmp.faked = 1;
        else
            tmp.faked = 0;
        func = fc_fhash_insert(fc_current_context->functions, &tmp);
    }
        /* debug */
    else
        fc_debug("enter: finding function %p", fnc);

    /* clear temporary local time */
    if (func->recursive_state == 0)
        func->temp_ltime = 0;

    /* do not change the order of these instructions */

    /* insert function in the stack */
    fc_stack_push(fc_current_context->stack, func, time, call_site);
    /* if we have a parent, compute local time for the parent */
    if (fc_stack_size(fc_current_context->stack) > 1)
    {
        fc_stack_getp(fc_current_context->stack, &pfunc, &ptime);
        fc_debug("enter: prev. func=%p. adding %u to local", pfunc->symbol, time - pfunc->last_time);
        /* add the local time */
        pfunc->temp_ltime += time - pfunc->last_time;
    }

    /* set changes */
    func->recursive_state++;
    func->calls++;
    func->last_time = time; /* last active time */
    fc_debug("enter: now: rec=%d, calls=%d, last=%u",
             func->recursive_state, func->calls, func->last_time);
}

/* treat exiting a function */
void fc_functions_exit(void *fnc, void *call_site, unsigned long long time)
{
    FC_Function *func;
    unsigned long long delta, ftime;
    FC_Function *pfunc;
    unsigned long long ptime;
    void *nuse;

    /* 1st, check if the top of the stack correspond to this function */
    fc_stack_get(fc_current_context->stack, &func, &ftime, &nuse);
    if (func == NULL) /* empty stack! */
    {
        fc_message("exit from function %p but stack is empty! ignored.", fnc);
        fc_message("  this may means that your program uses fork or");
        fc_message("  threads in SINGLE mode or that you are in FORK");
        fc_message("  mode with more than 16 exits after a fork()");
        return;
    }
    if ((func->symbol != fnc) && (func->symbol != NULL))
    {
        /* special: if it is a faked function, we now get it's
             real symbol address. set it and remove 'fake' flag */
        if (func->faked)
        {
            fc_debug("exit from faked %p. now %p.", func->symbol, fnc);
            func->symbol = fnc;
            func->faked = 0;
        }
        else /* really a bug! */
        {
            fc_message("exit from function %p but top of the stack is about function %p! Ignored.", fnc, func->symbol);
            return;
        }
    }
    if (func->symbol == NULL)
    {
        /* this function is here to allow exit from functions
             after a fork in the child context (these functions
             cannot be known before the exit time)  */
        /* set the symbol, even if it is not exactly the good one */
        func->symbol = fnc;
        if (fc_stack_size(fc_current_context->stack) > 1)
        {
            fc_stack_getp(fc_current_context->stack, &pfunc, &ptime);
            pfunc->symbol = call_site;
        }
    }

    /** treat the function **/

    /* compute elapsed time */
    delta = time - ftime;
    fc_debug("exit: from %p (delta=%u)", func->symbol, delta);

    /* this function may have local time to treat */
    func->temp_ltime += time - func->last_time;

    if (func->recursive_state <= 1)
        func->local_time += func->temp_ltime;

    /* min/max local time */
    if (func->temp_ltime > func->max_ltime)
        func->max_ltime = func->temp_ltime;
    if (func->temp_ltime < func->min_ltime)
        func->min_ltime = func->temp_ltime;

    /* if exist, re-set the last active time */
    if (fc_stack_size(fc_current_context->stack) > 1)
    {
        fc_stack_getp(fc_current_context->stack, &pfunc, &ptime);
        /* re-set last time */
        fc_debug("exit: re-set last time of %p", pfunc->symbol);
        pfunc->last_time = time;
    }

    /* treat MIN/MAX */
    if (delta > func->max_time)
        func->max_time = delta;
    if (delta < func->min_time)
        func->min_time = delta;

    /* if not a recursive function */
    /* NOTE: recursive functions MUST NOT be treated here.
    else, a 1s fnc calling itself 4 times will have a total
    time of 4+3+2+1=10s. it would generate erroneous total
    execution time as a total a 4s is really spend in this
    part of the call-tree */
    if (func->recursive_state <= 1)
    {/* add total time */
        fc_debug("exit: non recursive. adding total");
        func->total_time += delta;
    }

    /* update the state of the function */
    func->recursive_state--;

    /* remove the function from the stack */
    fc_stack_pop(fc_current_context->stack);
}


