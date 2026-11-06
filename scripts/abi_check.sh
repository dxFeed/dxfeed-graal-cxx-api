#!/bin/bash
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Catches an incompatible change of the ABI of the shared library within a major version before a release is
# published. The ABI is compared with libabigail (abidw, abidiff) on the types and functions of the public headers.
#
# Usage:
#   abi_check.sh dump <library> <include dir> <out.xml>
#       Writes the ABI of <library> as XML (abidw). <library> is a shared library with debug info (RelWithDebInfo);
#       only the declarations of the headers in <include dir> count. The release publishes this file as the asset
#       abi-x86_64-linux.xml.
#   abi_check.sh baseline <major> <out.xml> [excluded tag]
#       Takes the asset abi-x86_64-linux.xml of the latest published release of the major (a tag vMAJOR.MINOR.PATCH,
#       not a draft, not a pre-release; the highest version that has the asset) from the GitHub releases of this
#       repository (gh; GITHUB_REPOSITORY and GH_TOKEN/GITHUB_TOKEN in CI). Without such a release: a notice, no file,
#       exit code 0. Any other failure (no gh, network, authentication) fails.
#   abi_check.sh compare <reference.xml> <library> <include dir>
#       Dumps <library> as `dump` does and compares it with the reference (abidiff, abi/abidiff.suppr).
#   abi_check.sh check <major> <library> <include dir> [excluded tag]
#       baseline + compare: the notice and exit code 0 without a release to compare with.
#   abi_check.sh major [CMakeLists.txt]
#       Prints the major version of the default DXFCXX_VERSION of CMakeLists.txt.
#
# Exit codes of abidiff and what compare does with them (bits):
#   1 error, 2 usage error  -> fail (the XML is broken, or abidiff could not read something);
#   8 incompatible change   -> fail (a removed or changed public function or type, a changed layout, a new virtual
#                              function of a polymorphic class, ...); the report is printed;
#   4 change (without 8)    -> pass (additions: a new function or class, and other compatible changes); the report
#                              is printed; but fail if the report shows a changed layout of a type (size, data
#                              members, bases): libabigail calls it compatible for classes seen only behind
#                              pointers, while here applications create and copy the public classes;
#   0                       -> pass.
# The report is also appended to $GITHUB_STEP_SUMMARY when it is set.
#
# Environment: ABI_ASSET (the asset name, default abi-x86_64-linux.xml), ABI_SUPPRESSIONS (default abi/abidiff.suppr
# next to this script's repository root), ABI_CURRENT_XML (keep the XML of the compared library there).

set -euo pipefail

ASSET="${ABI_ASSET:-abi-x86_64-linux.xml}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SUPPRESSIONS="${ABI_SUPPRESSIONS:-$ROOT/abi/abidiff.suppr}"

WORK=""
cleanup() {
    [ -z "$WORK" ] || rm -rf "$WORK"
}
trap cleanup EXIT

mkwork() {
    [ -n "$WORK" ] || WORK="$(mktemp -d)"
}

die() {
    echo "::error::abi_check: $*" >&2
    exit 1
}

notice() {
    if [ -n "${GITHUB_ACTIONS:-}" ]; then
        echo "::notice::abi_check: $*"
    else
        echo "notice: $*"
    fi
}

require_tool() {
    command -v "$1" > /dev/null ||
        die "$1 not found. Install libabigail (Oracle Linux 8: dnf -y install oracle-epel-release-el8 &&" \
            "dnf -y install libabigail; Ubuntu: apt-get install abigail-tools)."
}

usage() {
    sed -n '/^# Usage:/,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//' >&2
    exit 2
}

# The XML must be an ABI corpus, not an empty or truncated file.
check_xml() {
    local file="$1"
    [ -s "$file" ] || die "$file is missing or empty"
    head -c 4096 "$file" | grep -q '<abi-corpus' || die "$file is not an ABI XML of libabigail (no <abi-corpus>)"
    grep -q '</abi-corpus>' "$file" || die "$file is truncated (no </abi-corpus>)"
}

cmd_dump() {
    local library="${1:?library}" include="${2:?include dir}" out="${3:?out.xml}"
    require_tool abidw
    [ -f "$library" ] || die "library $library not found"
    [ -d "$include" ] || die "include directory $include not found"
    abidw --version >&2
    # --no-corpus-path: the path of the library is not part of the file (the reference and the current dump are made
    # in different directories). --drop-private-types: only the types of the public headers (--headers-dir) count.
    abidw --no-corpus-path --drop-private-types --headers-dir "$include" --out-file "$out" "$library" ||
        die "abidw failed on $library"
    check_xml "$out"
    # A library without debug info gives symbols without types: the check would see only functions' names.
    grep -q '<class-decl' "$out" || die "$out has no types: is $library built with debug info (RelWithDebInfo)?"
    grep -q '<function-decl' "$out" || die "$out has no functions: is $library built with debug info (RelWithDebInfo)?"
    echo "ABI of $library written to $out ($(wc -c < "$out") bytes)"
}

