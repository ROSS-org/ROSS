IF(WITH_EPI)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/rossnet)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/../../ross)
SET(epi_srcs
epi-global.c	epi-seir.c	epi-xml.c	epi-init.c
epi-setup.c	epi.c		epi-agent.c	epi-main.c
epi-stats.c	epi.h		epi-agent.h	epi-num.c
epi-timer.c	ip.log		epi-extern.h	epi-report.c
epi-types.h	queue-heap.c)

#FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(epi ${epi_srcs})
ENDIF(WITH_EPI)
