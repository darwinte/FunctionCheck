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
/** fc_graph.c:  **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fc_graph.h"


FC_Node **fc_list_of_nodes=NULL;
int fc_nb_list_of_nodes=0;


/* register a pointer to a node in the local list */
void fc_register_node(FC_Node *node)
{
  if (fc_list_of_nodes == NULL)
    {
    fc_list_of_nodes = malloc(sizeof(FC_Node*));
    }
  else
    {
    fc_list_of_nodes = realloc(fc_list_of_nodes, sizeof(FC_Node*)*
                                  (fc_nb_list_of_nodes+1));
    }
  if (fc_list_of_nodes == NULL)
    {
    fc_message("cannot allocate %d bytes for local node list.", sizeof(FC_Node*)*
                                  (fc_nb_list_of_nodes+1));
    fc_message("cycles detection will not be usable.");
    fc_nb_list_of_nodes = 0;
    return;
    }
  fc_list_of_nodes[fc_nb_list_of_nodes] = node;
  fc_nb_list_of_nodes++;
}



/* create a new node for a given function */
FC_Node *fc_create_node(FC_Function *function)
{
  FC_Node *tmp;
  
  if (fc_malloc(tmp, 1) == NULL)
    {
    fc_message("not enough memory");
    return(NULL);
    }
  if (fc_malloc(tmp->nexts, 1) == NULL)
    {
    fc_message("not enough memory");
    free(tmp);
    return(NULL);
    }
  if (fc_malloc(tmp->prevs, 1) == NULL)
    {
    fc_message("not enough memory");
    free(tmp->nexts);
    free(tmp);
    return(NULL);
    }
  if (fc_malloc(tmp->nnexts, 1) == NULL)
    {
    fc_message("not enough memory");
    free(tmp->nexts);
    free(tmp->prevs);
    free(tmp);
    return(NULL);
    }
  if (fc_malloc(tmp->nprevs, 1) == NULL)
    {
    fc_message("not enough memory");
    free(tmp->nexts);
    free(tmp->prevs);
    free(tmp->nnexts);
    free(tmp);
    return(NULL);
    }
  tmp->nexts[0]  = NULL;
  tmp->prevs[0]  = NULL;
  tmp->nnexts[0] = 0;
  tmp->nprevs[0] = 0;
  tmp->function = function;
  tmp->keep     = 1;
  tmp->treated  = 0;
  tmp->tag      = 0;

  return(tmp);
}

/* delete a node */
FC_Node *fc_delete_node(FC_Node *node)
{
  if (node == NULL)
    return(NULL);

  if (node->nexts != NULL)
    free(node->nexts);
  if (node->prevs != NULL)
    free(node->prevs);
  if (node->nnexts != NULL)
    free(node->nnexts);
  if (node->nprevs != NULL)
    free(node->nprevs);
  free(node);

  return(NULL);
}


/* add a child to a node */
int fc_add_child(FC_Node *node, FC_Node *child, int number)
{
  int nb=0;

  while(node->nexts[nb] != NULL)
    {
    nb++;
    }
  if ((node->nexts = realloc(node->nexts, (nb+2)*sizeof(FC_Node*))) == NULL)
    {
    fc_message("not enough memory.");
    return(0);
    }
  if ((node->nnexts = realloc(node->nnexts, (nb+2)*sizeof(int))) == NULL)
    {
    fc_message("not enough memory.");
    return(0);
    }
  node->nnexts[nb]  = number;
  node->nexts[nb]   = child;
  node->nexts[nb+1] = NULL;

  return(1);
}

/* add a parent to a node */
int fc_add_prev(FC_Node *node, FC_Node *prev, int number)
{
  int nb=0;

  while(node->prevs[nb] != NULL)
    nb++;
  if ((node->prevs = realloc(node->prevs, (nb+2)*sizeof(FC_Node*))) == NULL)
    {
    fc_message("not enough memory.");
    return(0);
    }
  if ((node->nprevs = realloc(node->nprevs, (nb+2)*sizeof(int))) == NULL)
    {
    fc_message("not enough memory.");
    return(0);
    }
  node->nprevs[nb]  = number;
  node->prevs[nb]   = prev;
  node->prevs[nb+1] = NULL;
  
  return(1);
}



