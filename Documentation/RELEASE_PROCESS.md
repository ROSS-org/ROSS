# ROSS release process

This document captures the release process for ROSS. There is no separate release manager or
formal release branch in active use. Releases happen by tagging `master`.

## Versioning

ROSS follows [Semantic Versioning](https://semver.org/) (adopted at v7.0.0).
Tags use a `v` prefix; the `.version` file does not.

| Bump  | When                                                       | Example         |
|-------|------------------------------------------------------------|-----------------|
| MAJOR | Breaking changes to the model-facing API or wire/checkpoint format | `v8.0.0` |
| MINOR | New features, backwards compatible                          | `v8.1.0`        |
| PATCH | Bug fixes only, no API changes                              | `v8.1.1`        |

## What ships in a release

- A signed-off commit on `master`
- A git tag of the form `vX.Y.Z` pointing at that commit
- A new section in [`CHANGELOG.md`](../CHANGELOG.md), compiled from
  per-PR fragments under [`Documentation/dev/`](dev/)
- A GitHub Release attached to the tag, body lifted from the new
  CHANGELOG section (optionally lightly edited for narrative)
- GitHub's auto-generated source tarball/zip for the tag (no extra
  build artifacts are uploaded today)

There is no published binary and no Doxygen auto-publish today (though there
are plans to add auto-publish for Doxygen docs).

## Changelog fragments

Every user-visible PR adds a fragment file under
[`Documentation/dev/`](dev/) named
`<short-slug>.<category>.md`. Categories are `feature`, `bugfix`,
`removal`, `build`, and `misc`. Breaking changes are marked inline with
a `**[Breaking]**` prefix. See [Documentation/dev/README.md](dev/README.md)
for the full contributor-facing format.

At release time, [`scripts/compile-changelog.sh`](../scripts/compile-changelog.sh)
concatenates the pending fragments into a new section at the top of
`CHANGELOG.md` and `git rm`s the fragments. This is what step 2 of
"Cutting a release" below uses.

## The `.version` file

`.version` at the repo root is the canonical source of truth for the
project version. It contains a single line, `X.Y.Z`, with no `v` prefix and
no trailing whitespace.

The top-level `CMakeLists.txt` reads `.version` before calling `project()`
and uses it for:

- `project(ross VERSION X.Y.Z ...)` — and thus `${ross_VERSION}`,
  `${PROJECT_VERSION}`, etc.
- The `Version:` field in the installed `ross.pc`
- The `ROSS_VERSION` macro embedded into `config.h`

If the working tree is a git checkout, `git describe --tags --dirty` is
also consulted to enrich `ROSS_VERSION` with a commit-count + sha +
`-dirty` suffix on developer builds (e.g., `8.1.1-18-g90cb62f4-dirty`).
Tarball / shallow / no-tag builds simply skip the suffix and report the
plain `X.Y.Z`. Git is never authoritative for the version.

A malformed `.version` (missing, empty, or not matching `^[0-9]+\.[0-9]+\.[0-9]+$`)
fails configure with a `FATAL_ERROR` — there is no silent fallback.

## Cutting a release

`master` is a protected branch — all changes, including the version bump,
land via pull request with CI green. Direct pushes are not allowed.

1. **Make sure `master` is in the desired state.** All PRs intended for
   the release are merged and CI is green on the latest `master` commit.

2. **Open a release PR that bumps `.version` and compiles the changelog:**

   ```bash
   git checkout -b release-8.2.0
   echo "8.2.0" > .version
   ./scripts/compile-changelog.sh 8.2.0
   git add .version            # compile-changelog.sh already staged CHANGELOG.md
                               # and git-rm'd the fragments
   git commit -m "Release v8.2.0"
   git push -u origin release-8.2.0
   ```

   Open the PR against `master`, wait for CI to pass, get a review, and
   merge. The resulting commit on `master` is what gets tagged in the
   next step.

   Review the compiled `CHANGELOG.md` diff before pushing — the script
   produces a usable first draft, but it's worth a copy-edit pass for
   prose flow, especially if many fragments landed in the cycle. The
   GitHub Release body in step 5 can lift the new section verbatim or
   trim further.

3. **Tag the merged bump commit on `master`:**

   ```bash
   git checkout master
   git pull --ff-only origin master
   git tag -a v8.2.0 -m "ROSS v8.2.0"
   git push origin v8.2.0
   ```

   Pulling first ensures the tag lands on the merge/squash commit GitHub
   produced, not on a stale local `master`. Branch protection does not
   apply to tag pushes, so `git push origin v8.2.0` succeeds directly.

   Annotated tags (`-a`) are preferred — they carry the tagger identity
   and date and survive `git describe` more reliably than lightweight
   tags. Historically most ROSS tags are lightweight; only `v8.1.0` is
   annotated. Either works.

4. **Verify the version flows correctly** by building from a clean
   checkout at the tag:

   ```bash
   git checkout v8.2.0
   cmake -S . -B build -DROSS_BUILD_MODELS=ON -DCMAKE_BUILD_TYPE=Debug
   cmake --build build -j
   # Confirm "ROSS version: 8.2.0" in the configure log (no -dirty suffix
   # on a clean checkout at the tag).
   ```

5. **Create the GitHub Release.** Open
   https://github.com/ROSS-org/ROSS/releases/new, choose the tag, and
   copy the new section from `CHANGELOG.md` into the release body. Add
   a one-sentence summary at the top if the changelog content alone
   feels too dry. The release body and CHANGELOG.md should say
   substantively the same thing.

   For pre-changelog releases (v8.1.1 and earlier), GitHub Releases
   were written from scratch with varying detail.

## After the release

- **Notify downstream consumers** if there are breaking changes. The
  primary consumer is [CODES](https://github.com/codes-org/codes), which
  discovers ROSS via `pkg_check_modules(ROSS REQUIRED IMPORTED_TARGET ross)`.
  Anything that affects the install layout (header paths, `ross.pc`
  contents, library filename) is a coordination point.

## Historical patterns (no longer in active use)

- **`release-X.Y.Z` branches:** v7.2.0 and v7.2.1 were prepared on
  dedicated `release-X.Y.Z` branches that were then merged into `master`
  and tagged. v8.x releases skipped this and tagged `master` directly.
  Either pattern is fine; the branch approach is useful when stabilizing
  a release while feature work continues on `master`.
- **`develop` branch:** an `origin/develop` branch exists but hasn't
  driven a release in several years. The current convention is to merge
  feature branches into `master` directly.

## Quick checklist

- [ ] `master` CI green
- [ ] Release PR opened: `.version` bumped, `scripts/compile-changelog.sh`
      run, CHANGELOG diff reviewed, CI green, reviewed, merged
- [ ] Annotated tag `vX.Y.Z` pushed against the merged commit on `master`
- [ ] Configure log shows the new version (no `-dirty` on the clean tag)
- [ ] GitHub Release created, body lifted from CHANGELOG section
- [ ] CODES / downstream consumers notified if anything in the install
      contract changed
