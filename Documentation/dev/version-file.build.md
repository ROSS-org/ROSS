The project version is now read from a tracked `.version` file at the
repo root rather than solely from `git describe`. Tarball,
shallow-clone, and no-tag builds report the correct release version
instead of falling back to a sentinel; `git describe` still augments
the runtime `ROSS_VERSION` string with a commit-count and `-dirty`
suffix on developer builds.
