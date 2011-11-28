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
/** fc_names.c:  **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "demangle.h"
#include "fc_names.h"


/** static string to prevent 'null' names **/
#define FC_INVALID_NAME  "<error>"


/** symbol list for low-level access to BFD data (local) **/
typedef struct
{
  char *name;
  void *addr;
  int flag;  /* an exported function */
}FC_Sym;
/* local table to treat symbols */
FC_Sym *fc_syms = NULL;
int fc_nb_syms  = 0;


/* open a binary object using BFD */
int fc_open_bfd_file(char *name, bfd **core_bfd, int *core_num_syms,
                     asection **core_text_sect, asymbol ***core_syms,
		     int *core_min_insn_size, int *core_offset_to_code)
{
  *core_bfd = bfd_openr(name, 0);
  if (!(*core_bfd))
    {
    fc_message("cannot open file %s", name);
    return(0);
    }

  /* check format */
  if (!bfd_check_format(*core_bfd, bfd_object))
    {
    fc_message("file '%s' is not an object", name);
    return(0);
    }

  /* get TEXT section */ /* ".text" */
  *core_text_sect = bfd_get_section_by_name(*core_bfd, ".text");
  if (!(*core_text_sect))
    {
    fc_message("BFD: trying $CODE$ section instead of .text");
    *core_text_sect = bfd_get_section_by_name (*core_bfd, "$CODE$");
    if (!(*core_text_sect))
      {
      fc_message("no TEXT section in object '%s'", name);
      return(0);
      }
    }

  /* read symbol table */
  *core_num_syms = bfd_get_symtab_upper_bound(*core_bfd);
  if (*core_num_syms < 0)
    {
    fc_message("bfd error: %s", bfd_errmsg(bfd_get_error()));
    return(0);
    }

  *core_syms = (asymbol **) malloc(sizeof(asymbol*)*(*core_num_syms));
  if (*core_syms == NULL)
    {
    fc_message("cannot allocate %d bytes for symbols", (int)sizeof(asymbol*)*(*core_num_syms));
    fc_message_fatal(FC_ERR_MEM, "treatments aborted");
    }

  *core_num_syms = bfd_canonicalize_symtab(*core_bfd, *core_syms);
  if (*core_num_syms < 0)
    {
    free(*core_syms);
    fc_message("bfd error: %s", bfd_errmsg(bfd_get_error()));
    return(0);
    }

  *core_min_insn_size = 1;
  *core_offset_to_code = 0;
  switch (bfd_get_arch(*core_bfd))
    {
    case bfd_arch_vax:
    case bfd_arch_tahoe:
      *core_offset_to_code = 2;
      break;
    case bfd_arch_alpha:
      *core_min_insn_size = 4;
      break;
    default:
      break;
    }

  /* ok */
  return(1);
}

/* close a binary object open with BFD */
int fc_close_bfd_file(bfd *core_bfd, int core_num_syms,
                     asection *core_text_sect, asymbol **core_syms,
		     int core_min_insn_size, int core_offset_to_code)
{
  bfd_close(core_bfd);
  free(core_syms);
  
  return(1);
}


/* sort  sym. table by address */
int fc_compare_pointers(const void *e1, const void *e2)
{
  if (((FC_Sym*)e1)->addr > ((FC_Sym*)e2)->addr)
    return(1);
  if (((FC_Sym*)e1)->addr < ((FC_Sym*)e2)->addr)
    return(-1);
  return(0);
}

/* init symbol table from BFD data */
int fc_init_extract_dynamic(int core_num_syms, asymbol **core_syms)
{
  int i, dest;

  if ((fc_syms = malloc(sizeof(FC_Sym)*core_num_syms)) == NULL)
    {
    fc_message("cannot allocate local symbol table");
    return(0);
    }
  /* put addr/names in table */
  dest = 0;
  for(i=0; i<core_num_syms; i++)
    {
    /* only functions */
    if (core_syms[i]->flags & BSF_FUNCTION)
      {
      fc_syms[dest].name = (char*)bfd_asymbol_name(core_syms[i]);
      fc_syms[dest].addr = (void*)bfd_asymbol_value(core_syms[i]);
      if ((core_syms[i]->flags & BSF_GLOBAL)||(core_syms[i]->flags & BSF_EXPORT))
        fc_syms[dest].flag = 1;
      else
        fc_syms[dest].flag = 0;
      dest++;
      }
    }

  fc_nb_syms = dest;

  /* empty table */
  if (fc_nb_syms == 0)
    {
    return(0);
    }

  /* sort it by pointer value */
  qsort(fc_syms, fc_nb_syms, sizeof(FC_Sym), fc_compare_pointers);

  return(1);
}

