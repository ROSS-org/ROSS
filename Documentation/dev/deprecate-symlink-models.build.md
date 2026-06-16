The "symlink your model into ROSS's `models/` directory and let the
in-tree build pick it up" workflow is now deprecated. Configuring with
a symlinked model present produces a CMake deprecation warning at
configure time. The workflow still works for this release; it will be
removed in a future release. New models should be standalone CMake
projects that consume ROSS via `find_package(ROSS)` — see
`models/README.md` for a worked example.
