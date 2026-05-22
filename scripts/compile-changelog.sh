#!/usr/bin/env bash
#
# compile-changelog.sh — compile Documentation/dev/*.md fragments into a new
# CHANGELOG.md section, then git-rm the fragments. Run as part of the release
# PR. See Documentation/dev/README.md for the fragment format.
#
set -euo pipefail

usage() {
    cat <<EOF
Usage: $(basename "$0") <version> [--date YYYY-MM-DD] [--dry-run]

Compiles changelog fragments under Documentation/dev/ into a new section
prepended to CHANGELOG.md, then 'git rm's the fragments.

Arguments:
    <version>       Semantic version (X.Y.Z) for the new release.

Options:
    --date DATE     Override the release date (default: today, UTC).
    --dry-run       Print the proposed section to stdout without
                    modifying any files.
EOF
}

VERSION=""
DATE="$(date -u +%Y-%m-%d)"
DRY_RUN=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run) DRY_RUN=1; shift ;;
        --date)    DATE="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        --)        shift; break ;;
        -*)        echo "Error: unknown option '$1'" >&2; usage >&2; exit 1 ;;
        *)
            if [[ -n "$VERSION" ]]; then
                echo "Error: unexpected argument '$1'" >&2
                usage >&2
                exit 1
            fi
            VERSION="$1"
            shift
            ;;
    esac
done

if [[ -z "$VERSION" ]]; then
    echo "Error: version argument required" >&2
    usage >&2
    exit 1
fi
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Error: version must be X.Y.Z, got '$VERSION'" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FRAGMENT_DIR="$REPO_ROOT/Documentation/dev"
CHANGELOG="$REPO_ROOT/CHANGELOG.md"

[[ -d "$FRAGMENT_DIR" ]] || { echo "Error: $FRAGMENT_DIR not found" >&2; exit 1; }
[[ -f "$CHANGELOG"   ]] || { echo "Error: $CHANGELOG not found"   >&2; exit 1; }

CATEGORIES=(feature bugfix removal build misc)
HEADERS=("Features" "Bug fixes" "Removals" "Build & packaging" "Misc")

# Validation pass: warn on fragments whose suffix isn't a recognized category.
unrecognized=()
shopt -s nullglob
for f in "$FRAGMENT_DIR"/*.md; do
    base=$(basename "$f")
    [[ "$base" == "README.md" ]] && continue
    name="${base%.md}"
    category="${name##*.}"
    valid=0
    for c in "${CATEGORIES[@]}"; do
        [[ "$c" == "$category" ]] && { valid=1; break; }
    done
    [[ $valid -eq 0 ]] && unrecognized+=("$base")
done
shopt -u nullglob

if [[ ${#unrecognized[@]} -gt 0 ]]; then
    echo "Warning: skipping fragments with unrecognized category:" >&2
    for u in "${unrecognized[@]}"; do echo "    $u" >&2; done
    echo "(Valid categories: ${CATEGORIES[*]})" >&2
fi

# Bail if nothing matched any recognized category.
total=0
shopt -s nullglob
for c in "${CATEGORIES[@]}"; do
    for f in "$FRAGMENT_DIR"/*.${c}.md; do
        total=$((total+1))
    done
done
shopt -u nullglob
if [[ $total -eq 0 ]]; then
    echo "Error: no usable fragments found in $FRAGMENT_DIR" >&2
    exit 1
fi

# Render one fragment file as a markdown bullet: first non-empty line gets the
# "- " prefix; subsequent lines are indented by two spaces (markdown bullet
# continuation). Leading and trailing blank lines are trimmed.
fragment_to_bullet() {
    awk '
        { lines[++n] = $0 }
        END {
            start = 0; for (i = 1; i <= n; i++) if (lines[i] != "") { start = i; break }
            end   = 0; for (i = n; i >= 1; i--) if (lines[i] != "") { end   = i; break }
            if (start == 0) exit
            for (i = start; i <= end; i++) {
                if (i == start)         printf("- %s\n", lines[i])
                else if (lines[i] == "") printf("\n")
                else                     printf("  %s\n", lines[i])
            }
        }
    ' "$1"
}

section=$(mktemp)
out=$(mktemp)
trap 'rm -f "$section" "$out"' EXIT

{
    printf '## v%s — %s\n\n' "$VERSION" "$DATE"
    for i in "${!CATEGORIES[@]}"; do
        c="${CATEGORIES[$i]}"
        h="${HEADERS[$i]}"
        shopt -s nullglob
        files=( "$FRAGMENT_DIR"/*.${c}.md )
        shopt -u nullglob
        [[ ${#files[@]} -eq 0 ]] && continue
        printf '### %s\n\n' "$h"
        # Sort by basename so the order is stable across filesystems.
        for f in $(printf '%s\n' "${files[@]}" | sort); do
            fragment_to_bullet "$f"
        done
        printf '\n'
    done
} > "$section"

if [[ $DRY_RUN -eq 1 ]]; then
    cat "$section"
    exit 0
fi

# Prepend the new section to CHANGELOG.md, before the first existing '## v'
# section header. If no prior versioned section exists, append at the end with
# a blank-line separator from the intro.
awk -v section_file="$section" '
    BEGIN { inserted = 0 }
    !inserted && /^## v/ {
        while ((getline line < section_file) > 0) print line
        close(section_file)
        inserted = 1
    }
    { print }
    END {
        if (!inserted) {
            print ""
            while ((getline line < section_file) > 0) print line
            close(section_file)
        }
    }
' "$CHANGELOG" > "$out"
mv "$out" "$CHANGELOG"

# Stage CHANGELOG.md and remove the fragments. Tracked fragments go through
# 'git rm' so the deletion appears in the release PR diff; untracked fragments
# (new in this release cycle) are just deleted from disk.
git add "$CHANGELOG"
for c in "${CATEGORIES[@]}"; do
    shopt -s nullglob
    for f in "$FRAGMENT_DIR"/*.${c}.md; do
        if git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
            git rm --quiet "$f"
        else
            rm -f "$f"
        fi
    done
    shopt -u nullglob
done

echo "Compiled changelog for v${VERSION}. Review the diff before pushing the release PR."
