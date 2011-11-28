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
/** fc_graph.h: manage the call-graph (arcs) during profile **/

#ifndef __fc_graph_h_
#define __fc_graph_h_

#include <stdio.h>
#include <stdlib.h>
#include "fc_xlhash.h"



/** functions **/
/* init the graph */
FC_LHash *fc_graph_init(int size);

/* add a new arc */
int fc_graph_add(FC_LHash *graph, void *from, void *to);

/* add a new arc (if not present) */
int fc_graph_add_single(FC_LHash *graph, void *from, void *to);

/* remove the graph */
int fc_graph_free(FC_LHash *graph);


#endif /* __fc_graph_h_ */
