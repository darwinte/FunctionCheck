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

#ifndef __fc_com_h_
#define __fc_com_h_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


/** flag to indicate if the lib is compiled with threads **/
extern int fc_allow_thread_hard;


/** basic structures **/

/* enter in a function */
typedef struct {
    void *from, *to; /* call-site, fnc */
    unsigned long long time;
} FC_CEnter;

/* exit from a function */
typedef struct {
    void *from, *to; /* call-site, fnc */
    unsigned long long time;
} FC_CExit;

/* malloc action */
typedef struct {
    void *ptr, *where; /* returned pointer and action place */
    unsigned int size; /* size of the block */
} FC_CMalloc;

/* free action */
typedef struct {
    void *ptr, *where; /* pointer and action place */
} FC_CFree;

/* memalign action */
typedef struct {
    void *ptr, *where; /* returned pointer and action place */
    unsigned int size, align; /* size of the block and requested alignment */
} FC_CMemalign;

/* realloc action */
typedef struct {
    void *ptr, *old, *where; /* returned pointer, initial pointer and action place */
    unsigned int size; /* new size */
} FC_CRealloc;

/* quit action */
typedef struct {
    unsigned long long time; /* time of the quit action */
} FC_CQuit;

/* dlopen action */
typedef struct {
    void *handle; /* returned handle */
    int flag; /* given flag */
    char name[32]; /* object name */
} FC_CDlopen;

/* dlclose action */
typedef struct {
    void *handle; /* handle to close */
} FC_CDlclose;

/* dlsym action */
typedef struct {
    void *handle, *fnc; /* handle and function pointer returned */
    char name[32]; /* function name */
} FC_CDlsym;

/* parent information */
typedef struct {
    int parent; /* parent of the concerned process */
} FC_CParent;

/* fork information */
typedef struct {
    int child; /* the created child */
    unsigned long long time;
} FC_CFork;

/* thread creation */
typedef struct {
    int thread; /* the thread_id of the create thread */
    unsigned long long time;
} FC_CThread;

/* time event */
typedef struct {
    unsigned int time;
} FC_CTime;


/** definition of types of messages **/
#define FC_TYPE_NDEF		255
#define FC_TYPE_FIRST		0
#define FC_TYPE_ENTER		1
#define FC_TYPE_EXIT		2
#define FC_TYPE_MALLOC		3
#define FC_TYPE_FREE		4
#define FC_TYPE_MEMALIGN	5
#define FC_TYPE_REALLOC		6
#define FC_TYPE_QUIT		7
#define FC_TYPE_DLOPEN		8
#define FC_TYPE_DLCLOSE		9
#define FC_TYPE_DLSYM		10
#define FC_TYPE_PARENT		11
#define FC_TYPE_FORK		12
#define FC_TYPE_THREAD		13
#define FC_TYPE_TIME		14
#define FC_TYPE_MAX		(FC_TYPE_TIME+1)


/** sizes of each type **/
extern unsigned int fc_type_sizes[];

/** structure for initialisation **/
typedef struct {
    int function_size;
    int stack_size;
    int graph_size;
    int buffer_size;
    int memory_size;
    int use_pid;
    int mode;
    unsigned long long start_time;
    int first_pid;
    int verbose;
    int debug;
    int memory;
    int time_mode;
    char dump_name[128];
    char dump_path[128];
    int follow; /* true if a list of dynamic lib follows */
    /* not finished */
} FC_INIT;

/** structure for dynamic libraries **/
typedef struct {
    void *addr;
    char name[128];
} FC_LDYN;


/** functions **/

/* init the communication process */
int fc_com_init(int mode, int buffer_size, unsigned int *shmid);

/* start fcmanager */
int fc_com_start_manager(unsigned int shmid);

/* write init message */
int fc_com_write_init(FC_INIT *init);

/* write init message */
int fc_com_write_lib(FC_LDYN *ldyn);

/* functions */
void fc_com_enter(void *f, void *s);
void fc_com_exit(void *f, void *s);
void fc_com_malloc(void *ptr, unsigned int size, void *where);
void fc_com_free(void *ptr, void *where);
void fc_com_realloc(void *ptr, void *inc, unsigned int size, void *where);
void fc_com_memalign(void *ptr, unsigned int align, unsigned int size, void *where);
void fc_com_dlopen(void *ptr, const char *filename, int flag);
void fc_com_dlclose(void *handle);
void fc_com_dlsym(void *ptr, void *handle, char *symbol);
void fc_com_fork(int pid);
void fc_com_thread(int tid);
void fc_com_parent(int pid);
void fc_com_quit(void);


/* stop the communication process */
int fc_com_fini(unsigned int shmid);


#endif /* __fc_com_h_ */
