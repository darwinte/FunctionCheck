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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define __USE_GNU
#include <link.h>
#ifndef FC_NO_DLFCN
#include <dlfcn.h>
#endif
#ifndef FC_NO_THREAD
#include <pthread.h>
#endif

#include "fc_global.h"
#include "fc_tools.h"
#include "fc_time.h"
#include "fc_ressources.h"
#include "fc_com.h"
#include "fc_memory.h"

/** IPC shared memory id **/
unsigned int fc_shared_memory_id = 0;

/** the initial number of dynamic libraries sent to the manager **/
int fc_initial_libraries = 0;


/** profiling mode (used by _fini) **/
static int fc_local_mode = 0;

/* help: print a reminder for available env. variables */
void fc_help(void)
{
    fc_message("%s V%s by Y.Perret", FC_PACKAGE, FC_VERSION);
    fc_message("Available environment variables are:");
    fc_message("FC_HELP                  : this message");
    fc_message("FC_QUIET | FC_NO_VERBOSE : disable messages");
    fc_message("FC_BUFFER_SIZE           : set the number of elements in FIFO buffer");
    fc_message("FC_STACK_SIZE            : set the default stack size");
    fc_message("FC_FUNCTION_SIZE         : set the size of functions table");
    fc_message("FC_GRAPH_SIZE            : set the call-graph default size");
    fc_message("FC_MEMORY_SIZE           : set the memory-table default size");
    fc_message("FC_TIME_MODE             : time unit used (tsc|ext|cpu|sys)");
    fc_message("FC_DUMP_NAME             : name for profile data file");
    fc_message("FC_DUMP_PATH             : path for profile data file");
    fc_message("FC_NO_FORK               : switch to single-process mode");
    fc_message("FC_ALLOW_THREAD          : switch to thread-only program");
    fc_message("FC_USE_PID               : always add PID to profile data file");
    fc_message("FC_DEBUG                 : switch on debug mode");
    fc_message("FC_MEMORY                : enable memory profiling");
    fc_message("FC_MEMORY_STACK          : set the call-stack size to store");
}

/* check if the given number is a power of 2. if not,
     set it to the first higher power of 2 */
void fc_check_power(int *nb)
{
    int n, i;

    n = 2;
    for (i = 1; (i < 31) && (n <= *nb); i++)
    {
        if (n == *nb)
            return; /* ok */
        n *= 2;
    }
    /* no match. set nb to n */
    *nb = n;
}


/** redirections for important functions **/

/* dlfnc familly */

/* redirection of the dlopen */
void *fc_redirect_dlopen(const char *filename, int flag)
{
    void *result;

    /* call dlopen */
#ifdef FC_NO_DLFCN
    fc_message("'dlopen' redirected to libfc, but libfc was not")
    fc_message("  compiled with 'dlfcn.h' available. Your 'dlopen'");
    fc_message("  command is dropped! Try to reinstall libfc.");
    result = NULL;
#else
    result = dlopen(filename, flag);
#endif

    /* if needed, send the result */
    if (result != NULL)
        fc_com_dlopen(result, filename, flag);

    return (result);
}

/* redirection of the dlclose */
int fc_redirect_dlclose(void *handle)
{
    int ret;

    /* call dlclose */
#ifdef FC_NO_DLFCN
    fc_message("'dlclose' redirected to libfc, but libfc was not")
    fc_message("  compiled with 'dlfcn.h' available. Your 'dlclose'");
    fc_message("  command is dropped! Try to reinstall libfc.");
    ret = 0;
#else
    ret = dlclose(handle);
    /* send the command */
    fc_com_dlclose(handle);
#endif

    return (ret);
}

/* redirection of the dlsym */
void *fc_redirect_dlsym(void *handle, char *symbol)
{
    void *result;

    /* call dlsym */
#ifdef FC_NO_DLFCN
    fc_message("'dlsym' redirected to libfc, but libfc was not")
    fc_message("  compiled with 'dlfcn.h' available. Your 'dlsym'");
    fc_message("  command is dropped! Try to reinstall libfc.");
    result = NULL;
#else
    result = dlsym(handle, symbol);
#endif

    /* if needed, send the result */
    if (result != NULL)
        fc_com_dlsym(result, handle, symbol);

    return (result);
}

