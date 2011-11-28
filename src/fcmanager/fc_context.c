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
/* fc_context.c: manage contexts for profile stats */

#include <time.h>
#include <limits.h>
#include "fc_context.h"
#include "fc_functions.h"
#include "fc_com.h"
#include "fc_com_manager.h"
#include "fc_global.h"


/* the active context */
FC_Context *fc_current_context = NULL;
int fc_current_context_id = 0;
/* default values for stack, func et graph sizes */
int fc_ctx_stack_size = 128;
int fc_ctx_func_size = 256;
int fc_ctx_graph_size = 512;
int fc_ctx_memory_size = 512;
char fc_ctx_name[512] = {'b', 'a', 'd', '_', 'i', 'n', 'i', 't', '\0'};
char fc_ctx_path[512] = {'\0'};
int fc_ctx_usepid = 0;


/* starting and ending time */
struct tm fc_ctx_start_time;
struct tm fc_ctx_stop_time;

/* unique ID for this particular execution */
char fc_ctx_unique_id[256] = "ndef";

/* mode of the profile */
int fc_ctx_pmode = FC_MODE_SINGLE;


/* list of existing contexts (for management purpose) */
FC_Context **fc_context_list = NULL;
int fc_nb_context_list = 0;
int fc_nb_context_max = 0;

/** current list of dynamic libraries **/
FC_LDYN *fc_list_ldyn = NULL;
int fc_nb_list_ldyn = 0;

/* set the current profile mode */
void fc_context_set_mode(int mode)
{
    fc_ctx_pmode = mode;
}

/* add an entry in the ldyn list */
int fc_ldyn_add(FC_LDYN *ldyn)
{
    int i;

    /* first check if the entry still exists */
    for (i = 0; i < fc_nb_list_ldyn; i++)
    {
        if (fc_list_ldyn[i].addr == ldyn->addr)
            return (1);
    }

    /* reallocate the table */
    if (fc_nb_list_ldyn == 0)
    {
        fc_nb_list_ldyn = 1;
        fc_list_ldyn = malloc(sizeof (FC_LDYN) * fc_nb_list_ldyn);
    }
    else
    {
        fc_nb_list_ldyn++;
        fc_list_ldyn = realloc(fc_list_ldyn, sizeof (FC_LDYN) * fc_nb_list_ldyn);
    }

    if (fc_list_ldyn == NULL)
    {
        fc_nb_list_ldyn = 0;
        fc_message("cannot (re)allocate %d bytes for dynamic lib list.",
                   sizeof (FC_LDYN) * fc_nb_list_ldyn);
        return (0);
    }

    /* add the element */
    fc_list_ldyn[fc_nb_list_ldyn - 1] = *ldyn;
    return (1);
}





/* loop on contexts */
int fc_context_loops = 0;

FC_Context *fc_context_first()
{
    fc_context_loops = 0;
    if (fc_nb_context_list == 0)
        return (NULL);
    return (fc_context_list[fc_context_loops]);
}

FC_Context *fc_context_next()
{
    fc_context_loops++;
    if (fc_context_loops >= fc_nb_context_list)
    {
        fc_context_loops = 0;
        return (NULL);
    }
    return (fc_context_list[fc_context_loops]);
}

/* register a new context in the list */
void fc_context_register(FC_Context *ctx)
{
    if (fc_context_list == NULL)
    {/* allocate the table */
        fc_context_list = malloc(sizeof (FC_Context*)*64);
        if (fc_context_list == NULL)
        {
            fc_message("cannot allocate initial context list!");
            fc_message_fatal(FC_ERR_MEM, "all data lost!");
        }
        fc_nb_context_max = 64;
    }

    fc_context_list[fc_nb_context_list++] = ctx;
    fc_debug("register context %d", fc_nb_context_list - 1);
    if (fc_nb_context_list == fc_nb_context_max)
    {/* reallocate the table */
        fc_context_list = realloc(fc_context_list, sizeof (FC_Context*)*
                                  (fc_nb_context_max + 64));
        if (fc_context_list == NULL)
        {
            fc_message("cannot reallocate context list!");
            fc_message_fatal(FC_ERR_MEM, "all data lost!");
        }
        fc_nb_context_max += 64;
    }
}

