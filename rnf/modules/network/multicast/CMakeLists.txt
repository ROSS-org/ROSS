IF(WITH_MULTICAST)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/kernel)
INCLUDE_DIRECTORIES(${ROSS_TOP_SOURCE_DIR}/trunk/ross)
SET(multicast_srcs
Routing.c	Util.c
gmcc-snd.c	pgmcc.c
Routing.h	Util.h
gmcc-snd.h	pgmcc.h
Multicast.c	Topo.c
formula.c	gmcc-util.c
tfmcc-sink.c	Multicast.h
Topo.h		formula.h
gmcc-util.h	tfmcc-sink.h
Packet.c	Transport.c
gmcc-rcv.c	ormcc.c
tfmcc.c		Packet.h
Transport.h	gmcc-rcv.h
ormcc.h		tfmcc.h)

#FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(multicast ${multicast_srcs})
ENDIF(WITH_MULTICAST)
