The install-RPATH stanza at the top of `CMakeLists.txt` is now gated
on `PROJECT_IS_TOP_LEVEL`. When ROSS is consumed via `add_subdirectory()`
or `FetchContent`, it no longer mutates the parent project's
directory-scope `CMAKE_INSTALL_RPATH`, `CMAKE_MACOSX_RPATH`,
`CMAKE_SKIP_BUILD_RPATH`, `CMAKE_BUILD_WITH_INSTALL_RPATH`, or
`CMAKE_INSTALL_RPATH_USE_LINK_PATH`. Top-level builds are unchanged:
the installed binary's RPATH still resolves to `<prefix>/lib`.
