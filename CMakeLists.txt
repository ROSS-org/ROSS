PROJECT(ROSS_TOP C)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

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
OPTION(MEMORY	  "ROSS Memory Buffers (membufs)"     ON)
SET(MEMORY        YES)

OPTION(TIMING	   "Perform ROSS timings"	ON)

SET(NETWORK       mpi)
#SET(NETWORK      tcp)
#SET(NETWORK      none)

# Pthread support is depreciate!
#SET(THREAD       pthread)
#SET(THREAD        none)

SET(GVT           mpi_allreduce)
#SET(GVT          7oclock)
#SET(GVT          none)

SET(ARCH $ENV{ARCH})
IF(NOT ARCH)
	MESSAGE(FATAL_ERROR
		"ARCH not defined!\n"
		"Please set ARCH to be one of (bgl | bgp | x86_64 | ppc64)")
ENDIF(NOT ARCH)

IF(ARCH STREQUAL i386)
        #SET(CC	 mpicc)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
        ADD_DEFINITIONS(-D_GNU_SOURCE)
        SET(CLOCK       i386)
ENDIF(ARCH STREQUAL i386)

IF(ARCH STREQUAL bgl)
       #SET(CC   mpixlc)
       #SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
       SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
       SET(OPTIONS "-qtune=440 -qarch=440d")
       SET(CLOCK   bgl)
ENDIF(ARCH STREQUAL bgl)

IF(ARCH STREQUAL bgp)
       #SET(CC   mpixlc)
       SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
       SET(OPTIONS "-qtune=450 -qarch=450d")
       SET(CLOCK   bgl)
ENDIF(ARCH STREQUAL bgp)

IF(ARCH STREQUAL ppc64)
       #SET(CC   mpicc)
       SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
       ADD_DEFINITIONS(-D_GNU_SOURCE)
       SET(CLOCK       ppc)
ENDIF(ARCH STREQUAL ppc64)

IF(ARCH STREQUAL x86_64)
        #SET(CC	 mpicc)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
        ADD_DEFINITIONS(-D_GNU_SOURCE)
        SET(CLOCK       amd64)
ENDIF(ARCH STREQUAL x86_64)

ADD_SUBDIRECTORY(ross)
ADD_SUBDIRECTORY(rnf)

FIND_PACKAGE(Doxygen)

IF(DOXYGEN_FOUND)
	OPTION(BUILD_DOXYGEN "Build Doxygen documentation" ON)
ENDIF(DOXYGEN_FOUND)

OPTION(CALL_GRAPHS "Build Doxygen Call Graphs" OFF)

IF(BUILD_DOXYGEN)
	IF(DOXYGEN_DOT_FOUND)
		SET(HAVE_DOT YES)
		IF(CALL_GRAPHS)
			SET(GRAPHS_ON YES)
		ELSE(CALL_GRAPHS)
			SET(GRAPHS_ON NO)
		ENDIF(CALL_GRAPHS)
	ELSE(DOXYGEN_DOT_FOUND)
		SET(HAVE_DOT NO)
		SET(CALL_GRAPHS NO)
	ENDIF(DOXYGEN_DOT_FOUND)


	CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.user.in
		       ${CMAKE_BINARY_DIR}/Doxyfile.user)

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
