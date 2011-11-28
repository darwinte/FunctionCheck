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
/** fc_names.h:  **/

#ifndef __fc_names_h_
#define __fc_names_h_

#include <stdio.h>
#include <stdlib.h>
#include <bfd.h>

#include "fc_tools.h"
#include "fc_global.h"
#include "fc_dump.h"


int fc_names_solve(int force_names,
                   int nb_fnc, FC_Function *fncs,
                   int nb_lib, FC_LDyn *dyns,
		   int nb_arcs, FC_Arc *arcs,
                   char *exec_name, int demangle, int style,
                   FC_NSym *lonly, int nb_only,
                   FC_NSym *lnot, int nb_not,
                   int nb_leaks, FC_MLeak *leaks,
                   int nb_free, FC_MFree *free,
                   int nb_realloc, FC_MRealloc *realloc
                   );


#endif /* __fc_names_h_ */