/* freed the local sym. table */
int fc_fini_extract_dynamic()
{
  if (fc_syms != NULL)
    free(fc_syms);
  fc_syms = NULL;
  fc_nb_syms = 0;

  return(1);
}

/* returns a name (char* to duplicate) from an address */
/* valid is true if it is a local name */
char *fc_extract_dynamic(void *addr, int *valid)
{
  int i;
  char tmp[1024], *buffer = tmp;

  *valid  = 0;
  if (fc_syms == NULL)
    return("<err0>");
  if (addr < fc_syms[0].addr)
    {
    sprintf(buffer, "<lo-%p>", addr);
    return((char*)buffer);  /* before first symbol */
    }
  if (addr > fc_syms[fc_nb_syms-1].addr)
    {
    sprintf(buffer, "<hi-%p>", addr);
    return((char*)buffer);  /* after last symbol */
    }

  /* I may use a better search method :o) */
  for(i=0; i<fc_nb_syms; i++)
    {
    /* only check EXACT addresses */
    if (addr == fc_syms[i].addr)
      {
      *valid = fc_syms[i].flag;
      return(fc_syms[i].name);
      }
    }
  /* the exact entry does not exist */
  return(NULL);
}

/* return true if 'name' is present in the sym-table AND
   is exported. Gives the corresponding address for get_nearest_line */
int fc_find_symbol_by_name(char *name, void **addr, int demangle, int style)
{
  int i;
  char *ntemp;

  *addr = NULL;

  if (name == NULL)
    return(0);

  /* loop for this name */
  for(i=0; i<fc_nb_syms; i++)
    {
    /* demangle it if needed (for a coherent strcmp) */
    if (demangle)
      {
      ntemp = cplus_demangle(fc_syms[i].name, style);
      if (ntemp == NULL)
	ntemp = fc_syms[i].name; /* use the basic name else */
      }
    else
      {
      ntemp = fc_syms[i].name; /* no demangle, use the basic name */
      }

    if ((ntemp[0] == name[0])&&(strcmp(ntemp, name) == 0))
      {/* found. check for the export status */
      if (fc_syms[i].flag)
        {/* ok */
	*addr = fc_syms[i].addr;
	return(1);
	}
      else
        {/* exit. it is not possible to get the same name twice (really?) */
	return(0);
	}
      }
    }
  /* no match at all */
  return(0);
}

/* returns the function pointer from a pointer inside the function */
void *fc_extract_dynamic_base(void *addr)
{
  int i;

  /* !?! */
  if (fc_syms == NULL)
    return(NULL);

  /* out of the bounds */
  if ((addr < fc_syms[0].addr)||(addr > fc_syms[fc_nb_syms-1].addr))
    {/* no changes */
    return(addr);
    }

  /* I may use a better search method :o) */
  for(i=0; i<fc_nb_syms-1; i++)
    {
    if ((addr >= fc_syms[i].addr)&&(addr < fc_syms[i+1].addr))
      {
      return(fc_syms[i].addr);
      }
    }

  /* may not occur */
  return(addr);
}

/* return the symbol address of the given function */
void *fc_search_function_by_name(char *name, int nbf, FC_Function *fncs)
{
  int i;

  for(i=0; i<nbf; i++)
    {
    if ((fncs[i].name.name != NULL)&&(fncs[i].name.name[0] == name[0])&&
        (strcmp(fncs[i].name.name, name) == 0))
      {
      return(fncs[i].symbol);
      }
    }
  /* not found */
  return(NULL);
}

