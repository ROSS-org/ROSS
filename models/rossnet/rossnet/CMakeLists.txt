INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/rossnet)
INCLUDE_DIRECTORIES(${ROSSNET_SOURCE_DIR}/../../ross)
SET(rn_srcs
rn-link.c		rn-timer.c
rn-lp.c 		rn-machine.c
hash-quadratic.c	rn-message.c
rn-area.c 		rn-models.c
rn-as.c 		rn-reverse.c
rn-event.c 		rn-routing.c
rn-setup.c 		rn-global.c 
rn-stream.c 		rn-init.c 
rn-subnet.c 		xml-libxml.c)

FIND_PACKAGE(LibXml2 REQUIRED)
SET(MPI_COMPILER mpicc)
FIND_PACKAGE(MPI REQUIRED)
LINK_DIRECTORIES(${ROSSNET_SOURCE_DIR}/../../ross)
#LINK_DIRECTORIES(/Users/laprej/pdes/trunk/ross/)
INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(ROSSNET ${rn_srcs})
ADD_EXECUTABLE(rn rn.c)
target_link_libraries (rn ${LIBXML2_LIBRARIES} ${MPI_EXTRA_LIBRARY} tcp ip ROSSNET ROSS)
