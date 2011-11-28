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
#include "fc_global.h"
#include "fc_com_manager.h"
#include "fc_time.h"
#include "fc_tools.h"
#include "fc_fifo.h"


/* the profile mode for other modules */
int fc_mcom_mode = FC_MODE_SINGLE;

/* FIFO for the com */
static FC_FIFO fc_com_fifo = FC_FIFO_NDEF;
static int fc_com_fifo_id = 0;

/* the initial PID (for default value) */
int fc_default_pid = 0;

static int fc_com_pid = 0;

/* real enter/exit for type 1 */
int fc_mcom_read(void **f, void **s, unsigned long long *time, int *id, char *type,
		 void **ptr, void **incoming, void **where, int *parent,
		 unsigned int *size, unsigned int *align, char *name)
{
    unsigned char* buffer;

    union
    {
        FC_CEnter;
        FC_CExit;
        FC_CMalloc;
        FC_CFree;
        FC_CRealloc;
        FC_CMemalign;
        FC_CDlsym;
        FC_CDlopen;
        FC_CDlclose;
        FC_CFork;
        FC_CThread;
        FC_CQuit;
        FC_CParent;
        FC_CTime;
    } data;

    buffer = fc_fifo_read_single(fc_com_fifo, sizeof(char) /*type*/ + sizeof(int) /*pid,tid*/ + sizeof(data), fc_com_pid);
    if (buffer == 0)
    {
        return 0;
    }

    *type = buffer[0];
    buffer++;

    /* if needed read the ID */
    if (fc_mcom_mode == FC_MODE_SINGLE)
    {
        *id = fc_default_pid;
    }
    else
    {
        *id = *(unsigned int*)buffer;
        buffer += sizeof(unsigned int);
        if (*id == 0)
        {
            fc_message("invalid id (0)");
            goto CLEANUP;
        }
    }

    /* now read the data regards to the type */
    switch (*type)
    {
    case FC_TYPE_ENTER:
    {
        FC_CEnter* fcenter = (FC_CEnter*)buffer;
        *f = fcenter->to;
        *s = fcenter->from;
        *time = fcenter->time;
        break;
    }
    case FC_TYPE_EXIT:
    {
        FC_CExit* fcexit = (FC_CExit*)buffer;
        *f = fcexit->to;
        *s = fcexit->from;
        *time = fcexit->time;
        break;
    }
    case FC_TYPE_MALLOC:
    {
        FC_CMalloc* fcmalloc = (FC_CMalloc*)buffer;
        *where = fcmalloc->where;
        *size = fcmalloc->size;
        *ptr = fcmalloc->ptr;
        break;
    }
    case FC_TYPE_FREE:
    {
        FC_CFree* fcfree = (FC_CFree*)buffer;
        *ptr = fcfree->ptr;
        *where = fcfree->where;
        break;
    }
    case FC_TYPE_REALLOC:
    {
        FC_CRealloc* fcrealloc = (FC_CRealloc*)buffer;
        *where = fcrealloc->where;
        *size = fcrealloc->size;
        *ptr = fcrealloc->ptr;
        *incoming = fcrealloc->old;
        break;
    }
    case FC_TYPE_MEMALIGN:
    {
        FC_CMemalign* fcmemalign = (FC_CMemalign*)buffer;
        *where = fcmemalign->where;
        *size = fcmemalign->size;
        *align = fcmemalign->align;
        *ptr = fcmemalign->ptr;
        break;
    }
    case FC_TYPE_DLSYM:
    {
        FC_CDlsym* fcdlsym = (FC_CDlsym*)buffer;
        *ptr = fcdlsym->fnc;
        *incoming = fcdlsym->handle;
        sprintf(name, "%s", fcdlsym->name); /* name must be large enough */
        break;
    }
    case FC_TYPE_DLOPEN:
    {
        FC_CDlopen* fcdlopen = (FC_CDlopen*)buffer;
        *ptr = fcdlopen->handle;
        sprintf(name, "%s", fcdlopen->name); /* name must be large enough */
        /* fcdlopen.flag  -> not used */
        break;
    }
    case FC_TYPE_DLCLOSE:
    {
        FC_CDlclose* fcdlclose = (FC_CDlclose*)buffer;
        *incoming = fcdlclose->handle;
        break;
    }
    case FC_TYPE_FORK:
    {
        FC_CFork* fcfork = (FC_CFork*)buffer;
        *parent = fcfork->child;
        *time = fcfork->time;
        break;
    }
    case FC_TYPE_THREAD:
    {
        FC_CThread* fcthread = (FC_CThread*)buffer;
        *parent = fcthread->thread;
        *time = fcthread->time;
        break;
    }
    case FC_TYPE_QUIT:
    {
        FC_CQuit* fcquit = (FC_CQuit*)buffer;
        *time = fcquit->time;
        break;
    }
    case FC_TYPE_PARENT:
    {
        FC_CParent* fcparent = (FC_CParent*)buffer;
        *parent = fcparent->parent;
        break;
    }
    case FC_TYPE_TIME:
    {
        FC_CTime* fctime = (FC_CTime*)buffer;
        *time = fctime->time;
        break;
    }
    default:
    {
        fc_message("invalid message, type: 0x%02x. Very bad...", *type);
        break;
    }
    }

CLEANUP:

    fc_fifo_read_single_done(fc_com_fifo, fc_com_pid);
    
    return 1;
}


/* init the communication process and start the manager */
int fc_mcom_init(int shmid, int *id, FC_INIT *init)
{
    fc_com_pid = getpid();
    /* connect to the FIFO */
    fc_com_fifo = fc_fifo_connect(shmid);
    fc_com_fifo_id = shmid;
    if (fc_com_fifo == FC_FIFO_NDEF)
    {
        fc_message("cannot connect to the FIFO.");
        return (0);
    }
    fc_debug("shared buffer '%d' mapped at address %p", shmid, fc_com_fifo);

    /* read the init message */
    if ( fc_fifo_read_init(fc_com_fifo, init, sizeof(FC_INIT), fc_com_pid) == 0 )
    {
        fc_message("end of FIFO while reading initialization message!");
        return (0);
    }

    /* set the pointers */
    if (init->mode == FC_MODE_SINGLE)
    {
        fc_mcom_mode = FC_MODE_SINGLE;
    }
    else
        if (init->mode == FC_MODE_FORK)
    {
        fc_mcom_mode = FC_MODE_FORK;
    }
    else
        if (init->mode == FC_MODE_THREAD)
    {
        fc_mcom_mode = FC_MODE_THREAD;
    }
    else
    {
        fc_message("invalid mode for communication initialisation (%d).\n");
    }

    /* store the default PID */
    fc_default_pid = init->first_pid;
    *id = init->first_pid;

    /* ok */
    return (1);
}

/* read a dynamic lib entry */
int fc_mcom_read_lib(FC_LDYN *ldyn)
{
    if ( fc_fifo_read_ldyn(fc_com_fifo, ldyn, sizeof (FC_LDYN), fc_com_pid) == 0 )
    {
        fc_message("end of FIFO while reading dynamic l ibrary list!");
        return 0;
    }

    return 1;
}

/* stop the communication process */
int fc_mcom_fini(unsigned int id)
{
  /* close the com */
  fc_fifo_close(fc_com_fifo, id, 1 /* delete shm if reference count is 0, no process or manager needs it */);

  return(1);
}