/* create a new context */
FC_Context *fc_context_create(int id, unsigned int first, int stack_size,
                              int func_size, int graph_size, int memory_size)
{
    int i;
    FC_Context *tmp;

    tmp = malloc(sizeof (FC_Context));
    if (tmp == NULL)
    {
        fc_message("cannot allocate %d bytes for a context.", sizeof (FC_Context));
        return (NULL);
    }

    /* init sub-structures */
    tmp->graph = fc_graph_init(graph_size > 0 ? graph_size : 512);
    if (tmp->graph == NULL)
    {
        fc_message("cannot finish context initialization (graph).");
        free(tmp);
        return (NULL);
    }
    tmp->stack = fc_stack_create(stack_size > 0 ? stack_size : 128);
    if (tmp->stack == NULL)
    {
        fc_message("cannot finish context initialization (stack).");
        fc_graph_free(tmp->graph);
        free(tmp);
        return (NULL);
    }
    tmp->functions = fc_fhash_create(func_size > 0 ? func_size : 512);
    if (tmp->functions == NULL)
    {
        fc_message("cannot finish context initialization (functions).");
        fc_graph_free(tmp->graph);
        fc_stack_delete(tmp->stack);
        free(tmp);
        return (NULL);
    }
    /* TODO: real initialization */
    tmp->memory = fc_memory_create(memory_size > 0 ? memory_size : 512);
    if (tmp->memory == NULL)
    {
        fc_message("cannot finish context initialization (memory).");
        fc_graph_free(tmp->graph);
        fc_stack_delete(tmp->stack);
        fc_fhash_delete(tmp->functions);
        free(tmp);
        return (NULL);
    }

    tmp->id = id;
    tmp->pid = 0;
    tmp->first_time = (unsigned long long) first;
    tmp->last_time = (unsigned long long) first;
    tmp->ulast_time = first;
    tmp->time_pad = 0;

    /* 1st call: set the default values */
    if (fc_current_context == NULL)
    {
        fc_current_context = tmp;
        fc_current_context_id = id;
        fc_ctx_stack_size = stack_size;
        fc_ctx_func_size = func_size;
        fc_ctx_graph_size = graph_size;
        fc_ctx_memory_size = memory_size;
        /* set the unique ID */
        sprintf(fc_ctx_unique_id, "%d_%d_%d", (int) getpid(), tmp->id,
                (int) time(NULL));
    }
    else
    {/* in case of fork mode, create a set of empty functions 
        to allow exits from unknown functions for childs */
        if (fc_mcom_mode == FC_MODE_FORK)
        {
            fc_current_context = tmp;
            for (i = 0; i < 16; i++)
                fc_functions_enter(NULL, NULL, first);
        }
    }

    return (tmp);
}

/* set the current context regards to the ID (create it if needed) */
void fc_context_set(int id, unsigned int first_time)
{
    int i;
    FC_Context *ctx;

    /* still the good context */
    if (id == fc_current_context_id)
        return;

    /* search the context */
    for (i = 0; i < fc_nb_context_list; i++)
    {
        if (fc_context_list[i]->id == id)
        {/* set the context */
            fc_debug("set context %d", i);
            fc_current_context = fc_context_list[i];
            fc_current_context_id = id;
            return;
        }
    }
    /* not found. create a new one */
    ctx = fc_context_create(id, first_time, fc_ctx_stack_size,
                            fc_ctx_func_size, fc_ctx_graph_size,
                            fc_ctx_memory_size);
    if (ctx != NULL)
    {
        fc_context_register(ctx);
        fc_current_context = ctx;
        fc_current_context_id = id;
        fc_debug("new ID (%d)", id);
    }
    else
    {/* this is a priori fatal... */
        fc_message("new context not created.");
    }
}

/* delete a context */
void fc_context_delete(FC_Context *ctx)
{
    /* delete parts of the structure */
    fc_fhash_delete(ctx->functions);
    fc_stack_delete(ctx->stack);
    fc_graph_free(ctx->graph);
    fc_memory_delete(ctx->memory);
    /* delete the structure */
    free(ctx);
}

/* compute the context file name */
int fc_context_getname(FC_Context *ctx, char *name)
{
    char tname[512];

    /* effective name */
    if (fc_ctx_usepid)
        sprintf(tname, "%s.%d.fc", fc_ctx_name, (int) getpid());
    else
        sprintf(tname, "%s.fc", fc_ctx_name);
    if (fc_nb_context_list == 1) /* only one process */
    {
        sprintf(name, "%s/%s", fc_ctx_path, tname);
    }
    else
    {
        sprintf(name, "%s/%s.%d", fc_ctx_path, tname, ctx->id);
    }
    return (1);
}

/* dump an arc to the given FILE */
void fc_arc_dump(unsigned long long key, int val, void *ptr1, void *ptr2, void *user_data)
{
    void *from, *to;
    unsigned long long mask = 0;
    unsigned int i;

    /* mask for the 1st word */
    for (i = 0; i<sizeof (int) *8; i++)
    {
        mask |= ((unsigned long long) 1 << i);
    }
    /* extract from/to */
    to = (void*) ((unsigned int) (key & mask));
    from = (void*) ((unsigned int) ((key >> (sizeof (int) *8)) & mask));

    fprintf((FILE*) user_data, "%p %p %d\n", from, to, val);
}