/* redirection of fork */
pid_t fc_redirect_fork(void)
{
    pid_t ret;
    pid_t pid;

    pid = getpid();
    /* call fork */
    ret = fork();

    if (ret == 0)
    {/* the child */
        /* send a 'parent' message. */
        fc_com_parent(pid);
    }
    else
        if (ret > 0)
    {/* the parent */
        /* no message here because the context for the child
             would be created immediatly, and so the starting
             time for it would be a long time before the real
             start of the child (due to child creation delay)
           the information is sent by the child itself with the
           'parent' message, indicating the _real_ start time */
        /*fc_com_fork((int)ret);*/
    }

    return (ret);
}
/* redirection of pthread_create */
#ifndef FC_NO_THREAD

int fc_redirect_pthread_create(pthread_t *thread, pthread_attr_t *attr,
                               void *(*start_routine)(void *), void *arg)
{
    int ret;

    /* call pthread_create */
    ret = pthread_create(thread, attr, start_routine, arg);

    /* on success, transmit info */
    if (ret == 0)
        fc_com_thread(*thread);

    return (ret);
}
#else

int fc_redirect_pthread_create(int *thread, void *attr,
                               void *(*start_routine)(void *), void *arg)
{
    return (1);
}
#endif

/* called at each begin of profiled function */
void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
    fc_com_enter(this_fn, call_site);
}

/* called at each end of profiled function */
void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    fc_com_exit(this_fn, call_site);
}

static int dl_phdr_callback(
                            struct dl_phdr_info* info,
                            size_t size,
                            void* data)
{
    FC_LDYN ldyn;

    if (info->dlpi_addr)
    {
        ldyn.addr = (void*) info->dlpi_addr;
        ldyn.name[0] = '\0';
        strcat(ldyn.name, info->dlpi_name);
        fc_com_write_lib(&ldyn);
        fc_initial_libraries++;
    }

    return 0;
}

