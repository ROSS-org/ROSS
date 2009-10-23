INCLUDES=-I$(top_srcdir)

#AM_CFLAGS=-O6 -Wall
AM_CFLAGS=-g -Wall -D_GNU_SOURCE

lib_LTLIBRARIES=libtlm.la

include_HEADERS=\
	tlm-extern.h \
	tlm-types.h \
	tlm.h 

libtlm_la_SOURCES=\
	tlm-global.c \
	tlm-cell.c \
	tlm-rc-cell.c \
	tlm-grid.c \
	tlm-helper.c \
	tlm-interface.c \
	tlm-particle.c \
	tlm-pe.c \
	tlm-proximity.c \
	tlm-model.c \
	tlm-xml.c \
	tlm.c
