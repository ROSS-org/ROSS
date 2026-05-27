# ROSSConfig.cmake — legacy-location shim
#
# Installed at <prefix>/${CMAKE_INSTALL_LIBDIR}/ROSSConfig.cmake to preserve
# compatibility with callers that hint find_package(ROSS) via
# -DROSS_DIR=<prefix>/lib (where master's hand-written ROSSConfig.cmake used
# to live). The canonical config now ships at
# <prefix>/${CMAKE_INSTALL_LIBDIR}/cmake/ROSS/ROSSConfig.cmake, which CMake
# discovers automatically from CMAKE_PREFIX_PATH.

message(DEPRECATION
    "find_package(ROSS) via -DROSS_DIR=<prefix>/lib is deprecated. "
    "ROSSConfig.cmake now installs at <prefix>/lib/cmake/ROSS/; drop the "
    "-DROSS_DIR hint and CMake will find it via CMAKE_PREFIX_PATH=<prefix>. "
    "This shim will be removed in a future ROSS release.")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/ROSS/ROSSConfig.cmake)

# The previously hand-written ROSSConfig.cmake set ROSS_INCLUDE_DIRS. The new
# canonical config deliberately does NOT (imported-targets-only is the modern
# convention), but legacy callers reading ${ROSS_INCLUDE_DIRS} directly keep
# working through this shim. Derived from the target so it always matches the
# real install path.
get_target_property(_ross_inc_dirs ROSS::ROSS INTERFACE_INCLUDE_DIRECTORIES)
set(ROSS_INCLUDE_DIRS ${_ross_inc_dirs})
unset(_ross_inc_dirs)
