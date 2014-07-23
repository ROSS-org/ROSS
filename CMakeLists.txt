PROJECT(ROSS_TOP)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
ENABLE_TESTING()

INCLUDE(CTest)

INCLUDE(CheckTypeSize)
CHECK_TYPE_SIZE("short"         SIZEOF_SHORT       )
CHECK_TYPE_SIZE("int"           SIZEOF_INT         )
CHECK_TYPE_SIZE("float"         SIZEOF_FLOAT       )
CHECK_TYPE_SIZE("double"        SIZEOF_DOUBLE      )
CHECK_TYPE_SIZE("long"          SIZEOF_LONG        )
CHECK_TYPE_SIZE("long int"      SIZEOF_LONG_INT    )
CHECK_TYPE_SIZE("long long"     SIZEOF_LONG_LONG   )
CHECK_TYPE_SIZE("uint"          SIZEOF_UINT        )
CHECK_TYPE_SIZE("void *"        SIZEOF_VPTR        )

#SET(QUEUE      calendar) #Calendar Queue
#SET(QUEUE      heap) #Push/Down Heap
SET(QUEUE       splay) #Splay Tree

#SET(MAPPING      none)
#SET(MAPPING       linear)
#SET(MAPPING      rrobin)

SET(RAND           clcg4)
OPTION(RAND_NORMAL "Turn on state for normal distribution" ON)

#MEMORY is either on or off depending on whether or not we desire
#memory buffers.  If it's not set to YES, it defaults to NO
OPTION(MEMORY	  "ROSS Memory Buffers (membufs)"     OFF)
#SET(MEMORY        YES)
OPTION(ALLOC_DEBUG "Use naive allocator to be more friendly to memory debugging tools" OFF)

OPTION(TIMING	   "Perform ROSS timings"	ON)

OPTION(WITH_NETDMF "NetDMF support"             OFF)

OPTION(WITH_ROSSNET "Build ROSS.Net?"		OFF)

OPTION(WITH_ROSS_IO "Build ROSS IO libraries?"  OFF)

OPTION(AVL_TREE	    "Use AVL trees for tw_events?" ON)
SET(AVL_TREE_SIZE 18 CACHE STRING "AVL tree contains 2^AVL_TREE_SIZE")

SET(NETWORK       mpi)
#SET(NETWORK      tcp)
#SET(NETWORK      none)

# Pthread support is depreciate!
#SET(THREAD       pthread)
#SET(THREAD        none)

SET(GVT           mpi_allreduce)
#SET(GVT          7oclock)
#SET(GVT          none)

SET(VALID_ARCH NO)
SET(ARCH $ENV{ARCH} CACHE STRING "Target architecture")
MARK_AS_ADVANCED(ARCH)
IF(NOT ARCH)
	MESSAGE(FATAL_ERROR
		"ARCH not defined!\n"
		"Please set ARCH to be one of (i386 | bgl | bgp | x86_64 | ppc64)")
ENDIF(NOT ARCH)

IF(ARCH STREQUAL i386)
        SET(VALID_ARCH YES)
        #SET(CC	 mpicc)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
        ADD_DEFINITIONS(-D_GNU_SOURCE)
        SET(CLOCK       i386)
ENDIF(ARCH STREQUAL i386)

IF(ARCH STREQUAL bgl)
       SET(VALID_ARCH YES)
       #SET(CC   mpixlc)
       #SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
       SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
       SET(OPTIONS "-qtune=440 -qarch=440d")
       SET(CLOCK   bgl)
ENDIF(ARCH STREQUAL bgl)

IF(ARCH STREQUAL bgp)
       SET(VALID_ARCH YES)
       #SET(CC   mpixlc)
       SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
       SET(OPTIONS "-qtune=450 -qarch=450d")
       SET(CLOCK   bgl)
ENDIF(ARCH STREQUAL bgp)

IF(ARCH STREQUAL bgq)
       SET(VALID_ARCH YES)
       OPTION(BGPM    "Blue Gene/Q specific Performance Counters" OFF)
       SET(BGPM_INCLUDE    /bgsys/drivers/ppcfloor/bgpm/include/)
       ADD_LIBRARY(imp_bgpm STATIC IMPORTED)
       SET_PROPERTY(TARGET imp_bgpm PROPERTY IMPORTED_LOCATION /bgsys/drivers/ppcfloor/bgpm/lib/libbgpm.a)
       SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${BGPM_INCLUDE} -O5 -qstrict -qprefetch=aggressive -qarch=qp -qtune=qp -qmaxmem=-1 -qsimd=noauto -qreport -qhot")
       SET(OPTIONS "")
       SET(CLOCK   bgq)
