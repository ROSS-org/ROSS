INCLUDES=-I$(top_srcdir)

#AM_CFLAGS=-O6 -Wall
AM_CFLAGS=-g -Wall

lib_LTLIBRARIES=libtcplayer.la

include_HEADERS=\
	tcp-layer-extern.h \
	tcp-layer-types.h \
	tcp-layer.h 

libtcplayer_la_SOURCES=\
	tcp-layer-global.c \
	tcp-layer-process-rc.c \
	tcp-layer-process.c \
	tcp-layer-util.c \
	tcp-layer-xml.c \
	tcp-layer.c