/* propagate given value (0/1) from the given node */ 
void fc_propagate_to_child(FC_Node*node, int val)
{
  int i;

  if (node->keep == val)
    return; /* still done */

  node->keep    = val;
  node->treated = 1;
  i = 0;
  while(node->nexts[i] != NULL)
    {
    fc_propagate_to_child(node->nexts[i], val);
    i++;
    }
}
/* retro-propagate given value (0/1) from the given node */ 
void fc_propagate_to_caller(FC_Node*node, int val)
{
  int i;

  if (node->keep == val)
    return; /* still done */

  node->keep = val;
  node->treated = 1;
  i = 0;
  while(node->prevs[i] != NULL)
    {
    fc_propagate_to_caller(node->prevs[i], val);
    i++;
    }
}

/* propagate given value (0/1) from the given node */ 
void fc_propagate_to_child_p(FC_Node*node)
{
  int i;

  if (node->treated == 1)
    return; /* still done */

  node->keep    = 1;
  node->treated = 1;
  i = 0;
  while(node->nexts[i] != NULL)
    {
    fc_propagate_to_child_p(node->nexts[i]);
    i++;
    }
}
/* retro-propagate given value (0/1) from the given node */ 
void fc_propagate_to_caller_p(FC_Node*node)
{
  int i;

  if (node->treated == 1)
    return; /* still done */

  node->keep    = 1;
  node->treated = 1;
  i = 0;
  while(node->prevs[i] != NULL)
    {
    fc_propagate_to_caller_p(node->prevs[i]);
    i++;
    }
}



/* propagate tags to nodes */
void fc_tag_nodes(FC_Node *node, int tag)
{
  int i;

  /* stop if node still tagged */
  if (node->tag != 0)
    return;
  /* tag the node */
  node->tag = tag;
  /* tag each child */
  i = 0;
  while(node->nexts[i] != NULL)
    {
    fc_tag_nodes(node->nexts[i], tag+1);
    i++;
    }
}


/* for debug */
void fc_display_recurs(int option, FC_Node *node, unsigned int nc)
{
  printf("%d: Simple recursion:\n", nc);
  printf("  '%s' calls itself.\n", node->function->name.name);
  printf("  Total time for this function: %f\n", node->function->total_time/1000000.);
  printf("\n");
}
void fc_display_cycle(int option, FC_Node *end, FC_Node *start, unsigned int nc)
{
  printf("%d: Cycle:\n", nc);
  printf("  cycle from '%s' to '%s'\n", start->function->name.name,
                                        end->function->name.name);
  printf("  Total time for this cycle: %f\n", start->function->total_time/1000000.);
  printf("\n");
}


/* check if a cycle is detected */
int fc_get_cycle_(FC_Node *node, FC_Node *init, FC_Node **last, int lng)
{
  int i, ret;

  /* loop found */
  if ((node == init)&&(lng != 0)) /* lng==0 => 1st call */
    {
    *last = NULL;
    return(lng);
    }

  if (node->tag > 0)
    {/* indirect loop. stop now */
    return(0);
    }

  /* tag this node to prevent loops */
  node->tag = 1;

  i = 0;
  while(node->nexts[i] != NULL)
    {
    if ((ret = fc_get_cycle_(node->nexts[i], init, last, lng + 1)) > 0)
      {
      if (*last == NULL)
        {
        *last = node;
	}
      return(ret);
      }
    i++;
    }
  /* no match */

  return(0);
}

/* real call (for recursive reasons) */
int fc_get_cycle(FC_Node *node, FC_Node **last)
{
  int i;

  /* clear all tags */
  for(i=0; i<fc_nb_list_of_nodes; i++)
    {
    fc_list_of_nodes[i]->tag = 0;
    }
  return(fc_get_cycle_(node, node, last, 0));
}


