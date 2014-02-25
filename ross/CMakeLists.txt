PROJECT(ROSS C)
INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(ross_srcs
ross-extern.h
ross-types.h
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
tw-stats.c
tw-timing.c
tw-timing.h
tw-util.c)

INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(ctime HAVE_CTIME)
CONFIGURE_FILE(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

IF(AVL_TREE)
	SET(ross_srcs ${ross_srcs} avl_tree.h avl_tree.c)
	MATH( EXPR x "1 << ${AVL_TREE_SIZE}" )
	SET(OPTIONS "${OPTIONS} -DUSE_AVL_TREE")
	SET(OPTIONS "${OPTIONS} -DAVL_NODE_COUNT=${x}")
ENDIF(AVL_TREE)

IF(BGPM)
	SET(OPTIONS "${OPTIONS} -DUSE_BGPM")
ENDIF(BGPM)

IF(MEMORY)
	SET(OPTIONS "${OPTIONS} -DROSS_MEMORY")
	SET(ross_srcs ${ross_srcs} tw-memory.c tw-memoryq.h tw-memory.h)
ENDIF(MEMORY)

IF(RAND_NORMAL)
	SET(OPTIONS "${OPTIONS} -DRAND_NORMAL")
ENDIF(RAND_NORMAL)

IF(TIMING)
	SET(OPTIONS "${OPTIONS} -DROSS_timing")
ENDIF(TIMING)

IF(NETWORK STREQUAL tcp)
	   SET(ross_srcs ${ross_srcs} socket-tcp.c socket-tcp.h)
ENDIF(NETWORK STREQUAL tcp)

SET(OPTIONS "${OPTIONS} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_${NETWORK} -DROSS_CLOCK_${CLOCK} -DROSS_GVT_${GVT} -DARCH_${ARCH}")

# From http://stackoverflow.com/questions/1435953/how-can-i-pass-git-sha1-to-compiler-as-definition-using-cmake
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
INCLUDE(GetGitRevisionDescription)
GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)
ADD_DEFINITIONS(-DROSS_VERSION="${GIT_SHA1}")

ADD_DEFINITIONS(${OPTIONS})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${OPTIONS}\"'")

ADD_LIBRARY(ROSS ${ross_srcs})

#
# install libROSS.a and the header files required to build a model
#
INSTALL(FILES ${CMAKE_BINARY_DIR}/ross/libROSS.a DESTINATION lib)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-extern.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-gvt.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-inline.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-kernel-inline.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-network.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-types.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/clock-${CLOCK}.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/network-${NETWORK}1.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/network-${NETWORK}2.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/hash-quadratic.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/rand-${RAND}.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/gvt-${GVT}.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-opts.h DESTINATION include) 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-eventq.h DESTINATION include) 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-memory.h DESTINATION include) 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-memoryq.h DESTINATION include) 
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-timer.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-random.h DESTINATION include)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-timing.h DESTINATION include)
