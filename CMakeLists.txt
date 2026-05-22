cmake_minimum_required(VERSION 3.16)

# Derive the project version from git so it stays in sync with release tags.
# This must run BEFORE project() — project(VERSION ...) needs numeric pieces.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/core/cmake/")
include(GetGitRevisionDescription)
git_describe_working_tree(ROSS_GIT_DESCRIBE --tags --dirty)
message(STATUS "ROSS git describe: ${ROSS_GIT_DESCRIBE}")

string(REGEX REPLACE "^v([0-9]+)\\..*"                "\\1" ROSS_VERSION_MAJOR "${ROSS_GIT_DESCRIBE}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*"          "\\1" ROSS_VERSION_MINOR "${ROSS_GIT_DESCRIBE}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" ROSS_VERSION_PATCH "${ROSS_GIT_DESCRIBE}")
set(ROSS_DETECTED_VERSION "${ROSS_VERSION_MAJOR}.${ROSS_VERSION_MINOR}.${ROSS_VERSION_PATCH}")

# project(VERSION ...) errors on malformed input. Tarball / shallow-clone /
# no-tag builds make git_describe_working_tree return sentinels like
# "GITDIR-NOTFOUND" or "HEAD-HASH-NOTFOUND" that the regexes above leave
# unchanged. Detect that and fall back to 0.0.0.
if(NOT ROSS_DETECTED_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(WARNING
        "Could not derive ROSS version from git (got '${ROSS_GIT_DESCRIBE}'); "
        "falling back to 0.0.0. This is expected for tarball, shallow, or "
        "no-tag builds.")
    set(ROSS_DETECTED_VERSION "0.0.0")
    set(ROSS_GIT_DESCRIBE "0.0.0")
endif()

project(ross
    VERSION ${ROSS_DETECTED_VERSION}
    DESCRIPTION "Rensselaer's Optimistic Simulation System"
    HOMEPAGE_URL "https://github.com/ROSS-org/ROSS"
    LANGUAGES C)

# Polyfill PROJECT_IS_TOP_LEVEL for CMake < 3.21. Drop this block when the
# minimum required CMake version is bumped to 3.21+ — the variable is then
# set by project() automatically.
if(NOT DEFINED PROJECT_IS_TOP_LEVEL)
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(PROJECT_IS_TOP_LEVEL TRUE)
    else()
        set(PROJECT_IS_TOP_LEVEL FALSE)
    endif()
endif()

# Full git-describe string (e.g., "v8.0.0-3-gabcdef-dirty") embedded into
# config.h as ROSS_VERSION; suffix after MAJOR.MINOR.PATCH used historically
# as VERSION_SHA1 (preserved for compatibility — no current consumer reads
# it, but keep the extraction in case config.h.in starts referencing it).
set(VERSION "${ROSS_GIT_DESCRIBE}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${VERSION}")
# VERSION_SHORT is consumed by core/ross.pc.in; In the future it will switch to
# @PROJECT_VERSION@.
set(VERSION_SHORT "${PROJECT_VERSION}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ROSS Configuration Options

ENABLE_TESTING()
INCLUDE(CTest)

# Follow section based on Spack doc:
# https://spack.readthedocs.io/en/latest/workflows.html#write-the-cmake-build
# enable @rpath in the install name for any shared library being built
# note: it is planned that a future version of CMake will enable this by default
set(CMAKE_MACOSX_RPATH 1)

# Always use full RPATH
# http://www.cmake.org/Wiki/CMake_RPATH_handling
# http://www.kitware.com/blog/home/post/510

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

# end of spack-related addition

# Priority Queue Implementation
SET(QUEUE splay)
# Other queue implementations are no longer supported.
# SET(QUEUE splay CACHE STRING "Queue type chosen by the user at configure time")
# SET_PROPERTY(CACHE QUEUE PROPERTY STRINGS splay calendar heap kp_splay)

# Random Library
SET(RAND clcg4)

# Network option
# tcp option removed
SET(NETWORK mpi)

# GVT algorithm option
#SET(GVT 7oclock)
SET(GVT mpi_allreduce)

# Architecture setting and management
SET(VALID_ARCH NO)
OPTION(ROSS_CLOCK_OVERRIDE "override platform detection to use gtod clock" NO)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i386)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK i386)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL i386)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL bgl)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
  SET(OPTIONS "-qtune=440 -qarch=440d")
  SET(CLOCK bgl)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL bgl)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL bgp)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qflag=i:i -qattr=full -O5")
  SET(OPTIONS "-qtune=450 -qarch=450d")
  SET(CLOCK bgl)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL bgp)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL bgq)
  SET(VALID_ARCH YES)
  OPTION(USE_BGPM "Blue Gene/Q specific Performance Counters" OFF)
  SET(BGPM_INCLUDE /bgsys/drivers/ppcfloor/bgpm/include/)
  ADD_LIBRARY(imp_bgpm STATIC IMPORTED)
  SET_PROPERTY(TARGET imp_bgpm PROPERTY IMPORTED_LOCATION /bgsys/drivers/ppcfloor/bgpm/lib/libbgpm.a)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${BGPM_INCLUDE} -O5 -qstrict -qprefetch=aggressive -qarch=qp -qtune=qp -qmaxmem=-1 -qsimd=noauto -qreport -qhot")
  SET(CLOCK bgq)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL bgq)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL ppc64)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK ppc)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL ppc64)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL ppc64le)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O5 -qprefetch=aggressive -qarch=pwr9 -qtune=auto -qmaxmem=-1 -qsimd=noauto -qhot")
  SET(CLOCK ppc64le)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL ppc64le)

