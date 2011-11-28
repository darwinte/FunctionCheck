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
/** fc_memory.h:  **/

#ifndef __fc_memory_h_
#define __fc_memory_h_

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_MALLOC_H
#ifdef HAVE_VALID_HOOKS
#ifndef	_MALLOC_INTERNAL
#define	_MALLOC_INTERNAL
#endif
#include <malloc.h>
#endif
#endif



/** true if memory is not available **/
extern int fc_no_memory_hard;

/* indicate the call-stack requested for memory actions */
extern int fc_memory_stack_size;

/** functions **/

/* init the memory hooks for memory profiling */
int fc_memory_init(void);

/* stop the memory hooks */
int fc_memory_fini(void);



#endif /* __fc_memory_h_ */