/* called at the real begin of the profiled program */
void __attribute__((constructor)) _init()
{
    int mode, tmode, from_rc;
    int fc_buffer_size = 128 * 1024;
    int fc_stack_size = 1024;
    int fc_graph_size = 512;
    int fc_memory_size = 512;
    int fc_function_size = 64 * 1024;
    char fc_dump_path[64] = "./";
    char fc_dump_name[64] = FC_DUMP_FILE;
    char fc_time_mode[64] = "tsc";
    int fc_verbose_mode = 1;
    int fc_debug_mode = 0;
    int fc_use_pid = 0;
    int fc_no_fork = 1;
    int fc_no_thread = 1;
    int use_memory = 0;
    int memory_stack = 4;
    FC_INIT init;
    /* special options */
    int give_help = 0;

    FC_LDYN ldyn;

    /* set the profiler name for messages */
    fc_set_message_name(FC_PROFILER_NAME);
    fc_set_message_mode(fc_verbose_mode);

    /* read ressource file */
    from_rc = fc_read_ressources();

    /* first get the env state in order to get activated options */
    if (!fc_read_env(&fc_buffer_size, &fc_stack_size,
                     &fc_function_size, &fc_graph_size,
                     &fc_memory_size, fc_dump_path,
                     fc_dump_name, fc_time_mode,
                     &fc_verbose_mode, &fc_use_pid,
                     &fc_no_fork, &fc_no_thread,
                     &fc_debug_mode, &give_help,
                     &use_memory, &memory_stack))
    {/* error ! */
        fc_message("warning: cannot read env state. starting in default mode.");
    }
    /* special */
    if (give_help)
    {
        fc_help();
        exit(FC_ERR_OK);
    }

    fc_set_message_mode(fc_verbose_mode);
    fc_set_debug_mode(fc_debug_mode);
    fc_debug("env readed");

    /* compute the mode */
    fc_debug("no_fork=%d, no_thread=%d", fc_no_fork, fc_no_thread);
    if (fc_no_fork && fc_no_thread)
        mode = FC_MODE_SINGLE;
    else
        if (!fc_no_thread)
    {
        if (!fc_allow_thread_hard)
        {
            fc_message("warning: %s was compiled without threads", FC_PACKAGE);
            fc_message("         allowed. Switching to 'fork' mode.");
            mode = FC_MODE_FORK;
        }
        else
        {
            mode = FC_MODE_THREAD;
        }
    }
    else
        mode = FC_MODE_FORK;

    fc_debug("running mode is %d", mode);

    /* init the time mode */
    tmode = fc_set_time_type(fc_time_mode);
    fc_debug("time mode is '%s' (%d)", fc_time_mode, tmode);

    /* init the time reference */
    fc_init_time();

    /* init the com process */
    fc_com_init(mode, fc_buffer_size, &fc_shared_memory_id);

    fc_local_mode = mode;

    /* prepare the init message */
    fc_check_power(&fc_function_size); /* be sure it's a power of 2 */
    init.function_size = fc_function_size;
    init.stack_size = fc_stack_size;
    init.buffer_size = fc_buffer_size;
    init.graph_size = fc_graph_size;
    init.memory_size = fc_memory_size;
    init.use_pid = fc_use_pid;
    init.mode = mode;
    init.verbose = fc_verbose_mode;
    init.debug = fc_debug_mode;
    init.time_mode = tmode;
    if (fc_no_memory_hard)
        init.memory = -1;
    else
    {
        if (use_memory)
            init.memory = memory_stack;
        else
            init.memory = -1;
    }
    fc_gettimeofday(&(init.start_time));
    sprintf(init.dump_name, "%s", fc_dump_name);
    sprintf(init.dump_path, "%s", fc_dump_path);

    init.follow = 1; /* some info about dynamic libs will come */

    /* send the init message */
    fc_debug("sending init message");
    fc_com_write_init(&init);
    fc_debug("(pid=%d)", init.first_pid);

    // load start addresses for shared libraries.
    dl_iterate_phdr(dl_phdr_callback, NULL);

    // mark end of shared library list
    ldyn.addr = 0;
    ldyn.name[0] = '\0';
    fc_com_write_lib(&ldyn);

    /* message to the user */
    fc_message("Starting %s V%s by Y.Perret", FC_PACKAGE, FC_VERSION);
    if (!from_rc)
        fc_message("Profile parameters are:");
    else
        fc_message("Profile parameters are (some from rc file):");
    fc_message("  functions table size : %d", fc_function_size);
    fc_message("      stack table size : %d", fc_stack_size);
    fc_message("            graph size : %d", fc_graph_size);
    fc_message("           buffer size : %d", fc_buffer_size);
    fc_message("           memory size : %d", fc_memory_size);
    fc_message("                   PID : %d", init.first_pid);
    fc_message("              used PID : %d", fc_use_pid);
    fc_message("               verbose : %d", init.verbose);
    fc_message("   follow dynamic libs : %d", init.follow);
    if (tmode == FC_MTIME_EXT)
        fc_message("             time mode : ext");
    else
        if (tmode == FC_MTIME_CPU)
        fc_message("             time mode : cpu");
    else
        if (tmode == FC_MTIME_TSC)
        fc_message("             time mode : tsc");
    else
        fc_message("             time mode : error!");
    if (mode == FC_MODE_SINGLE)
        fc_message("        profiling mode : no forks / no threads");
    else
        if (mode == FC_MODE_FORK)
        fc_message("        profiling mode : forks allowed / no threads");
    else
        if (mode == FC_MODE_THREAD)
        fc_message("        profiling mode : no forks / threads allowed");
    else
        fc_message("        profiling mode : error!");
    if (fc_allow_debug_hard)
        fc_message("            debug mode : %s", fc_debug_mode ? "on" : "off");
    else
        fc_message("            debug mode : unavailable");
    if (fc_no_memory_hard)
        fc_message("      profiling memory : unavailable");
    else
    {
        if (use_memory)
            fc_message("      profiling memory : on with stack size of %d", memory_stack);
        else
            fc_message("      profiling memory : off");
    }
    fc_message("        dump-file name : %s", fc_dump_name);
    fc_message("        dump-file path : %s", fc_dump_path);
    fc_message("");

    /* if needed, init the memory profiling */
    if (use_memory)
        fc_memory_init();

    fc_message("Starting fcmanager...");
    fc_com_start_manager(fc_shared_memory_id);

    fc_debug("leaving _init");
}

/* called after the end of the profiled program */
void __attribute__((destructor)) _fini()
{
    fc_debug("entering _fini");

    /* send the QUIT message, to inform the manager that this process is over */
    fc_com_quit();

    /* stop the memory profiling */
    fc_memory_fini();

    /* sleep a little to let childs (if any) starting.
       else, the manager may detect the end of the pipe
       even if a child will start soon
     */
    if (fc_local_mode != FC_MODE_SINGLE)
        sleep(2);

    /* stop the com process */
    fc_com_fini(fc_shared_memory_id);

    fc_debug("leaving _fini");
}
