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
/** fc_graph.h:  **/

#ifndef __fc_graph_h_
#define __fc_graph_h_

#include <stdio.h>
#include <stdlib.h>

#include "fc_dump.h"


/* structure of a node (for the call-graph) */
typedef struct _FC_Node FC_Node;
struct _FC_Node
{
  FC_Function *function;/* corresponding function */
  FC_Node **nexts;      /* list of childs (ended by NULL) */
  FC_Node **prevs;      /* list of callers (ended by NULL) */
  int *nnexts;          /* number values */
  int *nprevs;          /* number values */
  int keep;             /* bool: if true, the function is keeped */
  int treated;          /* bool+: used to propagate "keep" */
  int tag;              /* tag for cycle detection */
};



/** functions **/

/* perform all treatments to generate the call-graph */
int fc_graph_create(int *_nb_arcs, FC_Arc *arcs,
                    int nb_fncs, FC_Function *fncs,
                    FC_NSym *only, int nb_only,
                    FC_NSym *not,  int nb_not,
                    int propagate, int rpropagate);

/* remove all nodes created */
int fc_graph_delete(int nb_arcs, FC_Arc *arcs,
                    int nb_fncs, FC_Function *fncs);

/* create a new node for a given function */
FC_Node *fc_create_node(FC_Function *function);

/* delete a node */
FC_Node *fc_delete_node(FC_Node *node);

/* add a child to a node */
int fc_add_child(FC_Node *node, FC_Node *child, int nb);

/* add a parent to a node */
int fc_add_prev(FC_Node *node, FC_Node *prev, int nb);

/* propagate given value (0/1) from the given node */ 
void fc_propagate_to_child(FC_Node*node, int val);

/* retro-propagate given value (0/1) from the given node */ 
void fc_propagate_to_caller(FC_Node*node, int val);

/* propagate given value (0/1) from the given node */ 
void fc_propagate_to_child_p(FC_Node*node);

/* retro-propagate given value (0/1) from the given node */ 
void fc_propagate_to_caller_p(FC_Node*node);

/* propagate tags to nodes */
void fc_tag_nodes(FC_Node *node, int tag);
void fc_display_recurs(int options, FC_Node *node, unsigned int nc);
void fc_display_cycle(int options, FC_Node *end, FC_Node *start, unsigned int nc);

/* compute list of cycles */
int fc_compute_cycles(int options);



#endif /* __fc_graph_h_ */
