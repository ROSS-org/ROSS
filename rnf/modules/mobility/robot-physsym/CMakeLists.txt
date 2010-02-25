IF(WITH_ROBOT_PHYSSYM)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/kernel)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/modules/physical/tlm)
INCLUDE_DIRECTORIES(${ROSS_TOP_SOURCE_DIR}/trunk/ross)

SET(rp_srcs
rp-global.c	rp-node.c	rp.c
rp-helper.c	rp-types.h	rp.h
rp-extern.h	rp-model.c	rp-xml.c)

#FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(./physsym-layer)

ADD_LIBRARY(rp ${rp_srcs} ./physsym-layer/physsym.o ./physsym-layer/libpath46.a)
ENDIF(WITH_ROBOT_PHYSSYM)
