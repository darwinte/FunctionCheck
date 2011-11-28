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
/** fc_dump.c:  **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fc_dump.h"
#include "fc_graph.h"
#include "fc_tools.h"
#include "fc_global.h"
#include "fc_names.h"
#include "demangle.h"





/* list of arcs */
FC_Arc *fc_list_of_arcs = NULL;
int fc_nb_list_of_arcs = 0;

/* list of functions */
FC_Function *fc_list_of_functions = NULL;
int fc_nb_list_of_functions = 0;

/* list of libs */
FC_LDyn *fc_list_of_lib = NULL;
int fc_nb_list_of_lib = 0;

/* list of memory leaks */
FC_MLeak *fc_list_of_leaks = NULL;
int fc_nb_list_of_leaks = 0;

/* list of invalid free */
FC_MFree *fc_list_of_free = NULL;
int fc_nb_list_of_free = 0;

/* list of invalid realloc */
FC_MRealloc *fc_list_of_realloc = NULL;
int fc_nb_list_of_realloc = 0;



/* global flag to reverse sort order */
int fc_sort_order=-1;


/* the same function than scanf which test for
   unexpected end of file */
#define fc_fscanf(f, args...) {fscanf(f, ##args); if (feof(f)) \
  fc_message_fatal(FC_ERR_EOF, "profile data file seems to be troncated."); }



/* usage of the program */
void usage(char *name)
{
  printf("%s V%s by Y.Perret\n", FC_PACKAGE, FC_VERSION);
  printf("Usage: %s [options] prog\n", name);
  printf("Options are:\n"
         "   --help         : print this message\n"
	 "   --version      : functioncheck version\n"
	 "   --contact      : informations relative to functioncheck\n"
	 "   -debug         : switch to debug mode\n"
	 "   -pfile <file>  : set the profile data file name\n"
	 "   -flat          : display flat profile (default)\n"
	 "   -no-flat       : do not display flat profile\n"
         "   -sort <type>   : set the sort mode for flat profile\n"
         "   -rsort <type>  : set the sort mode for flat profile (reverse)\n"
         "   -cumul         : add the cumuled %% of local time (-> -sort local)\n"
         "   -graph         : display call-graph (default)\n"
         "   -rgraph        : display reverse call-graph (callers instead of called)\n"
	 "   -no-graph      : do not display call-graph\n"
	 "   -header        : display 'FunctionDump' header (default)\n"
	 "   -no-header     : do not display 'FunctionDump' header\n"
	 "   -info          : display general informations on the profile (default)\n"
	 "   -no-info       : do not display general informations\n"
	 "   -demangle      : demangle functions name (default)\n"
	 "   -demangle-gnu  : demangle style GNU\n"
	 "   -demangle-java : demangle style JAVA\n"
	 "   -demangle-ansi : include const, volatile... (put it AFTER style)\n"
	 "   -demangle-params: include function args (put it AFTER style)\n"
	 "   -no-demangle   : do not demangle names\n"
	 "   -details       : add file/line for functions name\n"
	 "   -basename      : use basename for files\n"
	 "   -no-names      : do not convert symbols into names\n"
	 "   -cycles        : display detected cycles in the call-graph (default)\n"
	 "   -no-cycles     : do not display detected cycles\n"
	 "   -only <list>   : only use these functions (f1,f2,f3...,fn)\n"
	 "   -not <list>    : do not use these functions (f1,f2,f3...,fn)\n"
	 "   -propagate     : propagate -not/-only effect to children\n"
	 "   -rpropagate    : propagate -not/-only effect to parents\n"
	 "   -vcg           : print call-graph in VCG format and exit\n"
	 );
  printf("   -memory        : print memory informations (default)\n"
         "   -no-memory     : do not print memory informations at all\n"
         "   -mem-leaks     : print memory leaks (if any) (default)\n"  
         "   -no-mem-leaks  : do not print memory leaks\n"
         "   -mem-free      : print invalid free (if any) (default)\n"  
         "   -no-mem-free   : do not print invalid free\n"
         "   -mem-realloc   : print invalid realloc (if any) (default)\n"  
         "   -no-mem-realloc: do not print invalid realloc\n" 
         "   -force-names   : gives addresses if symbols are not solved\n"
         );

  printf("\n"
	 "<type> for sort mode are:\n"
	 "  total   : by total time\n"
	 "  local   : by local time\n"
	 "  name    : by function name\n"
	 "  calls   : by number of calls\n"
	 "  min     : by min total time\n"
	 "  max     : by max total time\n"
	 "  lmin    : by min local time\n"
	 "  lmax    : by max local time\n");
  printf("\n");
}

/* current version */
void version(char *name)
{
  printf("%s V%s\n", FC_PACKAGE, FC_VERSION);
}

/* various informations */
void contacts(char *name)
{
  printf("\n%s V%s\n\n", FC_PACKAGE, FC_VERSION);
  printf("Author: Yannick Perret\n"
         "  mail: yperret@ligim.univ-lyon1.fr\n"
	 "\n"
	 "WEB page:\n"
	 "http://sourceforge.net/projects/fnccheck/\n"
	 "\n"
	 "Send bugs via sourceforge page.\n\n");
}