/* return true if the given address exist */
int fc_check_address(void *addr, int nbf, FC_Function *fncs)
{
  int i;

  for(i=0; i<nbf; i++)
    {
    if (fncs[i].symbol == addr)
      return(1);
    }
  /* not found */
  return(0);
}

int fc_names_solve(int force_names,
                   int nb_fnc, FC_Function *fncs,
                   int nb_lib, FC_LDyn *dyns,
		   int nb_arcs, FC_Arc *arcs,
                   char *exec_name, int demangle, int style,
                   FC_NSym *lonly, int nb_only,
                   FC_NSym *lnot, int nb_not,
                   int nb_leaks, FC_MLeak *leaks,
                   int nb_free, FC_MFree *free,
                   int nb_realloc, FC_MRealloc *realloc)
{
  /* BFD data for exec */
  bfd *core_bfd;
  int core_min_insn_size;
  int core_offset_to_code;
  asection *core_text_sect;
  int core_num_syms;
  asymbol **core_syms;
  /* various variables */
  int i, j, valid;
  char *tname, *_tname, *tobj, *ttt;
  int tline;
  int missing = 0;
  void *addr;

  fc_debug("Solving names...");

  /* first init the list of functions names */
  for(i=0; i<nb_fnc; i++)
    {
    fncs[i].name.name   = NULL;
    fncs[i].name.object = NULL;
    fncs[i].name.line   = 0;
    fncs[i].name.ok     = FC_OK_NDEF;
    }

  /* open the executable */
  fc_debug("  opening executable '%s'", exec_name);
  if (!fc_open_bfd_file(exec_name, &core_bfd, &core_num_syms,
                        &core_text_sect, &core_syms,
			&core_min_insn_size, &core_offset_to_code))
    {
    fc_message("The executable name given (%s) is not a valid object.", exec_name);
    if (!force_names)
      {
      fc_message_fatal(FC_ERR_ARGS, "Please check your parameters (try '--help' option)");
      }
    else
      {
      fc_message("Names will not be available (check your parameters)");
      }
    return(0);
    }
  if (!fc_init_extract_dynamic(core_num_syms, core_syms))
    {
    fc_message("names treatments aborted.");/* close bfd object */
    fc_close_bfd_file(core_bfd, core_num_syms,
                      core_text_sect, core_syms,
		      core_min_insn_size, core_offset_to_code);
    return(0);
    }

  /* for each arc replace call-site by the corresponding function */
  fc_debug("  cleaning arcs..");
  for(i=0; i<nb_arcs; i++)
    {
    arcs[i].from = fc_extract_dynamic_base(arcs[i].from);
    }

  /* for each entry try to extract data */
  fc_debug("  extracting names...");
  for(i=0; i<nb_fnc; i++)
    {
    /* get the name with our method: this is due to the fact
         that get_nearest_line failed to extract the name of a
	 function that comes from a dynamic object */
    tname = fc_extract_dynamic(fncs[i].symbol, &valid);

    /* if the name is local, try to get file:line */
    if (valid)
      {
      /* get name/file/line using BFD */
      bfd_find_nearest_line(core_bfd, core_text_sect, core_syms,
			    (bfd_vma)fncs[i].symbol-core_text_sect->vma,
			    (const char**)&tobj,
                            (const char**)&_tname,
                            (unsigned int *)&tline);
      }
    else
      {
      tobj = NULL;
      tline = 0;
      }

    fncs[i].name.object = tobj;
    fncs[i].name.line   = tline;
    if (valid)
      {
      fncs[i].name.ok = FC_OK_OK;
      }
    else
      {
      fncs[i].name.ok = FC_OK_NDEF;
      missing = 1;
      }

    if (tname == NULL)
      {
      ttt = NULL;
      }
    else
      {
      if (demangle)
	{
	ttt = cplus_demangle(tname, style);
	if (ttt == NULL)
	  ttt = strdup(tname);
	else
	  {
	  tname = strdup(ttt);
	  ttt = tname;
	  }
	}
      else
	{
	ttt = strdup(tname);
	}
      }

    fncs[i].name.name = (ttt==NULL?FC_INVALID_NAME:ttt);
    if (ttt == NULL)
      {
      fncs[i].name.ok = FC_OK_BAD;  /* mem error: not ok but not solvable */
      }
    }


  /* close our special treatments */
  fc_fini_extract_dynamic();

  /* close bfd object */
  fc_close_bfd_file(core_bfd, core_num_syms,
                    core_text_sect, core_syms,
		    core_min_insn_size, core_offset_to_code);

  /* now try to extract informations from dynamic libraries
     if some details are missing */
  if (missing)
    {
    fc_debug("  still missing details");
    for(i=0; i<nb_lib; i++)
      {
      /* open the ith lib */
      fc_debug("  trying object '%s'", dyns[i].name);
      if (!fc_open_bfd_file(dyns[i].name, &core_bfd, &core_num_syms,
                            &core_text_sect, &core_syms,
			    &core_min_insn_size, &core_offset_to_code))
        {
        fc_debug("  skipping library '%s' (bfd open error)", dyns[i].name);
        continue;
        }

      if (!fc_init_extract_dynamic(core_num_syms, core_syms))
        {
	/* close the BFD file */
        fc_close_bfd_file(core_bfd, core_num_syms,
                          core_text_sect, core_syms,
		          core_min_insn_size, core_offset_to_code);
	fc_debug("  skipping library '%s' (empty symbol table)", dyns[i].name);
	continue;
	}

      /* for each invalid symbol, try to extract it in this lib */
      for(j=0; j<nb_fnc; j++)
        {
	if (fncs[j].name.ok == FC_OK_NDEF)
	  {
	  /* check if this symbol is here and exported */
	  if (fc_find_symbol_by_name(fncs[j].name.name, &addr, demangle, style))
	    {
	    /* get file/line using BFD */
	    bfd_find_nearest_line(core_bfd, core_text_sect, core_syms,
				  (bfd_vma)addr-core_text_sect->vma,
				  (const char**)&tobj,
                        	  (const char**)&_tname,
                        	  (unsigned int *)&tline);
	    /* add this information in the symbol */
	    fncs[j].name.object = strdup(tobj);
	    fncs[j].name.line   = tline;
	    fncs[j].name.ok     = FC_OK_OK;
	    }
	  else
	    {/* still some job to do */
	    missing = 1;
	    }
	  }
	}


      fc_fini_extract_dynamic();
      /* close the BFD file */
      fc_close_bfd_file(core_bfd, core_num_syms,
                        core_text_sect, core_syms,
		        core_min_insn_size, core_offset_to_code);

      /* no more ? */
      if (!missing)
        break;
      }
    }

  /* if any, parse not/only lists to convert names to addresses */
  for(i=0; i<nb_only; i++)
    {
    if ((lonly[i].addr == NULL)&&(lonly[i].name != NULL))
      {
      lonly[i].addr = fc_search_function_by_name(lonly[i].name, nb_fnc, fncs);
      if (lnot[i].addr == NULL)
        {
        fc_message("function '%s' not found. discarded.", lnot[i].name);
        }
      }
    else
    if (lnot[i].addr != NULL)
      {
      if (!fc_check_address(lnot[i].addr, nb_fnc, fncs))
        {
        fc_message("address %p not found. discarded.", lnot[i].addr);
        lnot[i].addr = NULL;
        }
      }
    }
  for(i=0; i<nb_not; i++)
    {
    if ((lnot[i].addr == NULL)&&(lnot[i].name != NULL))
      {
      lnot[i].addr = fc_search_function_by_name(lnot[i].name, nb_fnc, fncs);
      if (lnot[i].addr == NULL)
        {
        fc_message("function '%s' not found. discarded.", lnot[i].name);
        }
      }
    else
    if (lnot[i].addr != NULL)
      {
      if (!fc_check_address(lnot[i].addr, nb_fnc, fncs))
        {
        fc_message("address %p not found. discarded.", lnot[i].addr);
        lnot[i].addr = NULL;
        }
      }
    }

  fc_debug("End of name extraction.");
  return(1);
}

