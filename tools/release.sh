#!/usr/bin/env bash
# Bumps the version in VersionNo.h, commits it, tags v<version> and pushes both to origin.
# The pushed tag starts .github/workflows/release.yml, which builds and publishes the release.
#
# Usage:  tools/release.sh [build|minor|major|X.Y.Z] [-y] [-n]
#   build   (default) 0.98.973 -> 0.98.974
#   minor   0.98.973 -> 0.99.0
#   major   0.98.973 -> 1.0.0
#   X.Y.Z   set an explicit version (must be higher than the current one)
#   -y      don't ask for confirmation
#   -n      dry run: show what would happen, change nothing
#
# Runs on macOS/Linux and in Git Bash on Windows. Needs git and perl; uses gh (if installed)
# to follow the workflow run.
set -euo pipefail

bump=build yes=0 dry=0
for arg in "$@"; do
    case "$arg" in
        -y) yes=1 ;;
        -n) dry=1 ;;
        -h|--help) sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        build|minor|major|[0-9]*.[0-9]*.[0-9]*) bump=$arg ;;
        *) echo "Unknown argument: $arg (see --help)" >&2; exit 2 ;;
    esac
done

die() { echo "error: $*" >&2; exit 1; }

cd "$(git rev-parse --show-toplevel)"
file=VersionNo.h

get() { perl -ne "print \$1 if /^#define\s+$1\s+(\d+)/" "$file"; }
major=$(get MAJOR_VERSION) minor=$(get MINOR_VERSION) build=$(get BUILD_VERSION)
[[ -n $major && -n $minor && -n $build ]] || die "could not read the version from $file"
current="$major.$minor.$build"

case "$bump" in
    build) new="$major.$minor.$((build + 1))" ;;
    minor) new="$major.$((minor + 1)).0" ;;
    major) new="$((major + 1)).0.0" ;;
    *)
        [[ $bump =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "bad version: $bump"
        new=$bump
        # The MSI only upgrades to a higher version.
        [[ $(printf '%s\n%s\n' "$current" "$new" | sort -t. -k1,1n -k2,2n -k3,3n | tail -1) == "$new" && $new != "$current" ]] \
            || die "$new is not higher than the current version $current"
        ;;
esac
IFS=. read -r nmajor nminor nbuild <<< "$new"
tag="v$new"

# Release only what is on origin/main, from a clean tree.
branch=$(git symbolic-ref --short HEAD 2>/dev/null || true)
[[ $branch == main ]] || die "not on main (on '${branch:-detached HEAD}')"
[[ -z $(git status --porcelain) ]] || die "the working tree has uncommitted changes"
git fetch -q --tags origin main
[[ $(git rev-parse HEAD) == $(git rev-parse origin/main) ]] \
    || die "main is not in sync with origin/main (pull or push first)"
git rev-parse -q --verify "refs/tags/$tag" >/dev/null && die "tag $tag already exists"

echo "Version: $current -> $new"
echo "Will commit $file, tag $tag and push main + $tag to origin (starts the Release workflow)."
if (( dry )); then echo "(dry run, nothing changed)"; exit 0; fi
if (( ! yes )); then
    read -r -p "Continue? [y/N] " answer
    [[ $answer == [yY]* ]] || { echo "Aborted."; exit 1; }
fi

# Edit in place, keeping the file's CRLF line endings.
perl -pi -e "
    s/^(#define\s+MAJOR_VERSION\s+)\d+/\${1}$nmajor/;
    s/^(#define\s+MINOR_VERSION\s+)\d+/\${1}$nminor/;
    s/^(#define\s+BUILD_VERSION\s+)\d+/\${1}$nbuild/;
    s/^(#define\s+SUB_BUILD_VERSION\s+)\d+/\${1}0/;
    s/^(#define\s+STRFILEVER\s+)\"[^\"]*\"/\${1}\"$nmajor, $nminor, $nbuild, 0\"/;
" "$file"
[[ $(get BUILD_VERSION) == "$nbuild" && $(get MINOR_VERSION) == "$nminor" ]] || die "failed to update $file"

git add "$file"
git commit -q -m "Version $new"
git tag -a "$tag" -m "Jaangle $new"
# Atomic: either both main and the tag reach origin, or neither does.
if ! git push -q --atomic origin main "$tag"; then
    echo "Push failed. Undo locally with: git tag -d $tag && git reset --hard HEAD~1" >&2
    exit 1
fi
echo "Pushed $tag."

repo_url=$(git remote get-url origin | sed -E 's#^git@github.com:#https://github.com/#; s#\.git$##')
echo "Workflow: $repo_url/actions/workflows/release.yml"
echo "Release (when done): $repo_url/releases/tag/$tag"

if command -v gh >/dev/null 2>&1; then
    # Give GitHub a moment to register the run started by the tag push.
    for _ in 1 2 3 4 5 6; do
        run=$(gh run list --workflow release.yml --branch "$tag" --limit 1 --json databaseId --jq '.[0].databaseId' 2>/dev/null || true)
        [[ -n $run ]] && break
        sleep 5
    done
    if [[ -n ${run:-} ]]; then
        gh run watch "$run" --exit-status
    fi
fi
