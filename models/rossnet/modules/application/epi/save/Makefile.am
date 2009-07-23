INCLUDES=-I$(top_srcdir)

#AM_CFLAGS=-g -Wall -I. -I$(ROSS) -L$(ROSS)/lib #-D_GNU_SOURCE
AM_CFLAGS=-g -Wall -I. -I$(ROSS) -L$(ROSS)/lib -D _DEBUG_
#AM_CFLAGS=-O6 -Wall -I. -I$(ROSS) -L$(ROSS)/lib

lib_LTLIBRARIES=libepi.la

include_HEADERS=\
	epi-extern.h	\
	epi-types.h	\
	epi.h

libepi_la_SOURCES=\
	queue-heap.c		\
	epi-report.c		\
	epi-timer.c		\
	epi-agent.c		\
	epi-global.c		\
	epi-stats.c		\
	epi-init.c		\
	epi-setup.c		\
	epi-seir.c		\
	epi-xml.c		\
	epi-main.c		\
	epi-num.c		\
	epi.c
