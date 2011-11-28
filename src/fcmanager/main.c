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
/** main.c: started by the profiled program. reads and treats
                  the data sent by the profiled program. **/

#include <limits.h>
#include "fc_global.h"
#include "fc_tools.h"
#include "fc_com_manager.h"
#include "fc_time.h"
#include "fc_context.h"
#include "fc_functions.h"
#include "fc_graph.h"
#include "fc_stack.h"
#include "fc_xhash.h"
#include "fc_memory_manager.h"

/* well... it's the main, ya know... */
int main(int argc, char*argv[])
{
    /* data returned at each 'read' */
    void *function, *from, *incoming, *ptr, *where;
    unsigned int size, align;
    unsigned long long vtime;
    char name[1024];
    int shmid, id, ret, parent;
    char type;
    FC_Context *ctmp;
    FC_Function *func;
    unsigned long long ftime;
    /* init structure with all default values */
    FC_INIT init;
    FC_LDYN ldyn;
    /* special */
    void *call_site;
    /* special case for thread: indicates that QUIT is done */
    /*
    int thread_quit_main = 0, thread_id_init = 0;
    int thread_mode_init = 0;
     */

    /* set the prog name */
    fc_set_message_name(FC_MANAGER_NAME);

    /* check the parameters */
    if (argc != 2)
    {
        fc_message("invalid arguments for the manager!");
        exit(FC_ERR_ARGS);
    }

    /* init the communication system */
    shmid = atoi(argv[1]);
    fc_mcom_init(shmid, &id, &init); /* id is the ID of the prog */

    /* init messages state */
    fc_set_message_mode(init.verbose);
    fc_set_debug_mode(init.debug);

    /* init default values regard to the init info */
    fc_context_set_functions(init.function_size);
    fc_context_set_graph(init.graph_size);
    fc_context_set_stack(init.stack_size);
    fc_context_set_memory(init.memory_size);
    fc_context_set_usepid(init.use_pid);
    fc_context_set_name(init.dump_name);
    fc_context_set_path(init.dump_path);
    fc_memory_set_stack_size(init.memory);
    fc_context_set_mode(init.mode);

    /* if any read the list of dynamic libraries */
    if (init.follow)
    {
        fc_mcom_read_lib(&ldyn);
        while (ldyn.addr != NULL)
        {
            /* debug */
            fc_message("Dyn lib:   %s [0x%x]", ldyn.name[0] ? ldyn.name : "NULL", ldyn.addr);
            fc_ldyn_add(&ldyn);
            fc_mcom_read_lib(&ldyn);
        }
    }
    
    fc_message("Profile manager: running [%d]", (int) getpid());

    /* init the 1st context */
    fc_context_set(id, init.start_time);

    /* set starting time */
    fc_context_set_starttime(time(NULL));

    /* loop to read messages */
    while (1)
    {
        /* read data */
        /* removed. may not be useful now */
        /*
        if ((nb_children <= 0)||(thread_quit_main))
          {
          fc_message("no more children referenced. end of profiled program.");
          fc_context_set_stoptime(time(NULL));
          break;
          }
         */
        ret = fc_mcom_read(&function, &from, &vtime, &id, &type,
                           &ptr, &incoming, &where, &parent, &size, &align, name);

        fc_debug("%p %p %u %d %d", function, from, vtime, id, (int) type);
        /* ret==0 => pipe closed => all clients are over */
        if (!ret)
        {/* leave the reading loop */
            fc_message("communication FIFO closed. end of profiled program.");
            fc_context_set_stoptime(time(NULL));
            break;
        }

        /** special messages **/
        /* fork notification */
        if (type == FC_TYPE_FORK)
        {
            /* set the context in order to
                1/  create it as it is a new one
                2/  set the starting time with the good value
             */
            fc_context_set(parent, vtime);

            continue; /* just read next event */
        }
        /* thread creation notification */
        if (type == FC_TYPE_THREAD)
        {
            /* set the context in order to
                1/  create it as it is a new one
                2/  set the starting time with the good value
             */
            fc_context_set(parent, vtime);

            continue; /* just read next event */
        }


        /* set the corresponding context */
        fc_context_set(id, vtime);
        /* if QUIT event, only the time is used, so read next event */
        if (type == FC_TYPE_QUIT)
        {
            fc_debug("QUIT message (%d). number of children is now (not yet implemented).", id);
            continue;
            /*nb_children--;
            if ((thread_mode_init == FC_MODE_THREAD)&&(id == thread_id_init))
              {
              fc_debug("QUIT message from the main thread. Forced exit.");
              thread_quit_main = 1;
              }
            continue;
             */
        }

        /* set the last time found for this context */
        fc_current_context->ulast_time = vtime;
        /* test if the time loops */
        if (vtime < fc_current_context->ulast_time)
        {
            fc_current_context->time_pad += UINT_MAX + 1;
        }
        fc_current_context->last_time = fc_current_context->time_pad + vtime;

        /* special message to inform about the process aprent */
        if (type == FC_TYPE_PARENT)
        {/* information on the parent process */
            fc_current_context->pid = (int) parent;
        }
        else /* standard message. treat the data */
        {
            /* treat the action */
            if (type == FC_TYPE_ENTER)
            {
                fc_functions_enter(function, from, fc_current_context->last_time);
            }
            else
                if (type == FC_TYPE_EXIT)
            {
                if (function != NULL) /* NULL -> just an exit from prog */
                {
                    /* first add the arc in the call-graph */
                    fc_debug("add arc %p -> %p", from, function);
                    fc_graph_add_single(fc_current_context->graph, from, function);
                    fc_functions_exit(function, from, fc_current_context->last_time);
                }
                else
                {
                    fc_debug("process %d is over.", id);
                }
            }
            else
                if (type == FC_TYPE_MALLOC)
            {
                fc_memory_add_malloc(fc_current_context, ptr, size, where);
            }
            else
                if (type == FC_TYPE_FREE)
            {
                fc_memory_add_free(fc_current_context, ptr, where);
            }
            else
                if (type == FC_TYPE_REALLOC)
            {
                fc_memory_add_realloc(fc_current_context, ptr, incoming, size, where);
            }
            else
                if (type == FC_TYPE_MEMALIGN)
            {
                fc_memory_add_memalign(fc_current_context, ptr, align, size, where);
            }
            else
            {
                if ((type == FC_TYPE_DLOPEN) || (type == FC_TYPE_DLCLOSE) ||
                        (type == FC_TYPE_DLSYM))
                {
                    fc_message("this message type (%d) is not implemented.", (int) type);
                }
                else
                {
                    fc_message("invalid message type (%d). ignored.", type);
                }
            }
        }
    }

    /* for each context, flush the stack. the stack is not empty
       if the profiled program stops due to a crash -> all remaining
       functions are treated as if they exit now */
    ctmp = fc_context_first();
    while (ctmp != NULL)
    {
        fc_debug("context %p: flushing stack", ctmp);
        fc_current_context = ctmp;
        /* simulate exits for each remaining function */
        while (!fc_stack_empty(ctmp->stack))
        {
            fc_stack_get(ctmp->stack, &func, &ftime, &call_site);
            if (func->symbol != NULL)
            {
                fc_graph_add_single(fc_current_context->graph, call_site, func->symbol);
                fc_functions_exit(func->symbol, NULL, ctmp->last_time);
            }
            else
                fc_stack_pop(ctmp->stack);
        }

        ctmp = fc_context_next(ctmp);
    }

    /* dump data */
    fc_debug("saving contexts");
    fc_context_save_all();

    /* stop communication system */
    fc_mcom_fini(shmid);

    /* exit */
    fc_message("Profile manager: exit");
    fc_debug("leaving fcmanager");
    return (FC_ERR_OK);
}
