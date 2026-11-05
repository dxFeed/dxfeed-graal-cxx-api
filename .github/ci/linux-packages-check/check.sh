#!/bin/bash
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Checks that the unpacked Linux packages load on the system it runs on (an older distribution in a container):
# ldd finds every library and symbol version that the libraries and executables of the packages need, and
# WriteTapeFileSample starts the Graal isolate and writes a tape file.
#
# Usage: check.sh <directory with the unpacked packages>
#
# Every package is a directory with lib/ and/or bin/. ldd gets its lib/ as the library path (the RUNPATH of the samples
# in subdirectories of bin/ does not reach lib/); WriteTapeFileSample, in bin/, starts without it, as users start it.

set -u
shopt -s globstar nullglob

root="${1:?usage: check.sh <directory with the unpacked packages>}"
failed=0
checked=0

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
        missing="$(LD_LIBRARY_PATH="$package/lib" ldd "$file" 2>&1 | grep 'not found')"
        checked=$((checked + 1))
        if [[ -n "$missing" ]]; then
            echo "FAIL: ${file#"$root"/}"
            echo "$missing" | sed 's/^/    /'
            failed=$((failed + 1))
        fi
    done
done

echo "ldd: $checked files checked, $failed with missing libraries or symbol versions"

started=0
for sample in "$root"/*/bin/WriteTapeFileSample; do
    [[ -f "$sample" ]] || continue
    started=1
    work="$(mktemp -d)"
    if (cd "$work" && timeout 120 "$sample") &&
        grep -q 'TEST1' "$work/WriteTapeFile.out.txt"; then
        echo "WriteTapeFileSample: wrote $work/WriteTapeFile.out.txt"
    else
        echo "FAIL: ${sample#"$root"/} did not write the tape file"
        failed=$((failed + 1))
    fi
    break
done

if [[ "$started" -eq 0 ]]; then
    echo "FAIL: no WriteTapeFileSample under $root"
    failed=$((failed + 1))
fi

if [[ "$checked" -eq 0 ]]; then
    echo "FAIL: no ELF files under $root"
    exit 1
fi

[[ "$failed" -eq 0 ]]
