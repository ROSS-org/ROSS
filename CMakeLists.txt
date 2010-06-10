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
# ADD_SUBDIRECTORY(rnf)
