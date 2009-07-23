lib_LTLIBRARIES=libbgp4.la

#AM_CFLAGS=-O6 -Wall -DBGP_CONVERGED
AM_CFLAGS=-g -Wall #-DBGP_CONVERGED #-DVERIFY_BGP -DVERIFY_BGP_DECISION

include_HEADERS=\
	bgp.h

INCLUDES=\
        -I../ospfv2

libbgp4_la_SOURCES=\
	bgp-connect.c \
	bgp-open.c \
	bgp-utils.c \
	bgp-decision.c \
	bgp-nbr.c \
	bgp-xml.c \
	bgp-mrai.c \
	bgp-global.c \
	bgp-keepalive.c \
	bgp-update.c \
	bgp-main.c \
	bgp-notify.c \
	bgp-ospf.c \
	bgp-router.c \
	bgp-init.c \
	bgp-stats.c \
	bgp-route.c 
