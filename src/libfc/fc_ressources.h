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

/* fc_ressources: manage env. variables and ressource file
                  to get initial state for the profiler
*/

#ifndef __fc_ressources_h_
#define __fc_ressources_h_

#include <stdio.h>
#include <stdlib.h>


/** read env. variables and set values in corresponding
    variables. If not set, variables are unchanged        **/
int fc_read_env(int *fc_buffer_size,
                int *fc_stack_size,
                int *fc_function_size,
                int *fc_graph_size,
                int *fc_memory_size,
                char*fc_dump_path,
                char*fc_dump_name,
                char*fc_time_mode,
                int *fc_verbose_mode,
                int *fc_use_pid,
		int *fc_no_fork,
		int *fc_no_thread,
		int *fc_debug,
		int *give_help,
                int *use_memory,
                int *memory_stack);


/** read ressources file (if exist) and set env. variables
    with corresponding values (to be done before fc_read_env) **/
int fc_read_ressources();


#endif /* __fc_ressources_h_ */
