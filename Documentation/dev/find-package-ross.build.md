`find_package(ROSS REQUIRED)` is now a fully-functional CMake package,
exporting the `ROSS::ROSS` imported target with include directories,
the MPI dependency (re-resolved on the consumer's machine via
`find_dependency(MPI)`), and link libraries propagated transparently.
The canonical config installs at `<prefix>/lib/cmake/ROSS/`, which
CMake discovers from `CMAKE_PREFIX_PATH=<prefix>` without any explicit
hint; a generated `ROSSConfigVersion.cmake` enables version-constrained
discovery (`find_package(ROSS 8.0)`).

Callers passing `-DROSS_DIR=<prefix>/lib` (master's install location)
keep working through a deprecation shim that prints a warning and
delegates to the canonical config; legacy `target_link_libraries(... ROSS)`
callers keep working through a back-compat ALIAS shipped inside
`ROSSConfig.cmake`. Both shims will be removed in a future release.
