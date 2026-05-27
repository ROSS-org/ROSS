SET(ross_srcs

queue/tw-queue.h
queue/${QUEUE}.c

ross-random.h
ross-random-internal.h
ross-random-internal.c
rand-${RAND}.h
rand-${RAND}.c

clock/${CLOCK}.h
clock/${CLOCK}.c

ross-gvt.h
ross-gvt-internal.h
gvt/${GVT}.h
gvt/${GVT}.c

ross-network.h
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

ross-base.h
ross-inline.h
ross-kernel-inline.h
ross.h
ross-types.h
ross-clock.h

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
instrumentation/ross-lps/specialized-lps.c

check-revent/crv-state.h
check-revent/crv-state.c
)

# ROSS VERSION INFORMATION
## Print Date and Time at top of ROSS output
INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(ctime HAVE_CTIME)

# Data Structure for remote Events
# If AVL_TREE is OFF, ROSS reverts to hashing
OPTION(AVL_TREE "Use AVL trees for optimistic mode events? (hash tables otherwise)" ON)
IF(AVL_TREE)
  SET(ross_srcs ${ross_srcs} avl_tree.h avl_tree.c)
ENDIF(AVL_TREE)

# RIO: Restart IO
OPTION(USE_RIO "Enable RIO checkpointing library?" OFF)
IF(USE_RIO)
  SET(ross_srcs ${ross_srcs} rio/io-mpi.c rio/io-serialize.c)
  INCLUDE_DIRECTORIES(rio)
ENDIF(USE_RIO)

# Use deterministic unbiased RNG tiebreaker for event ties
OPTION(USE_RAND_TIEBREAKER "Build with deterministic unbiased tiebreaker for event ties" ON)

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
# ROSS_BUILD_SHARED_LIBS, which propagates here as BUILD_SHARED_LIBS.
add_library(ROSS ${ross_srcs})
add_library(ROSS::ROSS ALIAS ROSS)

# MPI is PUBLIC because core/ross.h does `#include <mpi.h>` — consumers must
# inherit the MPI include path and link against MPI when they link ROSS.
target_link_libraries(ROSS PUBLIC MPI::MPI_C)
target_link_libraries(ROSS PRIVATE m)

# Public include dirs travel with the target:
#   BUILD_INTERFACE — in-tree models find both
#     the source dir (ross.h, ross-base.h, etc.) and the binary dir
#     (generated config.h).
#   INSTALL_INTERFACE — installed consumers find headers at <prefix>/include/ross
target_include_directories(ROSS
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/ross>
)

# Build Specific Config Header
CONFIGURE_FILE(config.h.in config.h)
SET(ross_srcs ${ross_srcs} config.h)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/config.h PROPERTIES GENERATED TRUE)


# CODES config bin
STRING(REPLACE " " "\\ " CMAKE_INSTALL_PREFIX_ESCAPED "\"${CMAKE_INSTALL_PREFIX}\"")
CONFIGURE_FILE(ross.pc.in ross.pc @ONLY)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/ross.pc.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/ross.pc PROPERTIES GENERATED TRUE)


# Install
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION include)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION include FILES_MATCHING PATTERN "*.h")

# Library target + namespaced export.
install(TARGETS ROSS
    EXPORT ROSSTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT ROSSTargets
    FILE ROSSTargets.cmake
    NAMESPACE ROSS::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ROSS)

# Canonical ROSSConfig.cmake + ROSSConfigVersion.cmake.
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/ROSSConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ROSSConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ROSS)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ROSSConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ROSSConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ROSSConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ROSS)

# Build-tree export so consumers can use
# find_package(ROSS) against the build dir without installing.
export(EXPORT ROSSTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/ROSSTargets.cmake
    NAMESPACE ROSS::)

# Legacy-location shim at <prefix>/${CMAKE_INSTALL_LIBDIR}/ROSSConfig.cmake
# delegates to the canonical config above. Catches users still passing
# -DROSS_DIR=<prefix>/lib. RENAME because both files are named ROSSConfig.cmake
# but live in different directories.
install(FILES ${CMAKE_SOURCE_DIR}/cmake/ROSSConfig-legacy-shim.cmake
    RENAME ROSSConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR})

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ross.pc DESTINATION lib/pkgconfig)
