IF(WITH_OSPF)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/rossnet)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/../../ross)
SET(ospf_srcs
ospf-lsa.c		ospf-main.c		ospf-message-types.h
ospf-ack.c		ospf-neighbor-list.c	ospf-data-structure.h
ospf-neighbor-state.c	ospf-data-types.h	ospf-neighbor-types.h
ospf-database.c		ospf-neighbor.c		ospf-dd.c
ospf-router-debug.c	ospf-decls.h		ospf-router-rc.c
ospf-experiments.c	ospf-router.c		ospf-extern-rc.h
ospf-routing.c		ospf-extern.h		ospf-stats.c
ospf-flood.c		ospf-timer-aging.c	ospf-global.c
ospf-timer-retransmit.c	ospf-hello-rc.c		ospf-timer.c
ospf-hello.c		ospf-types.h		ospf-interface.c
ospf-util.c		ospf-ip.c		ospf-xml.c
ospf-ls-request.c	ospf.h			ospf-ls.c)

#FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(ospf ${ospf_srcs})
ENDIF(WITH_OSPF)
