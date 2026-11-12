# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Checks that the sources mention no deprecated function of the Graal Native SDK.

The deprecated functions are those with a non-empty "deprecated" entry in dxfg-api.json of the SDK (the dxfg_ functions
that have a dxfg_v4_ twin). A name is found as a whole identifier, in the code and in the comments alike. GCC and Clang
fail the build on such a call (-Werror=deprecated-declarations); this check covers MSVC, where C4996 is disabled.

Usage: python3 scripts/check_deprecated_sdk_calls.py <dxfg-api.json> <directory or file>...
Exit code: 0 if nothing is found, 1 if a deprecated function is found, 2 on an error.
"""

import json
import pathlib
import re
import sys

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".inl"}
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


class CheckError(Exception):
    pass


def load_deprecated(api_path):
    """Returns {name: [replacement, ...]} of the deprecated functions listed in dxfg-api.json."""
    try:
        api = json.loads(pathlib.Path(api_path).read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise CheckError(f"cannot read {api_path}: {error}") from error

    functions = api.get("functions") if isinstance(api, dict) else None

    if not isinstance(functions, list):
        raise CheckError(f"{api_path} has no list of functions")

    deprecated = {}

    for function in functions:
        if not isinstance(function, dict):
            continue

        name = function.get("name")
        entry = function.get("deprecated")

        if isinstance(name, str) and entry:
            replacements = entry.get("replacements", []) if isinstance(entry, dict) else []
            deprecated[name] = [r for r in replacements if isinstance(r, str)]

    return deprecated


def source_files(paths):
    for path in paths:
        path = pathlib.Path(path)

        if path.is_file():
            yield path
        elif path.is_dir():
            yield from sorted(p for p in path.rglob("*") if p.is_file() and p.suffix.lower() in SOURCE_SUFFIXES)
        else:
            raise CheckError(f"no such file or directory: {path}")


def find(deprecated, paths):
    """Returns (path, line number, name) of each mention of a deprecated function."""
    found = []

    for path in source_files(paths):
        text = path.read_text(encoding="utf-8", errors="replace")

        for number, line in enumerate(text.splitlines(), 1):
            for match in IDENTIFIER.finditer(line):
                if match.group() in deprecated:
                    found.append((path, number, match.group()))

    return found


def main(argv):
    if len(argv) < 3:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    try:
        deprecated = load_deprecated(argv[1])

        if not deprecated:
            raise CheckError(f"{argv[1]} lists no deprecated function")

        found = find(deprecated, argv[2:])
    except CheckError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    for path, number, name in found:
        replacements = ", ".join(deprecated[name]) or "no replacement listed"
        print(f"{path.as_posix()}:{number}: deprecated SDK function {name} (use {replacements})")

    if found:
        print(f"{len(found)} mention(s) of deprecated SDK functions", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
