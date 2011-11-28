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

/* fc_time.h: functions to compute elapsed time */

#ifndef __fc_time_h_
#define __fc_time_h_

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/times.h>
#include <unistd.h>
#include <string.h>

#include "fc_global.h"





/* init the time system */
void fc_init_time();

/* set the time mode */
int fc_set_time_type(char *type);

/* set the time mode */
int fc_get_time_type();

/* my 'gettimeofday'. returns a 'timeval' structure containing
   the current time (in the good clock-mode) */
void fc_gettimeofday(unsigned long long *val);

#endif /* __fc_time_h_ */
