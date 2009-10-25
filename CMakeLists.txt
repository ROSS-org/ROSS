#SET(QUEUE      calendar) #Calendar Queue
#SET(QUEUE      heap) #Push/Down Heap
SET(QUEUE       splay) #Splay Tree
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_QUEUE_${QUEUE}")

#SET(MAPPING      none)
SET(MAPPING       linear)
#SET(MAPPING      rrobin)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_MAPPING_${MAPPING}")

SET(RAND           clcg4)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_RAND_${RAND}")

#MEMORY is either on or off depending on whether or not we desire
#memory buffers.  If it's not set to YES, it defaults to NO
SET(MEMORY        YES)
IF(MEMORY)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_MEMORY")
ENDIF(MEMORY)

SET(NETWORK       mpi)
#SET(NETWORK      tcp)
#SET(NETWORK      none)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_NETWORK_${NETWORK}")

#SET(THREAD       pthread)
SET(THREAD        none)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_THREAD_${THREAD}")

SET(CLOCK         amd64)
#SET(CLOCK        amd64)
#SET(CLOCK        i386)
#SET(CLOCK        ppc)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_CLOCK_${CLOCK}")

SET(GVT           mpi_allreduce)
#SET(GVT                  7oclock)
#SET(GVT                  none)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_GVT_${GVT}")

ADD_SUBDIRECTORY(ross)
ADD_SUBDIRECTORY(rnf)
ADD_SUBDIRECTORY(models)
