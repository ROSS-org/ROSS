INCLUDES=-I$(top_srcdir)

#AM_CFLAGS=-O6 -Wall
AM_CFLAGS=-g -Wall #-DVERIFY_TCP_TIMER -DTCP_DEBUG

lib_LTLIBRARIES=libtcp.la

include_HEADERS=\
	tcp-extern.h \
	tcp-types.h \
	tcp.h 

libtcp_la_SOURCES=\
	tcp-rc-process.c \
	tcp-rc-timeout.c \
	tcp-rc-timer.c \
	tcp-global.c \
	tcp-timer.c \
	tcp-process.c \
	tcp-update.c \
	tcp-timeout.c \
	tcp-model.c \
	tcp-util.c \
	tcp-xml.c \
	tcp.c
