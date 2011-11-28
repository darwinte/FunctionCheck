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

/** fc_com.h: manage coms between profiled program and the manager **/

#include <errno.h>
#include <stdio.h>
#include "fc_com.h"
#include "fc_time.h"
#include "fc_global.h"
#include "fc_tools.h"
#include "fc_fifo.h"
#include "fc_memory_manager.h"

#ifndef FC_NO_THREAD
#include <pthread.h>
#endif /* FC_NOTHREAD */

/** flag to indicate if the lib is compiled with threads **/
#ifndef FC_NO_THREAD
int fc_allow_thread_hard=1;
#else
int fc_allow_thread_hard=0;
#endif

/** sizes for each type **/
unsigned int fc_type_sizes[FC_TYPE_MAX] =
{
    sizeof (FC_CEnter),
    sizeof (FC_CExit),
    sizeof (FC_CMalloc),
    sizeof (FC_CFree),
    sizeof (FC_CMemalign),
    sizeof (FC_CRealloc),
    sizeof (FC_CQuit),
    sizeof (FC_CDlopen),
    sizeof (FC_CDlclose),
    sizeof (FC_CDlsym),
    sizeof (FC_CParent),
    sizeof (FC_CFork),
    sizeof (FC_CThread),
    sizeof (FC_CTime)
};

/* maximum element size of fc_type_sizes[] */
static unsigned int fc_max_tsize = 0;

/* FIFO for the com */
static FC_FIFO fc_com_fifo = FC_FIFO_NDEF;

/* mode */
int fc_used_mode=0;

/* current ID for SINGLE mode */
static int fc_single_id = 0;


/** internal data for messages **/
static unsigned char fc_buffer_total[512];
/* buffer for messages */
static unsigned char *fc_buffer_write = NULL;
/* offset to write the pid */
static unsigned char *fc_buffer_opid  = NULL;
/* offset to write the tid */
static unsigned char *fc_buffer_otid  = NULL;
/* offset to write the data */
static unsigned char *fc_buffer_odata = NULL;

/* impossible to code as inline due to containing #ifdef.
   Easier to code too as a function instead of inline. */
static inline void build_fc_com(void** com, unsigned int *curid, unsigned int type)
{
    unsigned char *buffer;
    
    if (fc_buffer_opid != NULL)
    {
        *curid = getpid();
    }
    else
    {
        /* else if needed the tid */
        if (fc_buffer_otid != NULL)
        {
#ifdef FC_NO_THREAD
            *curid = getpid();
#else
            *curid = (unsigned int) pthread_self();
#endif
        }
    }

    buffer = fc_fifo_write_single(fc_com_fifo, /* sizeof(int) for id + fc_max_tsize + sizeof(char) for type */ fc_max_tsize, *curid);
    if (buffer == 0)
    {
        fc_message("failed to write to fifo buffer, full already.");
        *com = 0;
        return;
    }

    buffer[0] = (char) type;
    buffer++;

    if (fc_buffer_opid != NULL)
    {
        *((unsigned int*) buffer) = *curid;
        buffer += sizeof (int);
    }
    else
    {
        if (fc_buffer_otid != NULL)
        {
            *((unsigned int*) buffer) = *curid;
            buffer += sizeof (int);
        }
    }

    *com = (void*)buffer;
    
    return;
}

#define     DECLARE_FC_COM(struct_)    \
                                                unsigned int curid = fc_single_id; \
                                                struct_* com;

#define     BUILD_FC_COM(type)      build_fc_com((void**)&com, &curid, type); \
                                    if (com == 0) \
                                        return;

#define     SEND_FC_COM()       fc_fifo_write_single_done(fc_com_fifo, curid);

/* functions */
void fc_com_enter(void *f, void *s)
{
    DECLARE_FC_COM(FC_CEnter);

    BUILD_FC_COM(FC_TYPE_ENTER);

    com->to = f;
    com->from = s;
    fc_gettimeofday(&(com->time));

    SEND_FC_COM();
}

