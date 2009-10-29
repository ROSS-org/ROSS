PROJECT(ROSS C)
INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})
SET(ross_srcs
clock-${CLOCK}.c
clock-${CLOCK}.h
network-${NETWORK}.c
network-${NETWORK}1.h
network-${NETWORK}2.h
hash-quadratic.c
hash-quadratic.h
queue-${QUEUE}.c
rand-${RAND}.c
rand-${RAND}.h
ross-global.c
signal-sigaction.c
thread-${THREAD}.c
thread-${THREAD}.h
tw-event.c
gvt-${GVT}.c
gvt-${GVT}.h
tw-kp.c
tw-lp.c
tw-opts.c
tw-opts.h
tw-pe.c
tw-timer.c
tw-timer.h
ross-random.c
ross-random.h
tw-sched.c
tw-setup.c
tw-signal.c
tw-state.c
tw-stats.c
tw-timing.c
tw-timing.h
tw-util.c)

IF(ARCH STREQUAL bgl)
	SET(CC	 mpixlc)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
	SET(OPTIONS "-qtune=440 -qarch=440d")
	SET(CLOCK   blg)
ENDIF(ARCH STREQUAL bgl)

IF(ARCH STREQUAL bgp)
	SET(CC	 mpixlc)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
	SET(OPTIONS "-qtune=450 -qarch=450d")
	SET(CLOCK   blg)
ENDIF(ARCH STREQUAL bgp)

IF(ARCH STREQUAL ppc64)
	SET(CC	 mpicc)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
	ADD_DEFINITIONS(-D_GNU_SOURCE)
	SET(CLOCK	ppc)
ENDIF(ARCH STREQUAL ppc64)

IF(ARCH STREQUAL x86_64)
	SET(CC	 mpicc)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O6 -Wall")
	ADD_DEFINITIONS(-D_GNU_SOURCE)
	SET(CLOCK	amd64)
ENDIF(ARCH STREQUAL x86_64)

IF(MEMORY)
	SET(OPTIONS "${OPTIONS} -DROSS_MEMORY")
	SET(ross_srcs ${ross_srcs} tw-memory.c tw-memoryq.h tw-memory.h)
ENDIF(MEMORY)

IF(TIMING)
	SET(OPTIONS "${OPTIONS} -DROSS_timing")
ENDIF(TIMING)

IF(NETWORK STREQUAL tcp)
	   SET(ross_srcs ${ross_srcs} socket-tcp.c socket-tcp.h)
ENDIF(NETWORK STREQUAL tcp)

SET(OPTIONS "${OPTIONS} -DROSS_MAPPING_${MAPPING} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_${NETWORK} -DROSS_CLOCK_${CLOCK} -DROSS_THREAD_${THREAD} -DROSS_GVT_${GVT} -DARCH_${ARCH}")

ADD_DEFINITIONS(${OPTIONS})

#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTIONS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${OPTIONS}\"'")
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${CMAKE_C_FLAGS}\"'")

#INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(ROSS ${ross_srcs})