/** list of sort functions **/
/* by total time */
int fc_sort_total(const void *f1, const void *f2)
{
  if (((FC_Function*)f1)->total_time <
      ((FC_Function*)f2)->total_time)
    return(fc_sort_order*-1);
  else
  if (((FC_Function*)f1)->total_time >
      ((FC_Function*)f2)->total_time)
    return(fc_sort_order*1);
  return(0);
}
/* by local time */
int fc_sort_local(const void *f1, const void *f2)
{
  if (((FC_Function*)f1)->local_time <
      ((FC_Function*)f2)->local_time)
    return(fc_sort_order*-1);
  else
  if (((FC_Function*)f1)->local_time >
      ((FC_Function*)f2)->local_time)
    return(fc_sort_order*1);
  return(0);
}
/* by name time */
int fc_sort_name(const void *f1, const void *f2)
{
  return(fc_sort_order*strcmp(((FC_Function*)f1)->name.name, ((FC_Function*)f2)->name.name));
}
/* by calls time */
int fc_sort_calls(const void *f1, const void *f2)
{
  if (((FC_Function*)f1)->calls <
      ((FC_Function*)f2)->calls)
    return(fc_sort_order*-1);
  else
  if (((FC_Function*)f1)->calls >
      ((FC_Function*)f2)->calls)
    return(fc_sort_order*1);
  return(0);
}
/* by max time */
int fc_sort_max(const void *f1, const void *f2)
{
  if (((FC_Function*)f1)->max_time <
      ((FC_Function*)f2)->max_time)
    return(fc_sort_order*-1);
  else
  if (((FC_Function*)f1)->max_time >
      ((FC_Function*)f2)->max_time)
    return(fc_sort_order*1);
  return(0);
}
/* by min time */
int fc_sort_min(const void *f1, const void *f2)
{
  if (((FC_Function*)f1)->min_time <
      ((FC_Function*)f2)->min_time)
    return(fc_sort_order*-1);
  else
  if (((FC_Function*)f1)->min_time >
      ((FC_Function*)f2)->min_time)
    return(fc_sort_order*1);
  return(0);
}
/* by min local time */
int fc_sort_lmin(const void *f1, const void *f2)
{
  if (((FC_Function*)f1)->min_ltime <
      ((FC_Function*)f2)->min_ltime)
    return(fc_sort_order*-1);
  else
  if (((FC_Function*)f1)->min_ltime >
      ((FC_Function*)f2)->min_ltime)
    return(fc_sort_order*1);
  return(0);
}
/* by max local time */
int fc_sort_lmax(const void *f1, const void *f2)
{
  if (((FC_Function*)f1)->max_ltime <
      ((FC_Function*)f2)->max_ltime)
    return(fc_sort_order*-1);
  else
  if (((FC_Function*)f1)->max_ltime >
      ((FC_Function*)f2)->max_ltime)
    return(fc_sort_order*1);
  return(0);
}


/* get the basename of a given filename */
char *fc_basename(char *name)
{
  int i;

  for(i=strlen(name)-1; i>=0; i--)
    {
    if (name[i] == '/')
      break;
    }
  if (name[i] != '/')
    return(name);
  return(&(name[i+1]));
}

/* set a buffer with the function name or address */
void fc_convert_name(FC_Function *fnc, char *buffer, int details, int basen)
{
  char temp[1024];

  buffer[0] = '\0';
  if (fnc->name.name != NULL)
    strcat(buffer, fnc->name.name);
  else
    {
    sprintf(buffer, "%p", fnc->symbol);
    }

  /* if details requiered */
  if (details)
    {
    if (fnc->name.object == NULL)
      strcat(buffer, ":??");
    else
      {
      strcat(buffer, ":");
      if (basen)
        strcat(buffer, fc_basename(fnc->name.object));
      else
        strcat(buffer, fnc->name.object);
      }
    sprintf(temp, ":%d", fnc->name.line);
    strcat(buffer, temp);
    }
}


/* reads 'string' as a comma-separated list of functions and create a NSym
   list using it. If names are addresses (in decimal or hexadecimal format)
   they are directly converted into symbol address.  */
int fc_read_flist(FC_NSym **list, int *nb_list, char *_string)
{
  int i, nb, taddr;
  char *pos;
  char string[1024]={'\0'};

  /* first duplicate the incoming string (it might be write protected) */
  strcat(string, _string);
  /* count the number of ',' to allocate the list */
  nb = 0;
  for(i=0; i<strlen(string); i++)
    {
    if (string[i] == ',')
      nb++;
    }
  nb++;

  /* allocate or reallocate the list */
  if (*list == NULL)
    {
    if (((*list) = malloc(sizeof(FC_NSym)*nb)) == NULL)
      {
      fc_message("cannot allocate %d bytes for a list of functions.", sizeof(FC_NSym)*nb);
      *list = NULL;
      *nb_list = 0;
      return(0);
      }
    }
  else
    {
    if (((*list) = realloc(*list, sizeof(FC_NSym)*(*nb_list+nb))) == NULL)
      {
      fc_message("cannot reallocate %d bytes for a list of functions.", sizeof(FC_NSym)*(nb+*nb_list));
      *list = NULL;
      *nb_list = 0;
      return(0);
      }
    }

  /* clean up */
  for(i=0; i<nb; i++)
    {
    (*list)[*nb_list+i].name = NULL;
    (*list)[*nb_list+i].file = NULL;
    (*list)[*nb_list+i].addr = NULL;
    }

  /* parse each entry */
  strtok(string, ",");
  pos = string;
  for(i=0; i<nb; i++)
    {
    if (pos == NULL)
      {
      fc_message("unexpected end of function list!");
      break;
      }
    /* numerical address */
    if ((pos[0] >= '0')&&(pos[0] <= '9'))
      {
      if ((pos[1] == 'x')||(pos[1] == 'X'))
        {/* hexadecimal address */
	sscanf(pos, "%p", &((*list)[*nb_list+i].addr));
	}
      else
        {/* integer address (anyone gives integer addresses ?) */
	sscanf(pos, "%d", &taddr);
	(*list)[*nb_list+i].addr = (void*)taddr;
	}
      }
    else
      {/* duplicate the text entry */
      (*list)[*nb_list+i].name = strdup(pos);
      }
    /* go to the next token */
    pos = strtok(NULL, ",");
    }

  /* if an unexected end of list occur, i might be < than nb */
  *nb_list += i;

  return(1);
}


