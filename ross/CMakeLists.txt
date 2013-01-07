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
tw-util.c
uthash.h)

IF(MEMORY)
	SET(OPTIONS "${OPTIONS} -DROSS_MEMORY")
	SET(ross_srcs ${ross_srcs} tw-memory.c tw-memoryq.h tw-memory.h)
ENDIF(MEMORY)

IF(RAND_NORMAL)
	SET(OPTIONS "${OPTIONS} -DRAND_NORMAL")
ENDIF(RAND_NORMAL)

IF(WITH_UTHASH)
	SET(OPTIONS "${OPTIONS} -DUTHASH")
ENDIF(WITH_UTHASH)

IF(TIMING)
	SET(OPTIONS "${OPTIONS} -DROSS_timing")
ENDIF(TIMING)

IF(NETWORK STREQUAL tcp)
	   SET(ross_srcs ${ross_srcs} socket-tcp.c socket-tcp.h)
ENDIF(NETWORK STREQUAL tcp)

SET(OPTIONS "${OPTIONS} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_${NETWORK} -DROSS_CLOCK_${CLOCK} -DROSS_GVT_${GVT} -DARCH_${ARCH}")

ADD_DEFINITIONS(${OPTIONS})

#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTIONS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${OPTIONS}\"'")
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${CMAKE_C_FLAGS}\"'")

#INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

ADD_LIBRARY(ROSS ${ross_srcs})

#
# install libROSS.a and the header files required to build a model
#
INSTALL(FILES ${CMAKE_BINARY_DIR}/ross/libROSS.a DESTINATION "/lib")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-extern.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-gvt.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-inline.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-kernel-inline.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-network.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-types.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/clock-${CLOCK}.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/network-${NETWORK}1.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/network-${NETWORK}2.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/hash-quadratic.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/rand-${RAND}.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/gvt-${GVT}.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-opts.h DESTINATION "/include") 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-eventq.h DESTINATION "/include") 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-memory.h DESTINATION "/include") 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-memoryq.h DESTINATION "/include") 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-timer.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-random.h DESTINATION "/include")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-timing.h DESTINATION "/include")

#
# build and install the ross-config script
#
file(READ ${CMAKE_SOURCE_DIR}/ross-config.in rcf0)
STRING(REGEX REPLACE "@ROSS_CFLAGS@" "${OPTIONS}" rcf1 "${rcf0}")
STRING(REGEX REPLACE "@ROSS_CC@" "$ENV{CC}" rcf2 "${rcf1}")
STRING(REGEX REPLACE "@ROSS_CXX@" "$ENV{CXX}" rcf3 "${rcf2}")
file(WRITE ${CMAKE_BINARY_DIR}/ross-config "${rcf3}")
INSTALL(FILES ${CMAKE_BINARY_DIR}/ross-config DESTINATION "/bin" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

OPTION(ROSS_BUILD_MODELS "Build ROSS models?" ON)

IF(ROSS_BUILD_MODELS)
    ADD_SUBDIRECTORY(models)
ENDIF(ROSS_BUILD_MODELS)

IF(UNIT_TESTS)
    ADD_SUBDIRECTORY(test)
ENDIF(UNIT_TESTS)
