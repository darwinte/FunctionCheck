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

/** includes **/
#include "fc_tools.h"
#include "fc_global.h"
#include <string.h>


/** table of strings for errors **/
char *fc_error_str[] = {
  "OK", "'exec*' error", "'fork' error", "memory error",
  "'pipe' error", "reading error", "writing error",
  "'close' error", "thread error", "time error",
  "undefined error",
  "bad arguments", "hash table error", "stack error",
  "fopen error", "bad header", "unexpected end of file",
  "overflow error",
};
char *fc_error_ndef = "bad error number";


/** the FILE where to write messages/errors (default: stderr) **/
FILE *fc_message_stream = NULL;
/* switch for the stream */
#define FC_STREAM (fc_message_stream==NULL?stderr:fc_message_stream)

/** the name used for functioncheck itself (in messages) **/
char *fc_lib_name = "FCHECK";

/** flag to forbid messages **/
int fc_allow_messages = 1;

/** flag for verbose mode (debug) **/
int fc_allow_debug = 0;

/* flag to validate the debug */
#ifndef FC_NO_DEBUG
int fc_allow_debug_hard=1;
#else
int fc_allow_debug_hard=0;
#endif


/** functions **/


/** strings for exit status **/
char *fc_strerror(int err)
{
  if ((err < FC_ERR_FIRST)||(err > FC_ERR_LAST))
    return(fc_error_ndef);
  return(fc_error_str[err]);
}


/* set the message stream */
void fc_set_message_stream(FILE*f)
{
  fc_message_stream = f;
}


/* set the message name */
void fc_set_message_name(char *name)
{
  char *tmp;  /* sorry, old name is not deleted */

  tmp = strdup(name);

  if (tmp != NULL)
    fc_lib_name = tmp;
}


/* mode==true: allow messages */
void fc_set_message_mode(int mode)
{
  fc_allow_messages = mode;
}

/* set debug mode */
void fc_set_debug_mode(int mode)
{
  fc_allow_debug = mode;
}


/* write a message */
void fc_message(char *format, ...)
{
  va_list args;
  char buffer[1024];

  if (fc_allow_messages)
    {
    va_start(args, format);
    fprintf(FC_STREAM, "%s: ", fc_lib_name);
    vsnprintf(buffer, 1023, format, args);
    fprintf(FC_STREAM, buffer);
    fprintf(FC_STREAM, "\n");
    va_end(args);
    }
}

/* write a debug message */
void fc_rdebug(char *format, ...)
{
  va_list args;
  char buffer[1024];

  if (!fc_allow_debug)
    return;

  if (fc_allow_messages)
    {
    va_start(args, format);
    fprintf(FC_STREAM, "%s:DEBUG: ", fc_lib_name);
    vsnprintf(buffer, 1023, format, args);
    fprintf(FC_STREAM, buffer);
    fprintf(FC_STREAM, "\n");
    va_end(args);
    }
}

void fc_message_fatal(int ret, char *format, ...)
{
  va_list args;
  char buffer[1024];

  if (fc_allow_messages)
    {
    va_start(args, format);
    fprintf(FC_STREAM, "%s: ", fc_lib_name);
    vsnprintf(buffer, 1023, format, args);
    fprintf(FC_STREAM, buffer);
    fprintf(FC_STREAM, "\n");
    va_end(args);
    /* exit with return code */
    fprintf(FC_STREAM, "FATAL (%d)!\n", ret);
    }
  exit(ret);
}