void fc_com_exit(void *f, void *s)
{
    DECLARE_FC_COM(FC_CExit);

    BUILD_FC_COM(FC_TYPE_EXIT);

    com->to = f;
    com->from = s;
    fc_gettimeofday(&(com->time));

    SEND_FC_COM();
}

void fc_com_malloc(void *ptr, unsigned int size, void *where)
{
    DECLARE_FC_COM(FC_CMalloc);

    BUILD_FC_COM(FC_TYPE_MALLOC);

    com->ptr = ptr;
    com->where = where;
    com->size = size;

    SEND_FC_COM();
}

void fc_com_free(void *ptr, void *where)
{
    DECLARE_FC_COM(FC_CFree);

    BUILD_FC_COM(FC_TYPE_FREE);

    com->ptr = ptr;
    com->where = where;

    SEND_FC_COM();
}

void fc_com_realloc(void *ptr, void *inc, unsigned int size, void *where)
{
    DECLARE_FC_COM(FC_CRealloc);

    BUILD_FC_COM(FC_TYPE_REALLOC);

    com->ptr = ptr;
    com->old = inc;
    com->size = size;
    com->where = where;

    SEND_FC_COM();
}

void fc_com_memalign(void *ptr, unsigned int align, unsigned int size, void *where)
{
    DECLARE_FC_COM(FC_CMemalign);

    BUILD_FC_COM(FC_TYPE_MEMALIGN);

    com->ptr = ptr;
    com->align = align;
    com->size = size;
    com->where = where;

    SEND_FC_COM();
}

void fc_com_dlopen(void *ptr, const char *filename, int flag)
{
    DECLARE_FC_COM(FC_CDlopen);

    BUILD_FC_COM(FC_TYPE_DLOPEN);

    com->handle = ptr;
    com->flag = flag;
    sprintf(com->name, "%31s", filename);

    SEND_FC_COM();
}

void fc_com_dlclose(void *handle)
{
    DECLARE_FC_COM(FC_CDlclose);

    BUILD_FC_COM(FC_TYPE_DLCLOSE);

    com->handle = handle;

    SEND_FC_COM();
}

void fc_com_dlsym(void *ptr, void *handle, char *symbol)
{
    DECLARE_FC_COM(FC_CDlsym);

    BUILD_FC_COM(FC_TYPE_DLSYM);

    com->handle = handle;
    com->fnc = ptr;
    sprintf(com->name, "%31s", symbol);

    SEND_FC_COM();
}

void fc_com_fork(int pid)
{
    DECLARE_FC_COM(FC_CFork);

    BUILD_FC_COM(FC_TYPE_FORK);

    com->child = pid;
    fc_gettimeofday(&(com->time));

    SEND_FC_COM();
}

void fc_com_thread(int tid)
{
    DECLARE_FC_COM(FC_CThread);

    BUILD_FC_COM(FC_TYPE_THREAD);

    com->thread = tid;
    fc_gettimeofday(&(com->time));

    SEND_FC_COM();
}

void fc_com_parent(int pid)
{
    DECLARE_FC_COM(FC_CParent);

    BUILD_FC_COM(FC_TYPE_PARENT);

    com->parent = pid;

    SEND_FC_COM();
}

void fc_com_quit(void)
{
    DECLARE_FC_COM(FC_CQuit);

    BUILD_FC_COM(FC_TYPE_QUIT);

    fc_gettimeofday(&(com->time));

    SEND_FC_COM();
}

