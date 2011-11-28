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

/* fc_global.h: common definitions and values.
   WARNING: this file is used here and in ../dump/  */

#ifndef __fc_global_h_
#define __fc_global_h_


/** names **/
#define FC_PROFILER_NAME   "FCheck"
#define FC_MANAGER_NAME    "FCMng"
#define FC_DMANAGER_NAME   "FCDMng"

/** names for outputs **/
#ifndef PACKAGE
#define FC_PACKAGE "FunctionDump"
#else
#define FC_PACKAGE PACKAGE
#endif
#ifndef VERSION
#define FC_VERSION "3.0"
#else
#define FC_VERSION VERSION
#endif


/** files **/
#define FC_DUMP_FILE    "functioncheck"


/** headers **/
#define FC_CTX_HEADER   "functioncheck_v3.0"


/** version **/
#define FC_FC_VERSION   "3.0"
#define FC_FC_MAJOR     "3"
#define FC_FC_MINOR     "0"



/** modes for data structures **/
#define FC_MODE_SINGLE   1
#define FC_MODE_FORK     2
#define FC_MODE_THREAD   3


/** time modes **/
#define FC_MTIME_EXT  1
#define FC_MTIME_CPU  2
#define FC_MTIME_SYS  2
#define FC_MTIME_TSC  3
#define FC_MTIME_NDEF 0


/** exit status **/
#define FC_ERR_FIRST   0
#define FC_ERR_OK      0
#define FC_ERR_EXEC    1
#define FC_ERR_FORK    2
#define FC_ERR_MEM     3
#define FC_ERR_PIPE    4
#define FC_ERR_READ    5
#define FC_ERR_WRITE   6
#define FC_ERR_READ    5
#define FC_ERR_CLOSE   6
#define FC_ERR_THREAD  7
#define FC_ERR_TIME    8
#define FC_ERR_OTHER   9
#define FC_ERR_ARGS    10
#define FC_ERR_HASH    11
#define FC_ERR_STACK   12
#define FC_ERR_FOPEN   13
#define FC_ERR_HEADER  14
#define FC_ERR_EOF     15
#define FC_ERR_OVERF   16
#define FC_ERR_LAST    16


#endif /* __fc_global_h_ */