/* save a given context */
int fc_context_save(FC_Context *ctx)
{
    char fname[512];
    FILE *f;
    int i, j, nbl, nbf, nbr;

    /* compute the file name */
    fc_context_getname(ctx, fname);

    if ((f = fopen(fname, "w")) == NULL)
    {
        fc_message("cannot create profile file '%s'.", fname);
        return (0);
    }

    /* save the header */
    fprintf(f, "%s\n", FC_CTX_HEADER);
    /* the unique ID */
    fprintf(f, "%s\n", fc_ctx_unique_id);
    /* time mode */
    fprintf(f, "%d\n", fc_get_time_type());
    /* profile mode */
    fprintf(f, "%d\n", fc_ctx_pmode);
    /* ID */
    fprintf(f, "%d\n", ctx->id);
    /* Parent ID */
    fprintf(f, "%d\n", ctx->pid);
    /* execution time */
    fprintf(f, "%lld\n", ctx->last_time - ctx->first_time);
    /* save call-graph (arcs) */
    fprintf(f, "%d\n", (int) fc_lhash_size(ctx->graph));
    /* dump each data */
    fc_lhash_foreach(ctx->graph, fc_arc_dump, (void *) f);

    /* save functions stats */
    /* check the real number of functions */
    j = 0;
    for (i = 0; i < ctx->functions->current_size; i++)
    {
        if (ctx->functions->functions[i].symbol != NULL)
        {
            j++;
        }
    }
    fprintf(f, "%d\n", j);
    for (i = 0; i < ctx->functions->current_size; i++) /*this may be in fc_hash */
    {
        if (ctx->functions->functions[i].symbol != NULL)
            fprintf(f, "%p %u %lld %lld %lld %lld %lld %lld\n",
                    ctx->functions->functions[i].symbol,
                    ctx->functions->functions[i].calls,
                    ctx->functions->functions[i].local_time,
                    ctx->functions->functions[i].total_time,
                    ctx->functions->functions[i].min_time,
                    ctx->functions->functions[i].max_time,
                    ctx->functions->functions[i].min_ltime,
                    ctx->functions->functions[i].max_ltime);
    }

    /* list of dynamic lib */
    fprintf(f, "%d\n", fc_nb_list_ldyn);
    for (i = 0; i < fc_nb_list_ldyn; i++)
    {
        fprintf(f, "%p %s\n", fc_list_ldyn[i].addr, fc_list_ldyn[i].name);
    }

    /* list of memory leaks */
    if ((ctx->memory == NULL) || (ctx->memory->list == NULL))
    {/* no entry */
        fprintf(f, "0\n");
        fprintf(f, "0\n");
        fprintf(f, "0\n");
        fclose(f);
    }

    /* # of elements */
    nbl = 0;
    nbr = 0;
    nbf = 0;
    for (i = 0; i < ctx->memory->nb_elements; i++)
    {
        if (ctx->memory->list[i].set)
        {
            if (ctx->memory->list[i].size == UINT_MAX)
            {/* invalid free or realloc */
                if (ctx->memory->list[i].realloc_place == NULL)
                {/* free */
                    nbf++;
                }
                else
                {/* realloc */
                    nbr++;
                }
            }
            else
            {/* memory leak */
                nbl++;
            }
        }
    }

    /* leaks */
    fprintf(f, "%d\n", nbl);
    for (i = 0; i < ctx->memory->nb_elements; i++)
    {
        if (ctx->memory->list[i].set)
        {
            if (ctx->memory->list[i].size != UINT_MAX)
            {
                fprintf(f, "%p %u %p %p", ctx->memory->list[i].pointer,
                        ctx->memory->list[i].size, ctx->memory->list[i].alloc_place,
                        ctx->memory->list[i].realloc_place);
                /* call-stack */
                j = 0;
                while ((j < fc_memory_stack_size) && (ctx->memory->list[i].alloc_stack[j] != NULL))
                {
                    fprintf(f, " %p", ctx->memory->list[i].alloc_stack[j]);
                    j++;
                }
                /* end of list */
                fprintf(f, " %p\n", NULL);
            }
        }
    }

    /* invalid free */
    fprintf(f, "%d\n", nbf);
    for (i = 0; i < ctx->memory->nb_elements; i++)
    {
        if (ctx->memory->list[i].set)
        {
            if ((ctx->memory->list[i].size == UINT_MAX) &&
                    (ctx->memory->list[i].alloc_place != NULL))
            {
                fprintf(f, "%p %p", ctx->memory->list[i].alloc_place,
                        ctx->memory->list[i].pointer);
                /* call-stack */
                j = 0;
                while ((j < fc_memory_stack_size) && (ctx->memory->list[i].alloc_stack[j] != NULL))
                {
                    fprintf(f, " %p", ctx->memory->list[i].alloc_stack[j]);
                    j++;
                }
                /* end of list */
                fprintf(f, " %p\n", NULL);
            }
        }
    }

    /* invalid realloc */
    fprintf(f, "%d\n", nbr);
    for (i = 0; i < ctx->memory->nb_elements; i++)
    {
        if (ctx->memory->list[i].set)
        {
            if ((ctx->memory->list[i].size == UINT_MAX) &&
                    (ctx->memory->list[i].realloc_place != NULL))
            {
                fprintf(f, "%p %p", ctx->memory->list[i].realloc_place,
                        ctx->memory->list[i].pointer);
                /* call-stack */
                j = 0;
                while ((j < fc_memory_stack_size) && (ctx->memory->list[i].alloc_stack[j] != NULL))
                {
                    fprintf(f, " %p", ctx->memory->list[i].alloc_stack[j]);
                    j++;
                }
                /* end of list */
                fprintf(f, " %p\n", NULL);
            }
        }
    }

    fclose(f);
    return (0);
}

