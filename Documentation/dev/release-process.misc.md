Added a release-process workflow built around per-PR changelog
fragments under `Documentation/dev/`. Maintainers compile them into
`CHANGELOG.md` at release time via `scripts/compile-changelog.sh`. See
[Documentation/RELEASE_PROCESS.md](Documentation/RELEASE_PROCESS.md)
for the maintainer side and
[Documentation/dev/README.md](Documentation/dev/README.md) for the
contributor format.