cmd_baseline() {
    local major="${1:?major}" out="${2:?out.xml}" exclude="${3:-}"
    [[ "$major" =~ ^[0-9]+$ ]] || die "major must be a number: $major"
    require_tool gh
    local repo="${GITHUB_REPOSITORY:-}"
    if [ -z "$repo" ]; then
        repo="$(gh repo view --json nameWithOwner --jq .nameWithOwner)" ||
            die "cannot find the repository (set GITHUB_REPOSITORY)"
    fi
    rm -f "$out"

    # Published stable releases of the major, the highest version first. A pre-release (-rc, -beta) may change its
    # ABI before the release, a draft is not published. Each key is reversed by its own r: a key with its own n does
    # not take a global -r.
    local tags
    tags="$(gh release list --repo "$repo" --limit 1000 --json tagName,isDraft,isPrerelease \
        --jq '.[] | select(.isDraft == false and .isPrerelease == false) | .tagName')" ||
        die "gh release list failed for $repo"
    tags="$(printf '%s\n' "$tags" | grep -E "^v${major}\.[0-9]+\.[0-9]+$" | sort -t. -k1.2,1nr -k2,2nr -k3,3nr || true)"

    local tag assets
    for tag in $tags; do
        [ "$tag" = "$exclude" ] && continue
        assets="$(gh release view "$tag" --repo "$repo" --json assets --jq '.assets[].name')" ||
            die "gh release view $tag failed"
        if printf '%s\n' "$assets" | grep -qxF "$ASSET"; then
            gh release download "$tag" --repo "$repo" --pattern "$ASSET" --output "$out" ||
                die "gh release download $tag failed"
            check_xml "$out"
            echo "baseline: $ASSET of the release $tag -> $out"
            return 0
        fi
        echo "baseline: the release $tag has no $ASSET, skipped"
    done
    notice "no published release of the major $major with $ASSET: nothing to compare the ABI with"
    return 0
}

cmd_compare() {
    local reference="${1:?reference.xml}" library="${2:?library}" include="${3:?include dir}"
    require_tool abidiff
    [ -f "$SUPPRESSIONS" ] || die "suppressions $SUPPRESSIONS not found"
    check_xml "$reference"

    local current report
    mkwork
    current="${ABI_CURRENT_XML:-$WORK/current.xml}"
    cmd_dump "$library" "$include" "$current"
    abidiff --version >&2

    # Two XML files, made by the same abidw call: the comparison does not depend on how abidiff reads the library.
    report="$WORK/report.txt"
    local rc=0
    abidiff --suppressions "$SUPPRESSIONS" "$reference" "$current" > "$report" 2>&1 || rc=$?
    cat "$report"

    # libabigail does not count the change of the layout of a class as incompatible when the class is only seen behind
    # pointers in the exported functions (a data member added to a class, its size changed). Here the classes are
    # created and copied by the applications (inline std::make_shared, values), so every change of the layout of a
    # public type is incompatible.
    local layout=0 layout_re
    layout_re='type size changed from|[0-9]+ data members? (insertion|deletion|change)'
    layout_re+='|offset changed from|[0-9]+ base class'
    if ((rc & 4)) && ! ((rc & 8)) && grep -Eq "$layout_re" "$report"; then
        layout=1
    fi

    local verdict
    if ((rc & 3)); then
        verdict="FAIL: abidiff reported an error (exit code $rc): the ABI was not compared"
    elif ((rc & 8)); then
        verdict="FAIL: incompatible ABI change (exit code $rc): the major version must change or the change be undone"
    elif ((layout)); then
        verdict="FAIL: the layout of a public type changed (exit code $rc): change the major version or undo it"
    elif ((rc & 4)); then
        verdict="PASS: compatible ABI changes only, additions (exit code $rc)"
    else
        verdict="PASS: the ABI is unchanged (exit code $rc)"
    fi
    echo "$verdict"

    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        {
            echo "### ABI check"
            echo
            echo "$verdict"
            if [ -s "$report" ]; then
                echo
                echo '```'
                cat "$report"
                echo '```'
            fi
        } >> "$GITHUB_STEP_SUMMARY"
    fi

    if [[ "$verdict" == FAIL* ]]; then
        echo "::error::abi_check: ${verdict#FAIL: }" >&2
        return 1
    fi
    return 0
}

cmd_check() {
    local major="${1:?major}" library="${2:?library}" include="${3:?include dir}" exclude="${4:-}"
    mkwork
    cmd_baseline "$major" "$WORK/reference.xml" "$exclude"
    if [ -f "$WORK/reference.xml" ]; then
        cmd_compare "$WORK/reference.xml" "$library" "$include"
    fi
}

cmd_major() {
    local cmake="${1:-$ROOT/CMakeLists.txt}" version
    version="$(sed -n 's/^set(DXFCXX_VERSION "v\{0,1\}\([0-9][0-9]*\)\..*".*/\1/p' "$cmake" | head -n 1)"
    [ -n "$version" ] || die "DXFCXX_VERSION not found in $cmake"
    echo "$version"
}

command="${1:-}"
[ $# -gt 0 ] && shift
case "$command" in
    dump) cmd_dump "$@" ;;
    baseline) cmd_baseline "$@" ;;
    compare) cmd_compare "$@" ;;
    check) cmd_check "$@" ;;
    major) cmd_major "$@" ;;
    *) usage ;;
esac
