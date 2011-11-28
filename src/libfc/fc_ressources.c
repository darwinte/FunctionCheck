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

#include "fc_ressources.h"
#include <string.h>


/* switch for getenv (if not defined) */
#ifndef FC_NO_GETENV
#define FC_GETENV(n) getenv(n)
#else
#define FC_GETENV(n) NULL
#endif

/* switch for setenv (if not defined) */
#ifndef FC_NO_SETENV
#define FC_SETENV(n,v,o) setenv((n),(v),(o))
#else
#define FC_SETENV(n,v,o) {}  /*empty command */
#endif

/* switch for unsetenv (if not defined) */
#ifndef FC_NO_UNSETENV
#define FC_UNSETENV(n) unsetenv(n)
#else
#define FC_UNSETENV(n) {}  /*empty command */
#endif

/* get env. variable 'var' and convert in integer */
int fc_get_env_integer(char *var, int *nb)
{
    char *tmp;

    if ((tmp = FC_GETENV(var)) == NULL)
        return (0);

    sscanf(tmp, "%d", nb);
    return (1);
}

/* get env. variable 'var' and convert in bool */
int fc_get_env_bool(char *var)
{
    if (FC_GETENV(var) == NULL)
        return (0);

    return (1);
}

/* get env. variable 'var' and convert in float */
int fc_get_env_float(char *var, float *nb)
{
    char *tmp;

    if ((tmp = FC_GETENV(var)) == NULL)
        return (0);

    sscanf(tmp, "%f", nb);
    return (1);
}

/* get env. variable 'var' and convert in char* (must be pre-allocated) */
int fc_get_env_char(char *var, char *nb)
{
    char *tmp;

    if ((tmp = FC_GETENV(var)) == NULL)
        return (0);

    nb[0] = '\0';
    strcat(nb, tmp);
    return (1);
}

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
                int *memory_stack)
{
    /* no verbose */
    if (fc_get_env_bool("FC_QUIET"))
        *fc_verbose_mode = 0;

    /* help */
    if (fc_get_env_bool("FC_HELP"))
        *give_help = 1;

    /* memory profiling */
    if (fc_get_env_bool("FC_MEMORY"))
        *use_memory = 1;

    /* the size of the buffer (for communications) */
    fc_get_env_integer("FC_MEMORY_STACK", memory_stack);

    /* the size of the buffer (for communications) */
    fc_get_env_integer("FC_BUFFER_SIZE", fc_buffer_size);

    /* the size of the stack */
    fc_get_env_integer("FC_STACK_SIZE", fc_stack_size);

    /* the size of the function list */
    fc_get_env_integer("FC_FUNCTION_SIZE", fc_function_size);

    /* the size of the function list */
    fc_get_env_integer("FC_GRAPH_SIZE", fc_graph_size);

    /* the size of the memory-entry list */
    fc_get_env_integer("FC_MEMORY_SIZE", fc_memory_size);

    /* the time unit to use */
    fc_get_env_char("FC_TIME_MODE", fc_time_mode);

    /* the base name of the dump file */
    fc_get_env_char("FC_DUMP_NAME", fc_dump_name);

    /* the path for each output files */
    fc_get_env_char("FC_DUMP_PATH", fc_dump_path);

    /* no forks */
    if (fc_get_env_bool("FC_NO_FORK"))
        *fc_no_fork = 1;

    /* allow threads */
    if (fc_get_env_bool("FC_ALLOW_THREAD"))
        *fc_no_thread = 0;

    /* use PID for dump file name (for multiple profiles) */
    if (fc_get_env_bool("FC_USE_PID"))
        *fc_use_pid = 1;

    /* switch to debug mode */
    if (fc_get_env_bool("FC_DEBUG"))
        *fc_debug = 1;

    /* disable messages */
    if (fc_get_env_bool("FC_NO_VERBOSE"))
        *fc_verbose_mode = 0;

    return (1);
}

/** read ressources file (if exist) and set env. variables
    with corresponding values (to be done before fc_read_env) **/
int fc_read_ressources()
{
    FILE *f;
    char temp[1024];
    char keyw[1024], val[1024];

    if ((f = fopen("./.functioncheckrc", "r")) == NULL)
        return (0);

    while (1)
    {
        /* read a line from file */
        fgets(temp, 1023, f);
        /* EOF: stop */
        if (feof(f))
            break;
        /* get VARIABLE VALUE pair */
        sscanf(temp, "%s%s", keyw, val);
        /* check if the VARIABLE is a FC_ one */
        if (strncmp(keyw, "FC_", 3) == 0)
        {
            /* set this value as an env. variable */
            if (strcmp(val, "!") == 0)
            {/* special case: unset the variable */
                FC_UNSETENV(keyw);
            }
            else
            {
                FC_SETENV(keyw, val, 1);
            }
        }
    }


    fclose(f);
    return (1);
}

