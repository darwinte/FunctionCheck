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

/* fc_tools: various tools for the lib part of functioncheck
             in particular for outputs
*/

#ifndef __fc_tools_h_
#define __fc_tools_h_


/** includes **/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/** the FILE where to write messages/errors (default: stderr) **/
extern FILE *fc_message_stream;
/* allow to set the output stream */
void fc_set_message_stream(FILE*f);


/** strings for exit status **/
char *fc_strerror(int err);

/** flag to indicate if debug is available */
extern int fc_allow_debug_hard;


/** the name used for functioncheck itself (in messages) **/
extern char *fc_lib_name;
/* allow to set the lib name */
void fc_set_message_name(char *name);
/* mode==true: allow messages */
void fc_set_message_mode(int mode);
/* set debug mode */
void fc_set_debug_mode(int mode);


/** message functions **/
void fc_message(char *format, ...);
void fc_message_fatal(int ret, char *format, ...);
void fc_rdebug(char *format, ...);
#ifndef FC_NO_DEBUG
#define fc_debug fc_rdebug
#else
#define fc_debug(args...)  {}
#endif


/** allocation functions **/
#define fc_malloc(var, nbel) \
  ((var) = malloc((nbel)*(sizeof(*(var)))))
#define fc_realloc(var, nbel) \
  ((var) = realloc((var),(nbel)*(sizeof(*(var)))))



#endif /*__fc_tools_h_*/
