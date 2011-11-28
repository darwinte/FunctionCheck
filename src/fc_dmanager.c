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
/** fc_dmanager.c: started by the profiled program. reads and treats
                   the data sent by the profiled program, with
		   possibility to control the way profile is done **/

#include <limits.h>
#include "fc_global.h"
#include "fc_tools.h"
#include "fc_com_manager.h"
#include "fc_time.h"
#include "fc_context.h"
#include "fc_functions.h"
#include "fc_graph.h"
#include "fc_stack.h"
#include "fc_hash.h"




/* well... it's the main, ya know... */
int main(int argc, char*argv[])
{
  /* data returned at each 'read' */
  void *function, *from;
  unsigned int vtime;
  int id, ret;
  char type;
  FC_Context *ctmp;
  FC_Function *func;
  long long int ftime;
  /* init structure with all default values */
  FC_Init init;
  FC_LDyn ldyn;
  /* file name for the control program */
  char *control_file = NULL;


  /* set the prog name */
  fc_set_message_name(FC_DMANAGER_NAME);
  
  /* check the parameters */
  if (argc != 3)
    {
    fc_message("invalid arguments for the manager!");
    exit(FC_ERR_ARGS);
    }

  /* copy the control file */
  control_file = strdup(argv[2]);

  /* read and compile the control file */
  /**/

  /* init the communication system */
  fc_mcom_init(atoi(argv[1]), &id, &init);  /* id is the ID of the prog */

  /* init messages state */
  fc_set_message_mode(init.verbose);
  fc_set_debug_mode(init.debug);

  /* init default values regard to the init info */
  fc_context_set_functions(init.function_size);
  fc_context_set_graph(init.graph_size);
  fc_context_set_stack(init.stack_size);
  fc_context_set_usepid(init.use_pid);
  fc_context_set_name(init.dump_name);
  fc_context_set_path(init.dump_path);
  fc_context_set_starttime(init.start_time);

  /* if any read the list of dynamic libraries */
  if (init.follow)
    {
    fc_mcom_read_lib(&ldyn);
    while(ldyn.addr != NULL)
      {
      fc_ldyn_add(&ldyn);
      fc_mcom_read_lib(&ldyn);
      }
    }

  fc_message("Profile manager: running [%d]", (int)getpid());

  /* init the 1st context */
  fc_context_set(id, init.start_time);

  /* loop to read messages */
  while(1)
    {
    /* read data */
    ret = fc_mcom_read(&function, &from, &vtime, &id, &type);
    fc_debug("%p %p %u %d %d", function, from, vtime, id, (int)type);

    /* ret==0 => pipe closed => all clients are over */
    if (!ret)
      {/* leave the reading loop */
      fc_message("communication pipe closed. end of profiled program.");
      fc_context_set_stoptime(time(NULL));
      break;
      }

  
    /* set the corresponding context */
    fc_context_set(id, vtime);

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
      fc_current_context->pid = (int)vtime;
      }
    else /* standard message. treat the data */
      {
      /* first add the arc in the call-graph */
      fc_graph_add_single(fc_current_context->graph, from, function);
      /* treat the action */
      if (type == FC_TYPE_ENTER)
        {
	fc_functions_enter(function, fc_current_context->last_time);
	}
      else
      if (type == FC_TYPE_EXIT)
        {
	if (function != NULL) /* NULL -> just an exit from prog */
  	  fc_functions_exit(function, from, fc_current_context->last_time);
	else
	  {
	  fc_debug("process %d is over.", id);
	  }
	}
      else
        {
	fc_message("invalid message type (%d). ignored.", type);
	}
      }
    }

  /* for each context, flush the stack. the stack is not empty
     if the profiled program stops due to a crash -> all remaining
     functions are treated as if they exit now */
  ctmp = fc_context_first();
  while(ctmp != NULL)
    {
    fc_debug("context %p: flushing stack\n", ctmp);
    fc_current_context = ctmp;
    /* simulate exits for each remaining function */
    while(!fc_stack_empty(ctmp->stack))
      {
      fc_stack_get(ctmp->stack, &func, &ftime);
      if (func->symbol != NULL)
        fc_functions_exit(func->symbol, NULL, ctmp->last_time);
      else
        fc_stack_pop(ctmp->stack);
      }
    
    ctmp = fc_context_next(ctmp);
    }

  /* dump data */
  fc_debug("saving contexts");
  fc_context_save_all();

  /* stop communication system */
  fc_mcom_fini();

  /* exit */
  fc_message("Profile manager: exit");
  fc_debug("leaving fc_manager");
  return(FC_ERR_OK);
}
