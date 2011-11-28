bin_PROGRAMS = simple_test simple_test_calib simple_test_fork \
        simple_test_thread simple_test_lib simple_test_mem

AM_CFLAGS = -g -finstrument-functions
AM_CXXFLAGS = -g -finstrument-functions
AM_LDFLAGS = -ldl -lpthread

COMMONLIB = ../src/libfc/libfc.la

simple_test_SOURCES = simple_test.c
simple_test_LDADD = $(COMMONLIB)

simple_test_mem_SOURCES = simple_test_mem.c
simple_test_mem_LDADD = $(COMMONLIB)

simple_test_calib_SOURCES = simple_test_calib.c
simple_test_calib_LDADD = $(COMMONLIB)

simple_test_fork_SOURCES = simple_test_fork.c
simple_test_fork_LDADD = $(COMMONLIB)

simple_test_thread_SOURCES = simple_test_thread.c
simple_test_thread_LDADD = $(COMMONLIB)

simple_test_lib_SOURCES = simple_test_lib.c
simple_test_lib_LDADD = -lm -lsimple_test_lib2 $(COMMONLIB)

lib_LTLIBRARIES = libsimple_test_lib2.la

libsimple_test_lib2_la_SOURCES = \
        simple_test_lib2.c

libsimple_test_lib2_la_LDFLAGS = -nostartfiles



