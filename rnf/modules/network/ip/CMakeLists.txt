IF(WITH_IP)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/rossnet)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/../../ross)
SET(ip_srcs
ip-global.c		ip-upstream.c
ip-model.c		ip-xml.c
ip-packet.c		ip.c
ip-rc-downstream.c	ip.h
ip-downstream.c		ip-routing.c
ip-extern.h		ip-types.h)

FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(ip ${ip_srcs})
ENDIF(WITH_IP)