IF((${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL amd64))
  SET(VALID_ARCH YES)
  SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK amd64)
ENDIF((${CMAKE_SYSTEM_PROCESSOR} STREQUAL x86_64) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL amd64))

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL aarch64)
  SET(VALID_ARCH YES)
  SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK aarch64)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL aarch64)

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL armv7l)
  SET(VALID_ARCH YES)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  ADD_DEFINITIONS(-D_GNU_SOURCE)
  SET(CLOCK armv7l)
ENDIF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL armv7l)

IF(VALID_ARCH AND NOT ${ROSS_CLOCK_OVERRIDE})
  MESSAGE(STATUS "System architecture detected: ${CMAKE_SYSTEM_PROCESSOR}")
  MESSAGE(STATUS "Using C_FLAGS: ${CMAKE_C_FLAGS}")
  MESSAGE(STATUS "Using CLOCK: ${CLOCK}")
ELSE(VALID_ARCH AND NOT ${ROSS_CLOCK_OVERRIDE})
  MESSAGE(WARNING "System architecture not recognized!\n"
    "Found: ${CMAKE_SYSTEM_PROCESSOR}\n"
    "Falling back to get-time-of-day clock implementation.")
  SET(CLOCK gtod)
ENDIF(VALID_ARCH AND NOT ${ROSS_CLOCK_OVERRIDE})


## MPI
INCLUDE(SetupMPI)
IF(MPI_C_FOUND)
        INCLUDE_DIRECTORIES(${MPI_C_INCLUDE_PATH})
        LIST(APPEND ROSS_EXTERNAL_LIBS ${MPI_C_LIBRARIES})
ELSE(MPI_C_FOUND)
        MESSAGE("WARNING: Could not find MPI!")
        MESSAGE("         Either add an MPI compiler to your path (using modules)")
        MESSAGE("         Or force CMake to build using the correct compiler (`export CC=mpicc`)")
ENDIF(MPI_C_FOUND)

# Code coverage instrumentation (GCC/Clang only). Applied at directory scope
# so flags propagate to core/ and models/. Collect with lcov/gcovr after ctest.
option(ROSS_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
mark_as_advanced(ROSS_ENABLE_COVERAGE)
if(ROSS_ENABLE_COVERAGE)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    message(FATAL_ERROR
      "ROSS_ENABLE_COVERAGE requires GCC or Clang; got ${CMAKE_C_COMPILER_ID}")
  endif()
  message(STATUS "Code coverage enabled (--coverage)")
  add_compile_options(--coverage)
  add_link_options(--coverage)
endif()

# ROSS Core code
ADD_SUBDIRECTORY(core)

# ROSS Models directory
OPTION(ROSS_BUILD_MODELS "Build ROSS models?" OFF)
IF(ROSS_BUILD_MODELS)
  ADD_SUBDIRECTORY(models)
ENDIF(ROSS_BUILD_MODELS)

# ROSS Documentation
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  OPTION(ROSS_BUILD_DOXYGEN "Build Doxygen documentation?" OFF)
  IF(ROSS_BUILD_DOXYGEN)
    ADD_SUBDIRECTORY(docs)
  ENDIF(ROSS_BUILD_DOXYGEN)
ENDIF(DOXYGEN_FOUND)
