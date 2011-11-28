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
/** fc_com.h: manage coms between profiled program and the manager
              (manager side)  **/

#ifndef __fc_com_manager_h_
#define __fc_com_manager_h_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "fc_com.h"


/** the profile mode for other modules **/
extern int fc_mcom_mode;


/** functions **/

/* init the communication stuff and read the init message */
int fc_mcom_init(int shmid, int *id, FC_INIT *init);

/* read a dynamic lib entry */
int fc_mcom_read_lib(FC_LDYN *ldyn);

/* function pointer of the 'read msg' treatment */
extern int fc_mcom_read(void **function, void **from,
                        unsigned long long *time, int *id, char *type,
			void **ptr, void **incoming, void **where, int *parent,
			unsigned int *size, unsigned int *align,
			char *name);

/* stop the communication stuff */
int fc_mcom_fini(unsigned int id);


#endif /* __fc_com_manager_h_ */
