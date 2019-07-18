PROJECT(ROSS C)
INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})

SET(ross_srcs

queue/tw-queue.h
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

network-${NETWORK}.h
network-${NETWORK}.c

hash-quadratic.h
hash-quadratic.c

buddy.h
buddy.c
lz4.h
lz4.c
tw-state.c

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
tw-util.c

instrumentation/st-instrumentation.h
instrumentation/st-instrumentation.c
instrumentation/st-stats-buffer.c
instrumentation/st-sim-engine.c
instrumentation/st-event-trace.c
instrumentation/st-model-data.c
instrumentation/ross-lps/analysis-lp.h
instrumentation/ross-lps/analysis-lp.c
instrumentation/ross-lps/specialized-lps.c)

# ROSS VERSION INFORMATION
## Print Date and Time at top of ROSS output
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(ctime HAVE_CTIME)

## Print ROSS Git Hash
# From http://stackoverflow.com/questions/1435953/how-can-i-pass-git-sha1-to-compiler-as-definition-using-cmake
# Now following this approach (which is based on the previous):
# http://ipenguin.ws/2012/11/cmake-automatically-use-git-tags-as.html
# This way lets us use the actual version numbers of ROSS, instead of the git commit
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
INCLUDE(GetGitRevisionDescription)
#GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)

git_describe(VERSION --tags --dirty)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${VERSION}")
# VERSION_SHORT used in the ross.pc file
set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")


# Data Structure for remote Events
# If AVL_TREE is OFF, ROSS reverts to hashing
OPTION(AVL_TREE "Use AVL trees for optimistic mode events? (hash tabels otherwise)" ON)
IF(AVL_TREE)
  SET(ross_srcs ${ross_srcs} avl_tree.h avl_tree.c)
ENDIF(AVL_TREE)

# RIO: Restart IO
OPTION(USE_RIO "Enable RIO checkpointing library?" OFF)
IF(USE_RIO)
  SET(ross_srcs ${ross_srcs} rio/io-mpi.c rio/io-serialize.c)
  INCLUDE_DIRECTORIES(rio)
ENDIF(USE_RIO)

# Damaris I/O and data management
OPTION(USE_DAMARIS "Build with Damaris library (for in situ vis/analysis)?" OFF)
if (USE_DAMARIS)
    ADD_SUBDIRECTORY(risa)
    INCLUDE_DIRECTORIES(${DAMARIS_INCLUDE})
    SET(ross_srcs ${ross_srcs} ${ROSS_Damaris_SOURCE_DIR}/core/damaris.h)
ENDIF(USE_DAMARIS)

OPTION(USE_TW_STIME_BITS "Use bits implementation of tw_stime" OFF)
IF(USE_TW_STIME_BITS)
  SET(ross_srcs ${ross_srcs} time/bits.h time/bits.c)
  INCLUDE_DIRECTORIES(time)
ENDIF(USE_TW_STIME_BITS)


# Use debugging-friendly memory allocation
OPTION(ROSS_ALLOC_DEBUG "Use naive allocator to be more friendly to memory debugging tools" OFF)

OPTION(RAND_NORMAL "Turn on state for normal distribution" ON)

# Show timing data at end of run
OPTION(ROSS_timing "Perform ROSS timings" ON)

# Used by ross-kernel-inline.h
OPTION(ROSS_runtime_checks "Perform ID checks" OFF)
IF(CMAKE_BUILD_TYPE MATCHES Debug)
	SET(ROSS_runtime_checks ON CACHE BOOL "Perform ID checks" FORCE)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

# Set all options
#SET(OPTIONS "${OPTIONS} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_mpi -DROSS_CLOCK_${CLOCK} -DROSS_GVT_${GVT} -DARCH_${ARCH}")
#SET(OPTIONS "${OPTIONS} -DROSS_QUEUE_${QUEUE} -DROSS_RAND_${RAND} -DROSS_NETWORK_mpi -DROSS_CLOCK_${CLOCK} -DROSS_GVT_${GVT} -DARCH_${ARCH}" PARENT_SCOPE)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROSS_OPTION_LIST='\"${OPTIONS}\"'")

# Generate Library
OPTION(ROSS_BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)
SET(BUILD_SHARED_LIBS ${ROSS_BUILD_SHARED_LIBS})
ADD_LIBRARY(ROSS ${ross_srcs})
SET_TARGET_PROPERTIES(ROSS PROPERTIES OUTPUT_NAME ROSS)
TARGET_LINK_LIBRARIES(ROSS ${ROSS_EXTERNAL_LIBS})
TARGET_INCLUDE_DIRECTORIES(ROSS INTERFACE ${MPI_C_INCLUDE_PATH})

if (COVERALLS)
    set(COVERAGE_SRCS ${ross_srcs})
    # Don't check coverage on lz4
    list(REMOVE_ITEM COVERAGE_SRCS lz4.h lz4.c)

    # Create the coveralls target.
    coveralls_setup(
        "${COVERAGE_SRCS}" # The source files.
        OFF)               # If we should upload.

endif()

# Build Specific Config Header
CONFIGURE_FILE(config.h.in config.h)
SET(ross_srcs ${ross_srcs} config.h)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/config.h PROPERTIES GENERATED TRUE)


# CODES config bin
SET(ROSS_CC $ENV{CC})
SET(ROSS_CXX $ENV{CXX})
CONFIGURE_FILE(ross-config.in ross-config @ONLY)
CONFIGURE_FILE(ross.pc.in ross.pc @ONLY)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/ross.pc.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/ross.pc PROPERTIES GENERATED TRUE)


# Make Install
INSTALL(FILES ${ROSS_BINARY_DIR}/ross-config DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${ROSS_BINARY_DIR}/config.h DESTINATION include)
INSTALL(DIRECTORY ${ROSS_SOURCE_DIR}/ DESTINATION include FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS ROSS EXPORT ROSS-targets DESTINATION lib)
INSTALL(EXPORT ROSS-targets DESTINATION lib)
INSTALL(FILES ROSSConfig.cmake DESTINATION lib)
INSTALL(FILES ${ROSS_BINARY_DIR}/ross.pc DESTINATION lib/pkgconfig)
