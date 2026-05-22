Coverage tracking moved from the dead Coveralls path to Codecov. The
`-DCOVERALLS=ON` option and the `Coveralls*.cmake` helpers under
`core/cmake/` are gone; a new `-DROSS_ENABLE_COVERAGE=ON` opt-in adds
`--coverage` to the build, and a dedicated coverage job in the GitHub
Actions workflow uploads results to Codecov.
