lib_LTLIBRARIES = libfc.la

#TODO: add library check to configure script
AM_LDFLAGS = -ldl -lpthread

AM_CFLAGS = -g -I../include
AM_CXXFLAGS = -g -I../include

libfc_la_SOURCES = \
	fc_check.c fc_tools.c fc_time.c fc_ressources.c fc_com.c \
	fc_memory.c fc_fifo.c fc_semaphore.c \
                   fc_tools.h fc_time.h fc_ressources.h fc_com.h \
	fc_memory.h fc_fifo.h fc_semaphore.h

libfc_la_LDFLAGS = -Xcompiler -nostartfiles
