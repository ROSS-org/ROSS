PROJECT(ROSS C)
INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})
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
network-${NETWORK}.h
network-${NETWORK}.c

hash-quadratic.h
hash-quadratic.c

ross-extern.h
ross-global.c

ross-inline.h
ross-kernel-inline.h
ross.h
ross-types.h

tw-eventq.h
tw-event.c

tw-opts.h
tw-opts.c

tw-pe.c
tw-kp.c
tw-lp.c

tw-timing.h
tw-timing.c

tw-sched.c
tw-setup.c
tw-signal.c
tw-stats.c
tw-util.c)


# ROSS VERSION INFORMATION
## Print Date and Time at top of ROSS output
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(ctime HAVE_CTIME)
## Print ROSS Git Hash
## From http://stackoverflow.com/questions/1435953/how-can-i-pass-git-sha1-to-compiler-as-definition-using-cmake
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
INCLUDE(GetGitRevisionDescription)
GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)

# Data Structure for remote Events
# If USE_AVL_TREE is OFF, ROSS reverts to hashing
OPTION(USE_AVL_TREE "Use AVL trees for tw_events?" ON)
SET(AVL_TREE_SIZE 18 CACHE STRING "AVL Treet contians 2^AVL_TREE_SIZE nodes")
IF(USE_AVL_TREE)
  SET(ross_srcs ${ross_srcs} avl_tree.h avl_tree.c)
  MATH( EXPR AVL_NODE_COUNT "1 << ${AVL_TREE_SIZE}" )
ENDIF(USE_AVL_TREE)

# ROSS_MEMORY is either on or off depending on whether or not we desire
# memory buffers.  If it's not set to YES, it defaults to NO
OPTION(ROSS_MEMORY "ROSS Memory Buffers (membufs)" OFF)
IF(ROSS_MEMORY)
  SET(ross_srcs ${ross_srcs} tw-memory.c tw-memoryq.h tw-memory.h)
ENDIF(ROSS_MEMORY)

OPTION(RAND_NORMAL "Turn on state for normal distribution" ON)

# Show timing data at end of run
OPTION(ROSS_TIMING "Perform ROSS timings" ON)

# Set all options
#SET(OPTIONS "${OPTIONS} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_mpi -DROSS_CLOCK_${CLOCK} -DROSS_GVT_${GVT} -DARCH_${ARCH}")
#SET(OPTIONS "${OPTIONS} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_mpi -DROSS_CLOCK_${CLOCK} -DROSS_GVT_${GVT} -DARCH_${ARCH}" PARENT_SCOPE)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${OPTIONS}\"'")

# Generate Library
ADD_LIBRARY(ROSS ${ross_srcs})

# Build Specific Config Header
CONFIGURE_FILE(config.h.in config.h)