/* compute list of cycles */
int fc_compute_cycles(int options)
{
  int i;
  unsigned int ncycle=1;  /* current cycle */
  FC_Node **nodes, *last;
  int nbinfo, ret;

  if (fc_list_of_nodes == NULL)
    return(0);

  nodes = fc_list_of_nodes;
  nbinfo = fc_nb_list_of_nodes;


  /* try to detect every cycles */
  for(i=0; i<nbinfo; i++)
    {
    last = NULL;
    ret = fc_get_cycle(nodes[i], &last);
    
    if (ret == 1)
      {
      fc_display_recurs(options, nodes[i], ncycle);
      ncycle++;
      }
    if (ret > 1)
      {
      fc_display_cycle(options, nodes[i], last, ncycle);
      ncycle++;
      }
    }

  return(ncycle - 1);
}


/* search a function. if do not exist, create a faked one */
FC_Function *fc_search_function(void *addr, int nb_fncs, FC_Function *fncs)
{
  int i;
  FC_Function *tmp;
  char buf[512];

  for(i=0; i<nb_fncs; i++)
    {
    if (fncs[i].symbol == addr)
      {
      return(&(fncs[i]));
      }
    }
  /* not found */
  tmp = malloc(sizeof(FC_Function));
  if (tmp == NULL)
    {
    fc_message("cannot allocate %d bytes for a faked function.", sizeof(FC_Function));
    return(NULL);
    }

  tmp->node = NULL;
  tmp->symbol = addr;
  sprintf(buf, "<%p>", addr);
  tmp->name.name = strdup(buf);
  return(tmp);
}

/* propagate 'show function' to the children */
void fc_propagate_yes(FC_Node *node)
{
  int i;

  if (node == NULL)
    return;

  node->function->hide = 0;
  node->treated = 1;

  i = 0;
  while(node->nexts[i] != NULL)
    {
    if (node->nexts[i]->treated == 0)
      fc_propagate_yes(node->nexts[i]);
    i++;
    }
}
/* retro-propagate 'show function' to the children */
void fc_rpropagate_yes(FC_Node *node)
{
  int i;

  if (node == NULL)
    return;

  node->function->hide = 0;
  node->treated = 1;

  i = 0;
  while(node->prevs[i] != NULL)
    {
    if (node->prevs[i]->treated == 0)
      fc_propagate_yes(node->prevs[i]);
    i++;
    }
}


/* propagate 'hide function' to the children */
void fc_propagate_no(FC_Node *node)
{
  int i;

  if (node == NULL)
    return;

  node->function->hide = 1;
  node->treated = 1;

  i = 0;
  while(node->nexts[i] != NULL)
    {
    if (node->nexts[i]->treated == 0)
      fc_propagate_yes(node->nexts[i]);
    i++;
    }
}
/* retro-propagate 'hide function' to the children */
void fc_rpropagate_no(FC_Node *node)
{
  int i;

  if (node == NULL)
    return;

  node->function->hide = 1;
  node->treated = 1;

  i = 0;
  while(node->prevs[i] != NULL)
    {
    if (node->prevs[i]->treated == 0)
      fc_propagate_yes(node->prevs[i]);
    i++;
    }
}




