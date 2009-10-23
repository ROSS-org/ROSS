IF(WITH_RANDOM_WALK)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/rossnet)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/../../ross)
SET(rw_srcs
rw-global.c	rw-node.c	rw.c
rw-helper.c	rw-types.h	rw.h
rw-extern.h	rw-model.c	rw-xml.c)

#FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(rw ${rw_srcs})
ENDIF(WITH_RANDOM_WALK)
