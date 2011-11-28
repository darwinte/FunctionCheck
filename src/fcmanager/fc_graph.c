/*
 * FunctionCheck profiler
 * (C) Copyright 2000-2002 Yannick Perret 
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the FC_NU FC_eneral Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the FC_NU
 *  FC_eneral Public License for more details.
 *
 *  You should have received a copy of the FC_NU FC_eneral Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/** fc_graph.c:  **/

#include <stdio.h>
#include <stdlib.h>
#include "fc_xlhash.h"
#include "fc_global.h"
#include "fc_graph.h"
#include "fc_tools.h"

/* init the arc list */
FC_LHash *fc_graph_init(int size)
{
    /* we use 'from' part of arcs as key, because only one
         call can start from a given address */
    return (fc_lhash_new());
}

/* add a new arc */
int fc_graph_add(FC_LHash *graph, void *from, void *to)
{
    int *val;
    void **ptr1, **ptr2;
    unsigned long long key;

    if (graph == NULL)
        return (0);

    /* empty entry */
    if (from == NULL)
        return (1);

    /* compute the key */
    key = (((unsigned long long) ((unsigned int) from)) << (sizeof (int) *8)) +
            ((unsigned long long) ((unsigned int) to));

    /* look for the element */
    if (fc_lhash_lookup_modify(graph, key, &val, &ptr1, &ptr2))
    {
        /* still referenced: add the arc */
        (*val)++;
        return (1);
    }

    /* else add the key */
    fc_lhash_insert(graph, key, 1, NULL, NULL);

    return (1);
}

/* add a new arc (if not present) */
int fc_graph_add_single(FC_LHash *graph, void *from, void *to)
{
    /* redirect */
    return (fc_graph_add(graph, from, to));
}

/* remove the graph */
int fc_graph_free(FC_LHash *graph)
{
    fc_lhash_destroy(graph);

    return (1);
}


