INCLUDES=-I$(top_srcdir)

#AM_CFLAGS=-O6 -Wall
AM_CFLAGS=-g -Wall -D_GNU_SOURCE

lib_LTLIBRARIES=librw.la

include_HEADERS=\
	rw-extern.h \
	rw-types.h \
	rw.h 

librw_la_SOURCES=\
	rw-global.c \
	rw-helper.c \
	rw-node.c \
	rw-model.c \
	rw-xml.c \
	rw.c
