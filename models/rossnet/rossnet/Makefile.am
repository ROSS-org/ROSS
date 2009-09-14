lib_LTLIBRARIES=libROSSNET.la

AM_CFLAGS=-g -Wall #-DVERIFY_RN #-DRN_VERIFY_SETUP #-DVERIFY_RN
#AM_CFLAGS=-O6 -Wall

INCLUDES=\
	-I../modules/mobility/random-walk \
	-I../modules/physical/tlm \
	-I../modules/application/phold \
	-I../modules/application/epi \
	-I../modules/application/num \
	-I../modules/transport/tcp \
	-I../modules/network/ospfv2 \
	-I../modules/network/bgp4 \
	-I../modules/network/ip

include_HEADERS=\
	rossnet.h \
	rn-extern.h \
	rn-types.h \
	rn_config.h

libROSSNET_la_SOURCES=\
	hash-quadratic.c \
	rn-routing.c \
	rn-global.c \
	rn-init.c \
	rn-models.c \
	rn-area.c \
	rn-as.c \
	xml-libxml.c \
	rn-setup.c \
	rn-message.c \
	rn-event.c \
	rn-reverse.c \
	rn-link.c \
	rn-stream.c \
	rn-lp.c \
	rn-machine.c \
	rn-timer.c \
	rn-subnet.c

bin_PROGRAMS=rn

rn_SOURCES=rn.c

rn_LDADD=\
	$(top_builddir)/modules/mobility/random-walk/librw.la \
	$(top_builddir)/modules/physical/tlm/libtlm.la \
	$(top_builddir)/modules/application/phold/libphold.la \
	$(top_builddir)/modules/application/epi/libepi.la \
	$(top_builddir)/modules/application/num/libnum.la \
	$(top_builddir)/modules/transport/tcp/libtcp.la \
	$(top_builddir)/modules/network/ip/libip.la \
	$(top_builddir)/modules/network/bgp4/libbgp4.la \
	$(top_builddir)/modules/network/ospfv2/libospfv2.la \
	libROSSNET.la
