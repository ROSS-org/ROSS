lib_LTLIBRARIES=libospfv2.la

#AM_CFLAGS=-O6 -Wall #-DOSPF_LOG
AM_CFLAGS=-g -Wall #-DOSPF_LOG #-DVERIFY_LS #-DVERIFY_OSPF_CONVERGENCE -DLOG_ROUTING_TABLES -DVERIFY_LS #-DVERIFY_AGING

include_HEADERS=\
	ospf-extern.h \
	ospf-types.h \
	ospf-data-structure.h \
	ospf-data-types.h \
	ospf-decls.h \
	ospf-message-types.h \
	ospf-neighbor-types.h \
	ospf.h 

libospfv2_la_SOURCES=\
	ospf-global.c \
	ospf-util.c \
	ospf-ack.c \
	ospf-database.c \
	ospf-experiments.c \
	ospf-dd.c \
	ospf-flood.c \
	ospf-hello.c \
	ospf-hello-rc.c \
	ospf-interface.c \
	ospf-ls.c \
	ospf-lsa.c \
	ospf-neighbor-state.c \
	ospf-neighbor-list.c \
	ospf-neighbor.c \
	ospf-router-rc.c \
	ospf-router.c \
	ospf-routing.c \
	ospf-stats.c \
	ospf-timer-aging.c \
	ospf-timer.c \
	ospf-main.c \
	ospf-xml.c
