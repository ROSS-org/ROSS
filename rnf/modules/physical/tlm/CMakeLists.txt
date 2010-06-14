IF(WITH_TLM)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/kernel)
INCLUDE_DIRECTORIES(${ROSS_TOP_SOURCE_DIR}/ross)
SET(tlm_srcs
tlm-grid.c		tlm-proximity.c
tlm-helper.c		tlm-rc-cell.c
tlm-interface.c		tlm-types.h
tlm-cell.c		tlm-model.c
tlm-xml.c		tlm-extern.h
tlm-particle.c		tlm.c
tlm-global.c		tlm-pe.c
tlm.h)

#FIND_PACKAGE(LibXml2 REQUIRED)

INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(tlm ${tlm_srcs})
ENDIF(WITH_TLM)
