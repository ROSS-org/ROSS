INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/rossnet)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/../../ross)
SET(tcp_srcs
tcp-model.c		tcp-timer.c
tcp-process.c		tcp-types.h
tcp-rc-process.c	tcp-update.c
tcp-rc-timeout.c	tcp-util.c
tcp-rc-timer.c  	tcp-xml.c
tcp-extern.h		tcp-routing.c
tcp.c			tcp-global.c
tcp-timeout.c  tcp.h)

FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(tcp ${tcp_srcs})
