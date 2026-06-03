cmake_minimum_required(VERSION 3.16)

# Canonical version lives in .version at the repo root — bumped at release
# time alongside the matching git tag (e.g., .version = "8.1.1", tag v8.1.1).
# Tarball, shallow-clone, and no-tag builds all get the right answer from
# this file. git-describe is only consulted to enrich the runtime
# ROSS_VERSION string with a commit-count + sha + -dirty suffix on developer
# builds; it is never authoritative.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.version" ROSS_DETECTED_VERSION)
string(STRIP "${ROSS_DETECTED_VERSION}" ROSS_DETECTED_VERSION)
if(NOT ROSS_DETECTED_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
        "Malformed version in .version: '${ROSS_DETECTED_VERSION}'. "
        "Expected semantic version X.Y.Z (no leading 'v').")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(GetGitRevisionDescription)
git_describe_working_tree(ROSS_GIT_DESCRIBE --tags --dirty)
if(ROSS_GIT_DESCRIBE MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)$")
    set(VERSION "${ROSS_DETECTED_VERSION}${CMAKE_MATCH_1}")
else()
    set(VERSION "${ROSS_DETECTED_VERSION}")
endif()
message(STATUS "ROSS version: ${VERSION}")

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

# Suffix after MAJOR.MINOR.PATCH (e.g., "-18-g90cb62f4-dirty") preserved as
# VERSION_SHA1 for legacy compatibility.
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${VERSION}")
# VERSION_SHORT consumed by core/ross.pc.in; In the future it will switch to
# @PROJECT_VERSION@.
set(VERSION_SHORT "${PROJECT_VERSION}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

# ROSS Configuration Options

# RPATH policy is set only when ROSS is the top-level project. When ROSS is
# consumed via add_subdirectory()/FetchContent, the parent project owns
# install-RPATH and the global CMAKE_INSTALL_RPATH; mutating those from a
# subproject would override the parent's choices for every target it builds.
if(PROJECT_IS_TOP_LEVEL)
    # Full install-RPATH recipe; see CMake's own docs for the variables and
    # Spack's CMakePackage page for the downstream-packaging motivation:
    # https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
    # https://spack.readthedocs.io/en/latest/build_systems/cmakepackage.html

    # enable @rpath in the install name for any shared library being built
    # (CMake's default since 3.0 via CMP0042; kept explicit for documentation)
    set(CMAKE_MACOSX_RPATH 1)

    # use, i.e. don't skip the full RPATH for the build tree
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

    # when building, don't use the install RPATH already
    # (but later on when installing)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    # the RPATH to be used when installing, but only if it's not a system directory
    LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
    IF("${isSystemDir}" STREQUAL "-1")
       SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    ENDIF("${isSystemDir}" STREQUAL "-1")

    # end of spack-related addition
endif()

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
# MPI is required; build the system C compiler against MPI::MPI_C rather than
# forcing CC=mpicc. For non-standard installs, hint with -DMPI_HOME=... or
# `module load <mpi>` before configuring.
find_package(MPI REQUIRED COMPONENTS C)

# ROSS's own static/shared knob. Defaults from BUILD_SHARED_LIBS so a
# superbuild parent that sets the standard variable at the top gets ROSS
# to follow the convention; an explicit -DROSS_BUILD_SHARED_LIBS=... is
# the per-ROSS override.
option(ROSS_BUILD_SHARED_LIBS
    "Build ROSS libraries as shared instead of static"
    ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS ${ROSS_BUILD_SHARED_LIBS})

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

# Test suite gating. Project-scoped knob so:
#  - top-level builds preserve today's UX (default ON; `-DBUILD_TESTING=OFF`
#    still suppresses tests by feeding the default)
#  - subdirectory consumption defaults OFF (parent must opt in explicitly)
# Once set, ROSS_BUILD_TESTING is the sole gate on include(CTest) and on
# the add_test() registrations inside ROSS_TEST_SCHEDULERS /
# ROSS_TEST_INSTRUMENTATION (see models/CMakeLists.txt).
if(PROJECT_IS_TOP_LEVEL)
    if(DEFINED BUILD_TESTING)
        set(_ross_build_testing_default ${BUILD_TESTING})
    else()
        set(_ross_build_testing_default ON)
    endif()
    option(ROSS_BUILD_TESTING "Build ROSS test suite" ${_ross_build_testing_default})
    unset(_ross_build_testing_default)
else()
    option(ROSS_BUILD_TESTING "Build ROSS test suite" OFF)
endif()

if(ROSS_BUILD_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()

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