/* save all existing contexts */
int fc_context_save_all()
{
    int i, ret;
    char name[512];
    FILE *f;


    ret = 1;
    for (i = 0; i < fc_nb_context_list; i++)
        ret &= fc_context_save(fc_context_list[i]);

    /* if needed, create the execution-tree of processes */
    if (fc_nb_context_list > 1)
    {
        if (fc_ctx_usepid)
            sprintf(name, "%s/%s.%d.fcd", fc_ctx_path, fc_ctx_name, (int) getpid());
        else
            sprintf(name, "%s/%s.fcd", fc_ctx_path, fc_ctx_name);
        if ((f = fopen(name, "w")) == NULL)
        {
            fc_message("cannot create FunctionCheck descriptor file '%s'.", name);
            return (0);
        }
        fc_message("Profiles descriptor file is '%s'", name);
        fprintf(f, "FunctionCheck descriptor file.\n");
        fprintf(f, "%d processes treated.\n", fc_nb_context_list);
        fprintf(f, "Treatments started at: %d/%02d/%02d %02d:%02d:%02d\n",
                fc_ctx_start_time.tm_year + 1900, fc_ctx_start_time.tm_mon + 1,
                fc_ctx_start_time.tm_mday, fc_ctx_start_time.tm_hour,
                fc_ctx_start_time.tm_min, fc_ctx_start_time.tm_sec);
        fprintf(f, "Treatments ended at: %d/%02d/%02d %02d:%02d:%02d\n",
                fc_ctx_stop_time.tm_year + 1900, fc_ctx_stop_time.tm_mon + 1,
                fc_ctx_stop_time.tm_mday, fc_ctx_stop_time.tm_hour,
                fc_ctx_stop_time.tm_min, fc_ctx_stop_time.tm_sec);
        fprintf(f, "\n");
        for (i = 0; i < fc_nb_context_list; i++)
        {
            fc_context_getname(fc_context_list[i], name);
            fprintf(f, "Process id: %d\n", fc_context_list[i]->id);
            if (fc_context_list[i]->pid != 0)
                fprintf(f, "    parent: %d\n", fc_context_list[i]->pid);
            else
                fprintf(f, "    parent: pthread_create\n");
            fprintf(f, "      file: %s\n", name);
            fprintf(f, "\n");
        }
        fclose(f);
    }
    else
    {
        fc_context_getname(fc_context_list[i], name);
        fc_message("Profile file is '%s'", name);
    }

    return (ret);
}

/* set default values */
inline void fc_context_set_functions(int n)
{
    fc_ctx_func_size = n;
}

inline void fc_context_set_graph(int n)
{
    fc_ctx_graph_size = n;
}

inline void fc_context_set_stack(int n)
{
    fc_ctx_stack_size = n;
}

inline void fc_context_set_memory(int n)
{
    fc_ctx_memory_size = n;
}

inline void fc_context_set_name(char *n)
{
    sprintf(fc_ctx_name, "%s", n);
}

inline void fc_context_set_path(char *n)
{
    sprintf(fc_ctx_path, "%s", n);
}

inline void fc_context_set_usepid(int n)
{
    fc_ctx_usepid = n;
}

inline void fc_context_set_starttime(time_t start)
{
    fc_ctx_start_time = *(localtime(&start));
}

inline void fc_context_set_stoptime(time_t stop)
{
    fc_ctx_stop_time = *(localtime(&stop));
}


