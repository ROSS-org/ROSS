Added a project-scoped `ROSS_BUILD_TESTING` option that gates
`include(CTest)` and the test-registration helpers. Defaults to
`BUILD_TESTING`'s value when ROSS is the top-level project, so
`cmake -S . -B build` still runs tests and `-DBUILD_TESTING=OFF` still
suppresses them. Defaults `OFF` when ROSS is consumed via
`add_subdirectory()` or `FetchContent`, so a parent project's test
suite is no longer mixed with ROSS's by default — parents that want
ROSS's tests opt in with `-DROSS_BUILD_TESTING=ON`. Phold binaries
still build under `-DROSS_BUILD_TESTING=OFF -DROSS_BUILD_MODELS=ON`;
only test registration is suppressed.