int fc_read_stack(FILE *f, void **stack)
{
  void *tmp;
  int pos = 0;

  fscanf(f, "%p", &tmp);
  while(tmp != NULL)
    {
    stack[pos++] = tmp;
    if (pos == FC_MAX_STACK_SIZE-1)
      {
      break;
      }
    fscanf(f, "%p", &tmp);
    }
  stack[pos] = NULL;

  return(pos);
}
int fc_print_stack(void **stack, FC_Name *names)
{
  int i = 0;

  while(stack[i] != NULL)
    {
    printf(" %p", stack[i]);
    i++;
    }

  printf("\n");
  return(1);
}

/* main */
int main(int argc, char *argv[])
{
  int i, j, rj;
  /* relative to profile data file */
  char *pfile = NULL;
  char *efile = NULL;
  FILE *f;
  /* temporary data */
  char temp[1024];
  /* status of the profile */
  int time_mode, prof_mode;
  int id, pid;
  long long int total_time;
  /* function pointer for sorts */
  int (*to_sort)(const void*,const void*) = fc_sort_total;
  /* cumul treatments */
  int cumul = 0;
  long long int total_cumul = 0;
  char scum[32];
  /* call graph modifiers */
  int no_call_graph = 0;
  int call_called = 0;
  /* flat profile */
  int no_flat_profile = 0;
  /* header */
  int no_header = 0;
  /* general */
  int no_general = 0;
  /* demangle */
  int demangle =  1;
  int style = DMGL_AUTO; /* default */
  /* details */
  int draw_details = 0;
  int use_basename = 0;
  /* names */
  int no_names = 0;
  /* cycles */
  int no_cycles = 0;
  /* -only/-not list of functions (symbols and names) */
  FC_NSym * only_list = NULL;
  int nb_only_list = 0;
  FC_NSym * not_list = NULL;
  int nb_not_list = 0;
  int do_propagate = 0;
  int do_rpropagate = 0;
  int nb_hide = 0; /* number of hiden functions */
  /* call-graph output */
  int show_callgraph = 0;
  /* for call-stack managment */
  int nbstack = 0;
  /* memory info */
  int no_memory = 0;
  int no_memory_leaks = 0;
  int no_memory_free = 0;
  int no_memory_realloc = 0;
  /* debug */
  int debug_state = 0;
  /* force the use of addresses if invalid object */
  int force_names = 0;

  /* name for messages */
  fc_set_message_name("FDump");

  /** first, read the options **/
  if (argc < 2)
    {
    usage(argv[0]);
    fc_message_fatal(FC_ERR_ARGS, "not enough parameters.");
    }
  fc_debug("reading command line (%d)", argc);
  for(i=1; i<argc; i++)
    {
    if (argv[i][0] == '-')  /* an option */
      {/* print usage */
      if ((strcmp(argv[i], "--help") == 0)||
          (strcmp(argv[i], "-help") == 0)||
          (strcmp(argv[i], "--h") == 0)||
          (strcmp(argv[i], "-h") == 0)||
          (strcmp(argv[i], "-?") == 0))
	{
	usage(argv[0]);
	return(FC_ERR_OK);
	}
      else
      if ((strcmp(argv[i], "--version") == 0)||
          (strcmp(argv[i], "-version") == 0)||
          (strcmp(argv[i], "--v") == 0)||
          (strcmp(argv[i], "-v") == 0))
	{/* current version */
	version(argv[0]);
	return(FC_ERR_OK);
	}
      else
      if (strcmp(argv[i], "--contact") == 0)
	{/* various informations */
	contacts(argv[0]);
	return(FC_ERR_OK);
	}
      else
      if (strcmp(argv[i], "-cumul") == 0)
	{/* add cumul */
	cumul = 1;
	to_sort = fc_sort_local;
	}
      else
      if (strcmp(argv[i], "-debug") == 0)
	{/* debug mode */
	debug_state = 1;
	fc_set_debug_mode(debug_state);
	}
      else
      if (strcmp(argv[i], "-no-graph") == 0)
	{/* do not display call-graph */
	no_call_graph = 1;
	}
      else
      if (strcmp(argv[i], "-graph") == 0)
	{/* display call-graph */
	no_call_graph = 0;
	call_called = 0;
	}
      else
      if (strcmp(argv[i], "-rgraph") == 0)
	{/* display reverse call-graph */
	no_call_graph = 0;
	call_called = 1;
	}
      else
      if (strcmp(argv[i], "-flat") == 0)
	{/* display flat profile */
	no_flat_profile = 0;
	}
      else
      if (strcmp(argv[i], "-no-flat") == 0)
	{/* do not display flat profile */
	no_flat_profile = 1;
	}
      else
      if (strcmp(argv[i], "-info") == 0)
	{/* display general info */
	no_general = 0;
	}
      else
      if (strcmp(argv[i], "-no-info") == 0)
	{/* do not display general info */
	no_general = 1;
	}
      else
      if (strcmp(argv[i], "-header") == 0)
	{/* display header */
	no_header = 0;
	}
      else
      if (strcmp(argv[i], "-no-header") == 0)
	{/* do not display header */
	no_header = 1;
	}
      else
      if (strcmp(argv[i], "-vcg") == 0)
	{/* only display call-graph in VCG format */
	show_callgraph = 1;
	}
      else
      if (strcmp(argv[i], "-force-names") == 0)
	{/* force addresses if names not found */
	force_names = 1;
	}
      else
      if (strcmp(argv[i], "-demangle") == 0)
	{/* demangle names */
	demangle = 1;
	style = DMGL_AUTO;
	}
      else
      if (strcmp(argv[i], "-demangle-gnu") == 0)
	{/* demangle names */
	demangle = 1;
	style = DMGL_GNU;
	}
      else
      if (strcmp(argv[i], "-demangle-java") == 0)
	{/*  demangle names */
	demangle = 1;
	style = DMGL_JAVA;
	}
      else
      if (strcmp(argv[i], "-demangle-ansi") == 0)
	{/* demangle names */
	demangle = 1;
	style |= DMGL_ANSI;
	}
      else
      if (strcmp(argv[i], "-demangle-params") == 0)
	{/* demangle names */
	demangle = 1;
	style |= DMGL_PARAMS;
	}
      else
      if (strcmp(argv[i], "-no-demangle") == 0)
	{/* do not demangle names */
	demangle = 0;
	}
      else
      if (strcmp(argv[i], "-details") == 0)
	{/* add file/line to names */
	draw_details = 1;
	}
      else
      if (strcmp(argv[i], "-basename") == 0)
	{/* use basename for files */
	use_basename = 1;
	}
      else
      if (strcmp(argv[i], "-no-names") == 0)
	{/* do not solve names */
	no_names = 1;
	}
      else
      if (strcmp(argv[i], "-no-cycles") == 0)
	{/* do not display cycles */
	no_cycles = 1;
	}
      else
      if (strcmp(argv[i], "-cycles") == 0)
	{/* display cycles */
	no_cycles = 0;
	}
      else
      if (strcmp(argv[i], "-no-memory") == 0)
	{/* do not display memory at all */
	no_memory = 1;
	}
      else
      if (strcmp(argv[i], "-memory") == 0)
	{/* display memory */
	no_memory = 0;
	}
      else
      if (strcmp(argv[i], "-no-mem-leaks") == 0)
	{/* do not display memory leaks */
	no_memory_leaks = 1;
	}
      else
      if (strcmp(argv[i], "-mem-leaks") == 0)
	{/* display memory leaks */
	no_memory_leaks = 0;
	}
      else
      if (strcmp(argv[i], "-no-mem-free") == 0)
	{/* do not display invalid free */
	no_memory_free = 1;
	}
      else
      if (strcmp(argv[i], "-mem-free") == 0)
	{/* display invalid free */
	no_memory_free = 0;
	}
      else
      if (strcmp(argv[i], "-no-mem-realloc") == 0)
	{/* do not display invalid realloc */
	no_memory_realloc = 1;
	}
      else
      if (strcmp(argv[i], "-mem-realloc") == 0)
	{/* display invalid realloc */
	no_memory_realloc = 0;
	}
      else
      if ((strcmp(argv[i], "-only") == 0)&&(i+1 < argc))
	{/* list of functions */
	if (!fc_read_flist(&only_list, &nb_only_list, argv[i+1]))
	  {
	  fc_message("'-only' option will not be used.");
	  }
	i++;
	}
      else
      if ((strcmp(argv[i], "-not") == 0)&&(i+1 < argc))
	{/* list of functions */
	if (!fc_read_flist(&not_list, &nb_not_list, argv[i+1]))
	  {
	  fc_message("'-not' option will not be used.");
	  }
	i++;
	}
      else
      if (strcmp(argv[i], "-propagate") == 0)
	{/* propagate -not/-only */
	do_propagate = 0;
	}
      else
      if (strcmp(argv[i], "-rpropagate") == 0)
	{/* reverse-propagate -not/-only */
	do_rpropagate = 0;
	}
      else
      if ((strcmp(argv[i], "-pfile") == 0)&&(i+1 < argc))
	{/* the profile data file name */
	if (pfile != NULL)
	  free(pfile);
	pfile = strdup(argv[i+1]);
	i++;
	}
      else
      if (((strcmp(argv[i], "-sort") == 0)||(strcmp(argv[i], "-rsort") == 0))&&(i+1 < argc))
	{/* the sort mode */
	if (strcmp(argv[i], "-sort") == 0)
	  fc_sort_order = -1;
	else
	  fc_sort_order = 1;
	i++;
	if (strcmp(argv[i], "total") == 0)
	  to_sort = fc_sort_total;
	else
	if (strcmp(argv[i], "local") == 0)
	  to_sort = fc_sort_local;
	else
	if (strcmp(argv[i], "name") == 0)
	  to_sort = fc_sort_name;
	else
	if (strcmp(argv[i], "calls") == 0)
	  to_sort = fc_sort_calls;
	else
	if (strcmp(argv[i], "min") == 0)
	  to_sort = fc_sort_min;
	else
	if (strcmp(argv[i], "max") == 0)
	  to_sort = fc_sort_max;
	else
	if (strcmp(argv[i], "lmin") == 0)
	  to_sort = fc_sort_lmin;
	else
	if (strcmp(argv[i], "lmax") == 0)
	  to_sort = fc_sort_lmax;
	else
	  {
	  fc_message("WARNING: unknown sort mode '%s'. ignored.", argv[i]);
	  }
	}
      else
        {
	fc_message_fatal(FC_ERR_ARGS, "unknown option '%s'.", argv[i]);
	}
      }
    else /* the program name */
      {
      if (efile == NULL)
        {
	efile = strdup(argv[i]);
	}
      else /* duplicatation! */
        {
	fc_message("program name given (%s) as an other one", argv[i]);
	fc_message_fatal(FC_ERR_ARGS, "  still exists (%s).", efile);
	}
      }
    }
  /* no program name! */
  if (efile == NULL)
    {
    usage(argv[0]);
    fc_message_fatal(FC_ERR_ARGS, "no program name given.");
    }
  /* default name */
  if (pfile == NULL)
    pfile = strdup("functioncheck.fc");


  /** open the profile data file **/
  fc_debug("opening profile data file %s", pfile);
  if ((f = fopen(pfile, "r")) == NULL)
    {
    fc_message("cannot open profile data file '%s'.", pfile);
    fc_message_fatal(FC_ERR_FOPEN, "check if file exists or if the name is correct.");
    }

  /** read general data **/
  /* the header */
  fc_debug("  reading headers...");
  fc_fscanf(f, "%1023s", temp);
  if (strcmp(temp, FC_CTX_HEADER) != 0)
    {
    fc_message("invalid header in profile data file '%s'.", pfile);
    fc_message("maybe this file is not a profile data file");
    fc_message_fatal(FC_ERR_HEADER, "  or is from an old version of FunctionCheck.");
    }
  /* the unique ID (not used here) */
  fc_fscanf(f, "%1023s", temp);
  /* the time mode used */
  fc_fscanf(f, "%d", &time_mode);
  /* the profile mode */
  fc_fscanf(f, "%d", &prof_mode);
  /* the processus ID */
  fc_fscanf(f, "%d", &id);
  /* the parent ID */
  fc_fscanf(f, "%d", &pid);
  /* the execution time */
  fc_fscanf(f, "%lld" , &total_time);

  /** the list of arcs **/
  /* number of elements */
  fc_fscanf(f, "%d", &fc_nb_list_of_arcs);
  fc_debug("  reading arcs (%d)...", fc_nb_list_of_arcs);
  if (fc_nb_list_of_arcs == 0)
    {/* may never occur */
    fc_message_fatal(FC_ERR_MEM, "empty list of arcs in profile data file!");
    }
  /* allocate data */
  fc_malloc(fc_list_of_arcs, fc_nb_list_of_arcs);
  if (fc_list_of_arcs == NULL)
    {
    fc_message_fatal(FC_ERR_MEM, "cannot allocate memory for arcs.");
    }
  /* read elements */
  for(i=0; i<fc_nb_list_of_arcs; i++)
    {
    fc_fscanf(f, "%p%p%d", &(fc_list_of_arcs[i].from),
              &(fc_list_of_arcs[i].to), &(fc_list_of_arcs[i].number));
    }

  /** the list of functions **/
  /* number of elements */
  fc_fscanf(f, "%d", &fc_nb_list_of_functions);
  fc_debug("  reading functions (%d)...", fc_nb_list_of_functions);
  if (fc_nb_list_of_functions == 0)
    {/* may never occur */
    fc_message_fatal(FC_ERR_MEM, "empty list of functions in profile data file!");
    }
  /* allocate data */
  fc_malloc(fc_list_of_functions, fc_nb_list_of_functions);
  if (fc_list_of_functions == NULL)
    {
    fc_message_fatal(FC_ERR_MEM, "cannot allocate memory for functions.");
    }
  /* read elements */
  for(i=0; i<fc_nb_list_of_functions; i++)
    {
    fc_fscanf(f, "%p%d%lld%lld%lld%lld%lld%lld",
                 &(fc_list_of_functions[i].symbol),
                 &(fc_list_of_functions[i].calls),
                 &(fc_list_of_functions[i].local_time),
                 &(fc_list_of_functions[i].total_time),
                 &(fc_list_of_functions[i].min_time),
                 &(fc_list_of_functions[i].max_time),
                 &(fc_list_of_functions[i].min_ltime),
                 &(fc_list_of_functions[i].max_ltime));
    fc_list_of_functions[i].name.name   = NULL;
    fc_list_of_functions[i].name.object = NULL;
    fc_list_of_functions[i].name.line   = 0;
    fc_list_of_functions[i].hide = 0;
    fc_list_of_functions[i].my_index = i;
    }

  /** the list of dynamic libraries **/
  /* number of elements */
  fc_fscanf(f, "%d", &fc_nb_list_of_lib);
  fc_debug("  reading dynamic objects (%d)...", fc_nb_list_of_lib);
  if (fc_nb_list_of_lib >= 0)
    {
    /* allocate data */
    fc_malloc(fc_list_of_lib, fc_nb_list_of_lib);
    if (fc_list_of_lib == NULL)
      {
      fc_message_fatal(FC_ERR_MEM, "cannot allocate memory for lib.");
      }
    /* read elements */
    for(i=0; i<fc_nb_list_of_lib; i++)
      {
      fc_fscanf(f, "%p%255s", &(fc_list_of_lib[i].address),
                        	fc_list_of_lib[i].name);
      }
    }

  /** read the list of memory leaks **/
  fc_fscanf(f, "%d", &fc_nb_list_of_leaks);
  fc_debug("  reading memory leaks (%d)...", fc_nb_list_of_leaks);
  /* for backward compatibility: end of file -> old version of
       profile data files, without memory informations. Just
       do nothing (treated as if no memory info are available) */
  if (!feof(f))
    {
    if (fc_nb_list_of_leaks > 0)
      {
      /* allocate data */
      fc_malloc(fc_list_of_leaks, fc_nb_list_of_leaks);
      if (fc_list_of_leaks == NULL)
	{
	fc_message("cannot allocate memory for memory leaks.");
	fc_message("  memory leaks data lost!");
	}
      else
        {
        /* read elements */
        for(i=0; i<fc_nb_list_of_leaks; i++)
	  {
	  /* read common elements */
	  fscanf(f, "%p%u%p%p", &(fc_list_of_leaks[i].pointer),
	                        &(fc_list_of_leaks[i].size),
	                        &(fc_list_of_leaks[i].alloc_place),
	                        &(fc_list_of_leaks[i].realloc_place));
	  /* now get the call-stack */
	  nbstack = fc_read_stack(f, fc_list_of_leaks[i].alloc_stack);
	  fc_malloc(fc_list_of_leaks[i].stack_name, nbstack);
	  }
	}
      }

    /** read list of invalid free **/
    fc_fscanf(f, "%d", &fc_nb_list_of_free);
    fc_debug("  reading invalid free (%d)...", fc_nb_list_of_free);
    if (fc_nb_list_of_free > 0)
      {
      /* allocate data */
      fc_malloc(fc_list_of_free, fc_nb_list_of_free);
      if (fc_list_of_free == NULL)
	{
	fc_message("cannot allocate memory for invalid free.");
	fc_message("  invalid free data lost!");
	}
      else
        {
        /* read elements */
        for(i=0; i<fc_nb_list_of_free; i++)
	  {
	  /* read common elements */
	  fscanf(f, "%p%p", &(fc_list_of_free[i].free_place),
	                    &(fc_list_of_free[i].pointer));
	  /* now get the call-stack */
	  nbstack = fc_read_stack(f, fc_list_of_free[i].free_stack);
	  fc_malloc(fc_list_of_free[i].stack_name, nbstack);
	  }
	}
      }

    /** read list of invalid realloc **/
    fc_fscanf(f, "%d", &fc_nb_list_of_realloc);
    fc_debug("  reading invalid realloc (%d)...", fc_nb_list_of_realloc);
    if (fc_nb_list_of_realloc > 0)
      {
      /* allocate data */
      fc_malloc(fc_list_of_realloc, fc_nb_list_of_realloc);
      if (fc_list_of_realloc == NULL)
	{
	fc_message("cannot allocate memory for invalid realloc.");
	fc_message("  invalid realloc data lost!");
	}
      else
        {
        /* read elements */
        for(i=0; i<fc_nb_list_of_realloc; i++)
	  {
	  /* read common elements */
	  fscanf(f, "%p%p", &(fc_list_of_realloc[i].realloc_place),
	                    &(fc_list_of_realloc[i].pointer));
	  /* now get the call-stack */
	  nbstack = fc_read_stack(f, fc_list_of_realloc[i].realloc_stack);
	  fc_malloc(fc_list_of_realloc[i].stack_name, nbstack);
	  }
	}
      }
    }
  /* if no data, switch to no_memory */
  if ((fc_nb_list_of_realloc == 0)&&(fc_nb_list_of_free == 0)
                                  &&(fc_nb_list_of_leaks == 0))
    no_memory = 1;

  /** ok. close file **/
  fc_debug("done.");
  fclose(f);


  /** treat data **/

  /** solve symbols name  **/
  fc_debug("computing names...");
  if (!no_names)
    if (!fc_names_solve(force_names,
                        fc_nb_list_of_functions, fc_list_of_functions,
                	fc_nb_list_of_lib, fc_list_of_lib,
			fc_nb_list_of_arcs, fc_list_of_arcs,
                	efile, demangle, style,
                        only_list, nb_only_list,
                        not_list, nb_not_list,
                        no_memory||no_memory_leaks?0:fc_nb_list_of_leaks,
                        no_memory||no_memory_leaks?NULL:fc_list_of_leaks,
                        no_memory||no_memory_free?0:fc_nb_list_of_free,
                        no_memory||no_memory_free?NULL:fc_list_of_free,
                        no_memory||no_memory_realloc?0:fc_nb_list_of_realloc,
                        no_memory||no_memory_realloc?NULL:fc_list_of_realloc))
      {
      fc_message("WARNING: error while solving names.");
      fc_message("         names will not be available.");
      }

  /* sort the list of functions */
  fc_debug("sorting functions...");
  qsort(fc_list_of_functions, fc_nb_list_of_functions,
        sizeof(FC_Function), to_sort);

  /** compute the call-graph **/
  fc_debug("computing call-graph...");
  if (!fc_graph_create(&fc_nb_list_of_arcs, fc_list_of_arcs,
                       fc_nb_list_of_functions, fc_list_of_functions,
                       only_list, nb_only_list,
                       not_list, nb_not_list,
                       do_propagate, do_rpropagate))
    {
    fc_message("removing call-graph features.");
    no_call_graph = 1;
    }
  fc_debug("ok.");

  /** if requested, output the call-graph in order to be display
      with software 'CVG' (a GPL graph visualizer) **/
  if (show_callgraph)
    {
    printf("graph: {\n");
    printf(" orientation: left_to_right\n");
    /* display nodes */
    for(i=0; i<fc_nb_list_of_functions; i++)
      {
      printf("  node: { title: \"%d\" label: \"%s\" borderwidth:0}\n",
                 i, fc_list_of_functions[i].name.name);
      }
    /* display edges */
    for(i=0; i<fc_nb_list_of_functions; i++)
      {
      j = 0;
      while(((FC_Node*)(fc_list_of_functions[i].node))->nexts[j] != NULL)
        {
        printf("  edge: { sourcename: \"%d\" targetname: \"%d\" thickness: 1}\n",
                    i, ((FC_Node*)(fc_list_of_functions[i].node))->nexts[j]->function->my_index);
	j++;
	}
      }
    printf("    }\n");

    /* I should add memory cleanup here */
    return(FC_ERR_OK);
    }

  /** compute how many functions are hidden **/
  nb_hide = 0;
  for(i=0; i<fc_nb_list_of_functions; i++)
      {
      if (fc_list_of_functions[i].hide)
        nb_hide++;
      }

  /** display known informations **/
  if (!no_header)
    {
    printf("\n%s V%s by Y.Perret\n\n", FC_PACKAGE, FC_VERSION);
    }

  if (!no_general)
    {
    printf("Execution profile for program '%s'\n", efile);
    printf("Time mode used is: ");
    if (time_mode == FC_MTIME_EXT)
      printf("clock time\n");
    else
    if (time_mode == FC_MTIME_CPU)
      printf("CPU time\n");
    else
    if (time_mode == FC_MTIME_SYS)
      printf("SYSTEM time\n");
    else
      printf("unknown\n");
    printf("Profile mode is: ");
    if (prof_mode == FC_MODE_SINGLE)
      printf("single process\n");
    else
    if (prof_mode == FC_MODE_FORK)
      printf("forks allowed\n");
    else
    if (prof_mode == FC_MODE_THREAD)
      printf("threads allowed\n");
    else
      printf("unknown\n");
    printf("ID of this processus is %d\n", id);
    printf("Total time spend in this processus is %f\n", total_time/1000000.);

    printf("\n\n");
    printf("%d arc(s), %d function(s) (%d shown, %d hidden), %d library(ies)\n", fc_nb_list_of_arcs,
      fc_nb_list_of_functions, fc_nb_list_of_functions-nb_hide,
      nb_hide, fc_nb_list_of_lib);
    printf("\n\n");
    }


  /** display flat profile **/
  if (!no_flat_profile)
    {
    printf("     total   |     local   %s|     total     |     local     |   #   |function\n"
           " time  |  %%  | time  |  %%  %s|  min  |  max  |  min  |  max  | calls |  name\n"
           "-------|-----|-------|-----%s|-------|-------|-------|-------|-------|--------\n",
	   cumul?"      ":"",
	   cumul?"|cum %":"",
	   cumul?"|-----":"");
    for(i=0; i<fc_nb_list_of_functions; i++)
      {
      if (!fc_list_of_functions[i].hide)
        {
        fc_convert_name(&(fc_list_of_functions[i]), temp, draw_details, use_basename);
        total_cumul += fc_list_of_functions[i].local_time;
        if (cumul)
	  {
	  sprintf(scum, "|%5.1f", (100. * ((total_cumul / 1000000.) /
	               (total_time / 1000000.) )));
	  }
        printf("%7.2f|%5.1f|%7.2f|%5.1f%s|%7.2f|%7.2f|%7.2f|%7.2f|%7d| %s\n",
               fc_list_of_functions[i].total_time / 1000000.,
	       (100. * ((fc_list_of_functions[i].total_time / 1000000.) /
	               (total_time / 1000000.) )),
	       fc_list_of_functions[i].local_time / 1000000.,
	       (100. * ((fc_list_of_functions[i].local_time / 1000000.) /
	               (total_time / 1000000.) )),
	      cumul?scum:"",
	      fc_list_of_functions[i].min_time / 1000000.,
	      fc_list_of_functions[i].max_time / 1000000.,
	      fc_list_of_functions[i].min_ltime / 1000000.,
	      fc_list_of_functions[i].max_ltime / 1000000.,
	      fc_list_of_functions[i].calls,
	      temp);
        }
      }
    printf("\n");
    }

  /** display the call-graph **/
  if (!no_call_graph)
    {
    printf("\nCall-graph:\n\n");
    
    /* for each function */
    for(i=0; i<fc_nb_list_of_functions; i++)
      {
      if (!fc_list_of_functions[i].hide)
        {
        fc_convert_name(&(fc_list_of_functions[i]), temp, draw_details, use_basename);
        if (call_called)
          printf("'%s' called by:\n", temp);
        else
          printf("'%s' calls:\n", temp);

        /* for each child */
        j = 0;
        rj = 0;
	if (fc_list_of_functions[i].node != NULL)
	  {
          if (call_called)
	    while(((FC_Node*)(fc_list_of_functions[i].node))->prevs[j] != NULL)
	      {
              if (!(((FC_Node*)(fc_list_of_functions[i].node))->prevs[j]->function->hide))
        	{
		fc_convert_name(((FC_Node*)(fc_list_of_functions[i].node))->prevs[j]->function,
	                	temp, draw_details, use_basename);
		printf(" (%d) %s", ((FC_Node*)(fc_list_of_functions[i].node))->nprevs[j], temp);
        	rj++;
        	}
	      j++;
	      }
          else
            while(((FC_Node*)(fc_list_of_functions[i].node))->nexts[j] != NULL)
	      {
              if (!(((FC_Node*)(fc_list_of_functions[i].node))->nexts[j]->function->hide))
        	{
		fc_convert_name(((FC_Node*)(fc_list_of_functions[i].node))->nexts[j]->function,
	                	temp, draw_details, use_basename);
		printf(" (%d) %s", ((FC_Node*)(fc_list_of_functions[i].node))->nnexts[j], temp);
        	rj++;
        	}
	      j++;
	      }
          if (rj == 0)
            {
            printf("  nobody\n");
	    }
          else
            printf("\n");
          }
	else
	  { /* argl! this function exists in the list of functions,
	       but no arcs comes to it! don't know WHY it happends,
	       but in some case it happends... so this test prevent crashes */
	  printf("  this function is not in the call-graph!\n");
	  printf("  this message is a protection against a known bug\n");
	  }
        printf("\n");
        }
      }
    }

  /** cycles **/
  if (!no_cycles)
    {
    printf("Detected cycle(s):\n\n");
    if (!fc_compute_cycles(0)) /* this '0' is an unused option flag */
      printf("No cycles.\n");
    printf("\n");
    }


  /** memory informations **/
  if (!no_memory)
    {
    /** memory leaks **/
    if (!no_memory_leaks)
      {
      printf("Memory leaks detected:\n\n");
      for(i=0; i<fc_nb_list_of_leaks; i++)
        {
	printf("Block %p of original size %u\n",
	     fc_list_of_leaks[i].pointer,
	     fc_list_of_leaks[i].size);
	printf("  allocated at %p\n", fc_list_of_leaks[i].alloc_place);
	if (fc_list_of_leaks[i].realloc_place != NULL)
	  printf("  reallocated at %p\n",
	          fc_list_of_leaks[i].realloc_place);
	printf("  allocation call-stack was:\n");
	printf("    ");
	fc_print_stack(fc_list_of_leaks[i].alloc_stack,
	               fc_list_of_leaks[i].stack_name);
	printf("  was never freed.\n\n");
	}
      if (fc_nb_list_of_leaks == 0)
        printf("none.\n");
      printf("\n");
      }
    
    /** invalid free **/
    if (!no_memory_free)
      {
      printf("Invalid free detected:\n\n");
      for(i=0; i<fc_nb_list_of_free; i++)
        {
	printf("Call to 'free' with unreferenced pointer %p\n",
	               fc_list_of_free[i].pointer);
        printf("  at %p\n", fc_list_of_free[i].free_place);
	printf("  free call-stack was:\n");
	printf("    ");
	fc_print_stack(fc_list_of_free[i].free_stack,
	               fc_list_of_free[i].stack_name);
	printf("\n");
	}
      if (fc_nb_list_of_free == 0)
        printf("none.\n");
      printf("\n");
      }

    /** invalid realloc **/
    if (!no_memory_realloc)
      {
      printf("Invalid realloc detected:\n\n");
      for(i=0; i<fc_nb_list_of_realloc; i++)
        {
	printf("Call to 'realloc' with unreferenced pointer %p\n",
	               fc_list_of_realloc[i].pointer);
        printf("  at %p\n", fc_list_of_realloc[i].realloc_place);
	printf("  realloc call-stack was:\n");
	printf("    ");
	fc_print_stack(fc_list_of_realloc[i].realloc_stack,
	               fc_list_of_realloc[i].stack_name);
	printf("\n");
	}
      if (fc_nb_list_of_realloc == 0)
        printf("none.\n");
      printf("\n");
      }
    }



  /** freed all data **/
  fc_debug("freeing data...");
  if (!no_call_graph)
    fc_graph_delete(fc_nb_list_of_arcs, fc_list_of_arcs,
                    fc_nb_list_of_functions, fc_list_of_functions);
  if (fc_list_of_arcs != NULL)
    free(fc_list_of_arcs);
  if (fc_list_of_functions != NULL)
    free(fc_list_of_functions);
  if (fc_list_of_lib != NULL)
    free(fc_list_of_lib);

  /* end */
  fc_debug("exit.");
  return(FC_ERR_OK);
}
