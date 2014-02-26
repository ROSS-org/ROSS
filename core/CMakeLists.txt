PROJECT(ROSS C)
INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})
MESSAGE("ROSS_SOURCE_DIR=${ROSS_SOURCE_DIR}")

SET(ross_srcs

queue/${QUEUE}.c

ross-random.h
ross-random.c
rand-${RAND}.h
rand-${RAND}.c

clock/${CLOCK}.h
clock/${CLOCK}.c

ross-gvt.h
gvt/${GVT}.h
gvt/${GVT}.c

ross-network.h
network-mpi1.h
network-mpi2.h
network-mpi.c

hash-quadratic.h
hash-quadratic.c

ross-extern.h
ross-global.c

ross-inline.h
ross-kernel-inline.h
ross.h
ross-types.h

signal-sigaction.c

tw-eventq.h
tw-event.c

tw-opts.h
tw-opts.c

tw-pe.c
tw-kp.c
tw-lp.c

tw-timing.h
tw-timing.c
tw-timer.h
tw-timer.c

tw-sched.c
tw-setup.c
tw-signal.c
tw-stats.c
tw-util.c)


# ROSS VERSION INFORMATION
## Print Date and Time at top of ROSS output
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(ctime HAVE_CTIME)
ADD_DEFINITIONS(-DHAVE_CTIME=${HAVE_CTIME})
## Print ROSS Git Hash
## From http://stackoverflow.com/questions/1435953/how-can-i-pass-git-sha1-to-compiler-as-definition-using-cmake
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
INCLUDE(GetGitRevisionDescription)
GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)
ADD_DEFINITIONS(-DROSS_VERSION="${GIT_SHA1}")

# Data Structure for remote Events
# If AVL_TREE is OFF, ROSS reverts to hashing
OPTION(AVL_TREE "Use AVL trees for tw_events?" ON)
SET(AVL_TREE_SIZE 18 CACHE STRING "AVL Treet contians 2^AVL_TREE_SIZE nodes")
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

SET(OPTIONS "${OPTIONS} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_mpi -DROSS_CLOCK_${CLOCK} -DROSS_GVT_${GVT} -DARCH_${ARCH}")

MESSAGE("OPTIONS=${OPTIONS}")
ADD_DEFINITIONS(${OPTIONS})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${OPTIONS}\"'")

ADD_LIBRARY(ROSS ${ross_srcs})

#
# install libROSS.a and the header files required to build a model
#
# INSTALL(FILES ${CMAKE_BINARY_DIR}/ross/libROSS.a DESTINATION lib)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-extern.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-gvt.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-inline.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-kernel-inline.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-network.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-types.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/clock/${CLOCK}.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/network-mpi1.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/network-mpi2.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/hash-quadratic.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/rand-${RAND}.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/gvt/${GVT}.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-opts.h DESTINATION include) 
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-eventq.h DESTINATION include) 
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-memory.h DESTINATION include) 
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-memoryq.h DESTINATION include) 
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-timer.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/ross-random.h DESTINATION include)
# INSTALL(FILES ${CMAKE_SOURCE_DIR}/ross/tw-timing.h DESTINATION include)
