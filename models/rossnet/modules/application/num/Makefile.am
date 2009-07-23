INCLUDES=-I$(top_srcdir)

AM_CFLAGS=-g -Wall -I. -I$(ROSS) -L$(ROSS)/lib
#AM_CFLAGS=-O6 -Wall -I. -I$(ROSS) -L$(ROSS)/lib

lib_LTLIBRARIES=libnum.la

include_HEADERS=\
	num-extern.h	\
	num-types.h	\
	num.h

libnum_la_SOURCES=\
	num-report.c	\
	num-global.c	\
	num-main.c	\
	num-xml.c	\
	num-helper.c	\
	num-init.c	\
	num-setup.c	\
	num-file.c	\
	num.c

#num-timer.c
