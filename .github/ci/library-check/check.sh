#!/bin/bash
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Checks the shared library of an unpacked Release package of the C++ API: its version in the name (the SONAME on
# Linux, the install name on macOS) and the hardening of a Release build (DXFCXX_HARDENING).
#   Linux:   the SONAME is libdxFeedGraalCxxApi.so.<major> and lib/libdxFeedGraalCxxApi.so links to it; full RELRO
#            (GNU_RELRO and BIND_NOW), a non-executable stack, the stack protector and the _FORTIFY_SOURCE checks.
#   macOS:   the install name is @rpath/libdxFeedGraalCxxApi.<major>.dylib and lib/libdxFeedGraalCxxApi.dylib links to
#            it.
#   Windows: Control Flow Guard and CET compatibility of bin/dxFeedGraalCxxApi.dll (dumpbin of Visual Studio).
# Usage: check.sh <package directory> <version, for example v9.0.0>
set -u

package=${1:?usage: check.sh <package directory> <version>}
version=${2:?usage: check.sh <package directory> <version>}
major=$(printf '%s\n' "$version" | sed -n 's/^v\{0,1\}\([0-9][0-9]*\)\..*/\1/p')
failures=0

fail() {
    echo "FAIL: $*"
    failures=$((failures + 1))
}

ok() {
    echo "ok: $*"
}

if [ -z "$major" ]; then
    echo "FAIL: no major version in '$version'"
    exit 1
fi

case "$(uname -s)" in
Linux)
    library="$package/lib/libdxFeedGraalCxxApi.so"
    soname="libdxFeedGraalCxxApi.so.$major"
    [ -f "$library" ] || { echo "FAIL: no $library"; exit 1; }

    actual=$(readelf -d "$library" | sed -n 's/.*(SONAME).*\[\(.*\)\]/\1/p')
    [ "$actual" = "$soname" ] && ok "SONAME $actual" || fail "SONAME '$actual', expected $soname"
    [ "$(readlink "$library")" = "$soname" ] && ok "lib/libdxFeedGraalCxxApi.so -> $soname" ||
        fail "lib/libdxFeedGraalCxxApi.so -> '$(readlink "$library")', expected $soname"

    readelf -lW "$library" | grep -q 'GNU_RELRO' && ok "GNU_RELRO" || fail "no GNU_RELRO segment"
    readelf -d "$library" | grep -Eq '\(BIND_NOW\)|\(FLAGS\).*BIND_NOW|\(FLAGS_1\).*NOW' && ok "BIND_NOW" ||
        fail "no BIND_NOW (-z now)"
    stack=$(readelf -lW "$library" | grep 'GNU_STACK')
    if [ -z "$stack" ]; then
        fail "no GNU_STACK segment (the stack is executable)"
    elif printf '%s\n' "$stack" | grep -q 'RWE'; then
        fail "executable stack: $stack"
    else
        ok "non-executable stack"
    fi

    symbols=$(readelf --dyn-syms -W "$library")
    printf '%s\n' "$symbols" | grep -q ' UND __stack_chk_fail' && ok "stack protector" ||
        fail "no __stack_chk_fail import (stack protector)"
    checks=$(printf '%s\n' "$symbols" | grep -Eo ' UND __[a-z0-9_]+_chk\b' | sort -u | wc -l)
    [ "$checks" -gt 0 ] && ok "_FORTIFY_SOURCE: $checks __*_chk imports" ||
        fail "no __*_chk import (_FORTIFY_SOURCE)"
    ;;
Darwin)
    library="$package/lib/libdxFeedGraalCxxApi.dylib"
    name="libdxFeedGraalCxxApi.$major.dylib"
    [ -f "$library" ] || { echo "FAIL: no $library"; exit 1; }

    actual=$(otool -D "$library" | sed -n '2p')
    [ "$actual" = "@rpath/$name" ] && ok "install name $actual" || fail "install name '$actual', expected @rpath/$name"
    [ "$(readlink "$library")" = "$name" ] && ok "lib/libdxFeedGraalCxxApi.dylib -> $name" ||
        fail "lib/libdxFeedGraalCxxApi.dylib -> '$(readlink "$library")', expected $name"
    ;;
MINGW* | MSYS* | CYGWIN*)
    library="$package/bin/dxFeedGraalCxxApi.dll"
    [ -f "$library" ] || { echo "FAIL: no $library"; exit 1; }

    vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    dumpbin=$("$vswhere" -latest -find 'VC/Tools/MSVC/**/bin/Hostx64/x64/dumpbin.exe' | head -n 1 | tr -d '\r')
    [ -n "$dumpbin" ] || { echo "FAIL: no dumpbin.exe"; exit 1; }

    headers=$("$dumpbin" -nologo -headers "$(cygpath -w "$library")")
    printf '%s\n' "$headers" | grep -q 'Control Flow Guard' && ok "Control Flow Guard" ||
        fail "no Control Flow Guard (/guard:cf)"
    printf '%s\n' "$headers" | grep -q 'CET compatible' && ok "CET compatible" || fail "not CET compatible (/CETCOMPAT)"
    ;;
*)
    echo "FAIL: unknown system $(uname -s)"
    exit 1
    ;;
esac

if [ "$failures" -ne 0 ]; then
    echo "failed checks: $failures"
    exit 1
fi
