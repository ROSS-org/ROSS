# Changelog fragments

Pending release notes live here as one file per pull request. At release
time, [`scripts/compile-changelog.sh`](../../scripts/compile-changelog.sh)
concatenates everything in this directory into a new section of the
top-level [`CHANGELOG.md`](../../CHANGELOG.md) and `git rm`s the
fragments.

## Adding a fragment

Every user-visible change should add one file to this directory in the
same PR that introduces the change. Skip only for changes that are
genuinely invisible to anyone outside the PR (e.g., test refactors,
internal renames, comment-only tweaks).

**Filename**: `<short-slug>.<category>.md`

- `<short-slug>` — kebab-case, descriptive of the change, unique within
  the directory. No PR number needed.
- `<category>` — one of:
  - `feature` — new capability or option
  - `bugfix` — fixed incorrect behavior
  - `removal` — removed or renamed a public-facing API, option, or path
  - `build` — build-system / packaging / CMake / install-layout change
  - `misc` — internal cleanup, doc tweaks, anything else worth noting

**Contents**: one or two sentences in past tense, written for someone
upgrading ROSS. Focus on what changed and (briefly) why.

If the change breaks consumers — removed symbol, renamed option, changed
default, install-path move — prefix the entry with `**[Breaking]**`. The
marker stays inline with the entry in its natural category section.

## Examples

`Documentation/dev/version-file.build.md`:

```markdown
The project version is now read from a tracked `.version` file at the
repo root, not from `git describe`. Tarball and shallow-clone builds
report the correct version instead of falling back to a sentinel.
```

`Documentation/dev/mpi-discovery.build.md`:

```markdown
**[Breaking]** MPI is now discovered via `find_package(MPI)` rather
than the legacy `SetupMPI.cmake` helper. Users no longer need to set
`CC=mpicc` or `-DCMAKE_C_COMPILER=mpicc`; non-standard MPI installs are
hinted with `-DMPI_HOME=...` or `module load <mpi>`.
```

`Documentation/dev/coveralls-removal.removal.md`:

```markdown
Removed the dead Coveralls coverage path (`-DCOVERALLS=ON`,
`core/cmake/Coveralls*.cmake`). Coverage is now driven by
`-DROSS_ENABLE_COVERAGE=ON` plus the Codecov GitHub Actions job.
```

## Conventions

- One concept per fragment. Two unrelated changes from the same PR
  should produce two fragments.
- Refer to options, flags, and identifiers in backticks.
- Don't reference PR numbers, issue numbers, or contributor handles
  inside the fragment — those belong in the GitHub release notes the
  maintainer writes from this output. Fragments should still read
  cleanly five years from now.
- Don't quote configure-log output, full error messages, or large code
  blocks. Keep the fragment to a paragraph.
