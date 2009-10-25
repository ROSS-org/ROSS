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

#SET(ARCH	bgl)
#SET(ARCH	bgp)
#SET(ARCH	ppc64)
#SET(ARCH	x86_64)

#IF(ARCH 

IF(MEMORY)
	SET(ross_srcs ${ross_srcs} tw-memory.c tw-memoryq.h tw-memory.h)
ENDIF(MEMORY)

SET(OPTIONS "-DROSS_MAPPING_${MAPPING} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_${NETWORK} -DROSS_CLOCK_${CLOCK} -DROSS_THREAD_${THREAD} -DROSS_GVT_${GVT} -DROSS_RAND_${RAND} -DARCH_${ARCH}")

#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${OPTIONS}\"'")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${CMAKE_C_FLAGS}\"'")

#INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(ROSS ${ross_srcs})
