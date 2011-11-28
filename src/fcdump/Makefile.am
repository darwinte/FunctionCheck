bin_PROGRAMS = fcdump

#TODO: add library check to configure script
AM_LDFLAGS = -lbfd

fcdump_SOURCES = \
	fc_dump.c  fc_graph.c  fc_tools.c \
        fc_dump.h  fc_graph.h  fc_tools.h \
        fc_names.c fc_names.h  fc_global.h \
	demangle.h

if DEBUG
  AM_CFLAGS = -Wall -O3 -I../include -DFC_ALLOW_DEBUG
else
  AM_CFLAGS = -Wall -O3 -I../include -DFC_NO_DEBUG
endif
