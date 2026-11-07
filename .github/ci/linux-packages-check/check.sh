#!/bin/bash
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Checks that the unpacked Linux packages load on the system it runs on (an older distribution in a container), as
# users run them, without LD_LIBRARY_PATH:
# - ldd finds every library and symbol version that the libraries and executables of the packages need (the RUNPATH
#   of the executables in bin/ reaches lib/ of their package);
# - the versioned shared library is a chain of symbolic links: lib<name>.so -> lib<name>.so.<major> -> the file;
# - the samples are directly in bin/, with one dxfeed.system.properties;
# - WriteTapeFileSample starts the Graal isolate and writes a tape file; ConvertTapeFileSample and ScheduleSample run
#   on the data files installed next to them.
#
# Usage: check.sh <directory with the unpacked packages>
#
# Every package is a directory with lib/ and/or bin/.

set -u
shopt -s globstar nullglob

root="${1:?usage: check.sh <directory with the unpacked packages>}"
failed=0
checked=0

fail() {
    echo "FAIL: $*"
    failed=$((failed + 1))
}

head -n 1 /etc/os-release 2>/dev/null
ldd --version | head -n 1

# The UI sample links libOpenGL.so.0 and libGLX.so.0 (libglvnd), which the minimal images lack; they are system
# libraries, not parts of the packages.
if command -v apt-get > /dev/null; then
    apt-get -qq update > /dev/null &&
        DEBIAN_FRONTEND=noninteractive apt-get -qq install -y libopengl0 libglx0 > /dev/null
elif command -v microdnf > /dev/null; then
    microdnf -y install libglvnd-opengl libglvnd-glx > /dev/null
fi

for package in "$root"/*/; do
    package="${package%/}"
    for file in "$package"/**/*; do
        [[ -f "$file" && ! -L "$file" ]] || continue
        # ELF files only: the packages also have headers, scripts, properties and texts.
        [[ "$(head -c 4 "$file" | od -An -tx1 | tr -d ' \n')" == "7f454c46" ]] || continue
        # Static libraries and object files are not ELF on their own (ar archives), shared objects and
        # executables are; ldd prints "not a dynamic executable" for a static executable.
        # Any other failure of ldd (no ldd, a file it cannot run) fails too, rather than counting the file as checked.
        if output="$(env -u LD_LIBRARY_PATH ldd "$file" 2>&1)"; then
            missing="$(echo "$output" | grep 'not found')"
        elif [[ "$output" == *"not a dynamic executable"* ]]; then
            missing=""
        else
            missing="ldd failed: $output"
        fi
        checked=$((checked + 1))
        if [[ -n "$missing" ]]; then
            fail "${file#"$root"/}"
            echo "$missing" | sed 's/^/    /'
        fi
    done
done

echo "ldd: $checked files checked, $failed with missing libraries or symbol versions"

# The versioned shared library of the C++ API: the name the linker takes is a link to the SONAME, which is a link to
# the file.
links=0
for so in "$root"/*/lib/libdxFeedGraalCxxApi.so; do
    links=1
    soname="$(readlink "$so")"
    if [[ ! -L "$so" || ! "$soname" =~ ^libdxFeedGraalCxxApi\.so\.[0-9]+$ ]]; then
        fail "${so#"$root"/} is not a link to libdxFeedGraalCxxApi.so.<major>"
    elif [[ ! -L "$(dirname "$so")/$soname" || ! -f "$so" ]]; then
        fail "${so#"$root"/}: $soname is not a link to the library file"
    else
        echo "${so#"$root"/} -> $soname -> $(readlink "$(dirname "$so")/$soname")"
    fi
done
[[ "$links" -eq 1 ]] || fail "no lib/libdxFeedGraalCxxApi.so under $root"

samples=""
for sample in "$root"/*/bin/WriteTapeFileSample; do
    [[ -f "$sample" ]] && samples="$(dirname "$sample")" && break
done

if [[ -z "$samples" ]]; then
    fail "no bin/WriteTapeFileSample under $root"
else
    nested="$(find "$samples" -mindepth 2 -type f | head -n 5)"
    [[ -z "$nested" ]] || fail "files in subdirectories of ${samples#"$root"/}:"$'\n'"$nested"
    [[ -f "$samples/dxfeed.system.properties" ]] || fail "no dxfeed.system.properties in ${samples#"$root"/}"

    # The packages are read-only: the samples write into a temporary directory.
    work="$(mktemp -d)"
    if (cd "$work" && timeout 120 "$samples/WriteTapeFileSample") && grep -q 'TEST1' "$work/WriteTapeFile.out.txt"; then
        echo "WriteTapeFileSample: wrote $work/WriteTapeFile.out.txt"
    else
        fail "WriteTapeFileSample did not write the tape file"
    fi

    if (cd "$work" && timeout 120 "$samples/ConvertTapeFileSample" \
        "file:$samples/ConvertTapeFile.in[readAs=stream_data,speed=max]" \
        "tape:ConvertTapeFile.out[saveAs=stream_data,format=text]") && [[ -s "$work/ConvertTapeFile.out" ]]; then
        echo "ConvertTapeFileSample: converted bin/ConvertTapeFile.in"
    else
        fail "ConvertTapeFileSample did not convert bin/ConvertTapeFile.in"
    fi

    if (cd "$work" && timeout 120 "$samples/ScheduleSample" "$samples/schedule.properties" "$samples/sample.ipf.zip" \
        IBM 2011-05-26-14:15:00) > "$work/schedule.txt" 2>&1; then
        echo "ScheduleSample: $(head -n 1 "$work/schedule.txt")"
    else
        fail "ScheduleSample did not run on bin/schedule.properties and bin/sample.ipf.zip"
        sed 's/^/    /' "$work/schedule.txt"
    fi
fi

if [[ "$checked" -eq 0 ]]; then
    echo "FAIL: no ELF files under $root"
    exit 1
fi

echo "failed checks: $failed"
[[ "$failed" -eq 0 ]]
