PROJECT(ROSS_TOP C)
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ROSS Configuration Options

ENABLE_TESTING()
INCLUDE(CTest)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/core/cmake/")

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

# We probably don't want this to run on every build.
option(COVERALLS "Generate coveralls data" OFF)

if (COVERALLS)
    include(Coveralls)
    coveralls_turn_on_coverage()
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


# ## MPI
# INCLUDE(SetupMPI)
# IF(MPI_C_FOUND)
#         INCLUDE_DIRECTORIES(${MPI_C_INCLUDE_PATH})
#         LIST(APPEND ROSS_EXTERNAL_LIBS ${MPI_C_LIBRARIES})
# ELSE(MPI_C_FOUND)
#         MESSAGE("WARNING: Could not find MPI!")
#         MESSAGE("         Either add an MPI compiler to your path (using modules)")
#         MESSAGE("         Or force CMake to build using the correct compiler (`export CC=mpicc`)")
# ENDIF(MPI_C_FOUND)

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