ENDIF(ARCH STREQUAL bgq)

IF(ARCH STREQUAL ppc64)
       SET(VALID_ARCH YES)
       #SET(CC   mpicc)
       SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
       ADD_DEFINITIONS(-D_GNU_SOURCE)
       # For using Unity on a 64-bit platform
       ADD_DEFINITIONS(-DUNITY_INCLUDE_64)
       SET(CLOCK       ppc)
ENDIF(ARCH STREQUAL ppc64)

IF(ARCH STREQUAL x86_64)
        SET(VALID_ARCH YES)
        #SET(CC	 mpicc)
        #SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O6 -Wall")
        #SET(CMAKE_BUILD_TYPE Release)
        SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
        ADD_DEFINITIONS(-D_GNU_SOURCE)
        # For using Unity on a 64-bit platform
        ADD_DEFINITIONS(-DUNITY_INCLUDE_64)	
        SET(CLOCK       amd64)
ENDIF(ARCH STREQUAL x86_64)

IF(NOT VALID_ARCH)
       MESSAGE(FATAL_ERROR
		"ARCH not valid!\n"
		"Please set ARCH to be one of (i386 | bgl | bgp | x86_64 | ppc64)")
ENDIF(NOT VALID_ARCH)

ADD_SUBDIRECTORY(ross)
IF(WITH_ROSSNET)
	ADD_SUBDIRECTORY(rnf)
ENDIF(WITH_ROSSNET)

FIND_PACKAGE(Doxygen)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.user.in
               ${CMAKE_BINARY_DIR}/Doxyfile.user)

IF(DOXYGEN_FOUND)
	OPTION(BUILD_DOXYGEN "Build Doxygen documentation" OFF)
	IF(DOXYGEN_DOT_FOUND)
		OPTION(DEFAULT_GRAPHS "Build Default Doxygen Graphs" OFF)	
		OPTION(CALL_GRAPHS "Build Doxygen Call Graphs" OFF)
		OPTION(CALLER_GRAPHS "Build Doxygen Caller Graphs" OFF)
	ENDIF(DOXYGEN_DOT_FOUND)	
ENDIF(DOXYGEN_FOUND)

IF(BUILD_DOXYGEN)
	IF(DOXYGEN_DOT_FOUND)
		SET(HAVE_DOT YES)
		IF(CALL_GRAPHS)
			SET(CALL_GRAPHS_ON YES)
		ELSE(CALL_GRAPHS)
			SET(CALL_GRAPHS_ON NO)
		ENDIF(CALL_GRAPHS)
		IF(CALLER_GRAPHS)
			SET(CALLER_GRAPHS_ON YES)
		ELSE(CALLER_GRAPHS)
			SET(CALLER_GRAPHS_ON NO)
		ENDIF(CALLER_GRAPHS)
		IF(DEFAULT_GRAPHS)
			SET(DEFAULT_GRAPHS_ON YES)
		ELSE(DEFAULT_GRAPHS)
			SET(DEFAULT_GRAPHS_ON NO)
		ENDIF(DEFAULT_GRAPHS)
	ELSE(DOXYGEN_DOT_FOUND)
		SET(HAVE_DOT NO)
		SET(CALL_GRAPHS NO)
	ENDIF(DOXYGEN_DOT_FOUND)

	SET(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/docs/html/index.html)

	ADD_CUSTOM_COMMAND(
		OUTPUT ${DOXYGEN_OUTPUT}
		COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..."
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile.user
#		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
		COMMAND ${CMAKE_COMMAND} -E echo "Done."
#		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#		DEPENDS ${DOXYGEN_INPUT}
	)

	ADD_CUSTOM_TARGET(apidoc ALL DEPENDS ${DOXYGEN_OUTPUT})

ENDIF(BUILD_DOXYGEN)

OPTION(UNIT_TESTS        "Build ROSS tests" OFF)