/* init the communication process and start the manager */
int fc_com_init(int mode, int buffer_size, unsigned int *shmid)
{
    int i;

    fc_debug("entering fc_com_init");

    /* set the pointers */
    fc_used_mode = mode;
    if (mode == FC_MODE_SINGLE)
    {
        fc_buffer_write = fc_buffer_total;
        fc_buffer_opid = NULL;
        fc_buffer_otid = NULL;
        fc_buffer_odata = &(fc_buffer_total[1]);
    }
    else
        if (mode == FC_MODE_FORK)
    {
        fc_buffer_write = fc_buffer_total;
        fc_buffer_opid = &(fc_buffer_total[1]);
        fc_buffer_otid = NULL;
        fc_buffer_odata = &(fc_buffer_total[1 + sizeof (int) ]);
    }
    else
        if (mode == FC_MODE_THREAD)
    {
        fc_buffer_write = fc_buffer_total;
        fc_buffer_opid = NULL;
        fc_buffer_otid = &(fc_buffer_total[1]);
        fc_buffer_odata = &(fc_buffer_total[1 + sizeof (int) ]);
    }
    else
    {
        fc_message("invalid mode for communication initialisation (%d).");
        return (0);
    }

    /* compute the largest element size */
    for (i = 0; i <= FC_TYPE_MAX; i++)
    {
        fc_max_tsize = (fc_type_sizes[i] > fc_max_tsize) ? fc_type_sizes[i] : fc_max_tsize;
    }

    fc_max_tsize += (sizeof(char) /*type*/ + sizeof(int) /*pid/tid*/);
    
    /* create the FIFO */
    fc_com_fifo = fc_fifo_create(buffer_size, fc_max_tsize, shmid, mode == FC_MODE_SINGLE ? 1 : 0);
    if (fc_com_fifo == FC_FIFO_NDEF)
    {/* error */
        fc_message("error while opening fifo");
        return (0);
    }
    fc_debug("FIFO '%d' mapped at %p", shmid, (void*) fc_com_fifo);

    /* single-mode PID */
    fc_single_id = (int) getpid();

    /* ok */
    return (1);
}

int fc_com_start_manager(unsigned int shmid)
{
    int i, ret;
    char *args[64];
    char temp[1024];
    
    /* args for the manager */
    for (i = 0; i < 64; i++)
    {
        args[i] = NULL;
    }
    args[0] = strdup("fcmanager"); /* to change */
    sprintf(temp, "%d", shmid);
    args[1] = strdup(temp);
    fc_debug("args [%s] [%s]", args[0], args[1]);

    /* fork */
    fc_debug("starting manager");
    if ((ret = fork()) == 0)
    {/* the child */
        /* start the manager */
        fc_debug("CHILD SIDE: -> execvp");
        execvp(args[0], args);

        fc_message("error: unable to start profile manager.");
        fc_message_fatal(FC_ERR_EXEC, "unable to continute treatments.");
    }

    fc_debug("fork -> %d", ret);
    if (ret < 0)
    {
        fc_message("error: unable to fork.");
        fc_message_fatal(FC_ERR_FORK, "unable to continute treatments.");
    }

    return ret;
}

/* stop the communication process */
int fc_com_fini(unsigned int shmid)
{
  /* close the com */
  fc_fifo_close(fc_com_fifo, shmid, 0 /* don't delete shm, fcmanager still processing */);

  /* nothing to do */
  return(1);
}

/* send an init message */
int fc_com_write_init(FC_INIT *init)
{
    fc_debug("writing FC_INIT structure");
    if (fc_used_mode == FC_MODE_SINGLE)
    {
        init->first_pid = getpid();
    }
    else
        if (fc_used_mode == FC_MODE_FORK)
    {
        init->first_pid = getpid();
    }
    else
        if (fc_used_mode == FC_MODE_THREAD)
    {
#ifndef FC_NO_THREAD
        init->first_pid = pthread_self();
#else
        init->first_pid = getpid();
#endif
    }
    else
    {
        fc_message("invalid mode for communication initialisation (%d).");
        return 0;
    }

    fc_fifo_write_init(fc_com_fifo, init, sizeof(FC_INIT), fc_single_id);

    return 1;
}



/* send an dynamic-lib message */
int fc_com_write_lib(FC_LDYN *ldyn)
{
    fc_fifo_write_ldyn(fc_com_fifo, ldyn, sizeof (FC_LDYN), fc_single_id);

    return 1;
}