/* perform all treatments to generate the call-graph */
int fc_graph_create(int *_nb_arcs, FC_Arc *arcs,
                    int nb_fncs, FC_Function *fncs,
                    FC_NSym *only, int nb_only,
                    FC_NSym *not,  int nb_not,
                    int propagate, int rpropagate)
{
  int nb_arcs;
  int i, j;
  int dec;
  FC_Node *tmp;
  FC_Function *func;

  /* first remove duplicated entries in the arc table */
  nb_arcs = *_nb_arcs;
  fc_debug("  removing duplicated entries...");
  for(i=0; i<nb_arcs-1; i++)
    {
    for(j=i+1; j<nb_arcs; j++)
      {
      if ((arcs[i].from == arcs[j].from)&&
          (arcs[i].to == arcs[j].to))
        {
	arcs[i].from = arcs[i].to = NULL;
        /* sum the number of calls */
        arcs[j].number += arcs[i].number;
	break; /* no needs to test again */
	}
      }
    }
  dec = 0;
  fc_debug("  removing duplicated entries (bis)...");
  for(i=0; i<nb_arcs; i++)
    {
    arcs[dec].from = arcs[i].from;
    arcs[dec].to   = arcs[i].to;
    if (arcs[dec].from != NULL)
      dec++;
    }
  nb_arcs = dec;
  *_nb_arcs = dec;

  /* for each arc we search the corresponding function */
  fc_debug("  searching/creating arcs functions...");
  for(i=0; i<nb_arcs; i++)
    {
    arcs[i].ffrom = fc_search_function(arcs[i].from, nb_fncs, fncs);
    arcs[i].fto   = fc_search_function(arcs[i].to, nb_fncs, fncs);
    if ((arcs[i].ffrom == NULL)||(arcs[i].fto == NULL))
      {
      fc_message("error while computing call graph. removing call-graph.");
      return(0);
      }
    }

  /* now create a node for each part of a arc */
  fc_debug("  creating nodes for each arcs...");
  for(i=0; i<nb_arcs; i++)
    {
    if (arcs[i].ffrom->node == NULL)
      {
      tmp = fc_create_node(arcs[i].ffrom);
      arcs[i].ffrom->node = (void*)tmp;
      fc_register_node(tmp);
      }
    if (arcs[i].fto->node == NULL)
      {
      tmp = fc_create_node(arcs[i].fto);
      arcs[i].fto->node = (void*)tmp;
      fc_register_node(tmp);
      }
    }

  /* now create the call graph links */
  fc_debug("  linking nodes together...");
  for(i=0; i<nb_arcs; i++)
    {
    fc_add_child((FC_Node*)arcs[i].ffrom->node, (FC_Node*)arcs[i].fto->node, arcs[i].number);
    fc_add_prev( (FC_Node*)arcs[i].fto->node, (FC_Node*)arcs[i].ffrom->node, arcs[i].number);
    }

  /* now set/unset functions using -not/-only list */
  fc_debug("  if any, propagating not/only...");
  if ((nb_only != 0)&&(nb_not == 0))
    {
    /* only use these functions */
    /* first deselect all functions */
    for(i=0; i<nb_fncs; i++)
      {
      fncs[i].hide = 1;
      ((FC_Node*)fncs[i].node)->treated = 0;
      }
    /* for each function in the list activate it */
    for(i=0; i<nb_only; i++)
      {
      if (only[i].addr != NULL)
        {
        func = fc_search_function(only[i].addr, nb_fncs, fncs);
        func->hide = 0;
        /* propagate/retro-propagate */
        if (propagate)
          {
          fc_propagate_yes((FC_Node*)func->node);
          }
        if (rpropagate)
          {
          fc_rpropagate_yes((FC_Node*)func->node);
          }
        }
      }
    }
  else
  if ((nb_not != 0)&&(nb_only == 0))
    {
    /* do not use these functions */
    /* first select all functions */
    for(i=0; i<nb_fncs; i++)
      {
      fncs[i].hide = 0;
      ((FC_Node*)fncs[i].node)->treated = 0;
      }
    /* for each function in the list desactivate it */
    for(i=0; i<nb_not; i++)
      {
      if (not[i].addr != NULL)
        {
        func = fc_search_function(not[i].addr, nb_fncs, fncs);
        func->hide = 1;
        /* add the propagate/retro-propagate */
        if (propagate)
          {
          fc_propagate_no((FC_Node*)func->node);
          }
        if (rpropagate)
          {
          fc_rpropagate_no((FC_Node*)func->node);
          }
        }
      }
    }
  else
  if ((nb_not != 0)&&(nb_only != 0))
    {
    /* special mode */
    fc_message("combined -not/-only not implemented. Sorry.");
    }

  /* something else to compute ? */
  fc_debug("  all done.");
  return(1);
}

/* remove all nodes created */
int fc_graph_delete(int nb_arcs, FC_Arc *arcs,
                    int nb_fncs, FC_Function *fncs)
{
  int i;

  /* for each arc we search the node in the function */
  for(i=0; i<nb_arcs; i++)
    {
    if (arcs[i].ffrom->node != NULL)
      {
      arcs[i].ffrom->node = fc_delete_node(arcs[i].ffrom->node);
      }
    if (arcs[i].fto->node != NULL)
      {
      arcs[i].fto->node = fc_delete_node(arcs[i].fto->node);
      }
    }

  /* remove the local list of nodes */
  if (fc_list_of_nodes != NULL)
    free(fc_list_of_nodes);

  return(1);
}
