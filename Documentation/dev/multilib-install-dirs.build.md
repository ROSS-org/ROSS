All install destinations now honor `GNUInstallDirs`: the library,
pkg-config file, CMake config files, and headers use
`${CMAKE_INSTALL_LIBDIR}` / `${CMAKE_INSTALL_INCLUDEDIR}`, and the
phold binaries use `${CMAKE_INSTALL_BINDIR}`. On multilib systems
(RHEL/Fedora-style `lib64`), these artifacts install under
`<prefix>/lib64/` instead of being forced to `<prefix>/lib/`;
Spack-style per-component install-dir overrides also work. The
install-RPATH stanza was likewise updated to use
`${CMAKE_INSTALL_LIBDIR}` so installed binaries resolve `libROSS`
through the correct multilib directory.
