#!/usr/bin/env python3
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Prints the `include` list of the `build` job of build.yml as JSON.

Usage: build_matrix.py <full|pr> [matrix file]

`full` is every config x build type x sanitizer combination without the excluded ones; `pr` is the `pullRequest`
subset of it. Every `pullRequest` row must name a combination of the full matrix by all of its keys.
"""

import json
import pathlib
import sys

KEYS = ("name", "buildType", "sanitizers")


def matches(row, combination):
    return all(row[key] == combination[key] for key in KEYS if key in row)


def build(matrix, full):
    combinations = [
        {"name": config["name"], "buildType": build_type, "sanitizers": sanitizers}
        for config in matrix["configs"]
        for build_type in matrix["buildTypes"]
        for sanitizers in matrix["sanitizers"]
    ]
    combinations = [c for c in combinations if not any(matches(e, c) for e in matrix["exclude"])]

    if not full:
        unknown = [row for row in matrix["pullRequest"] if set(row) != set(KEYS)]

        if unknown:
            raise SystemExit(f"pullRequest rows must have exactly the keys {KEYS}: {unknown}")

        missing = [row for row in matrix["pullRequest"] if not any(matches(row, c) for c in combinations)]

        if missing:
            raise SystemExit(f"pullRequest rows not in the full matrix: {missing}")

        combinations = [c for c in combinations if any(matches(row, c) for row in matrix["pullRequest"])]

    configs = {config["name"]: config for config in matrix["configs"]}

    return [
        {"config": configs[c["name"]], "buildType": c["buildType"], "sanitizers": c["sanitizers"]}
        for c in combinations
    ]


def main():
    if len(sys.argv) not in (2, 3) or sys.argv[1] not in ("full", "pr"):
        raise SystemExit(__doc__)

    path = pathlib.Path(sys.argv[2]) if len(sys.argv) == 3 else pathlib.Path(__file__).with_name("build-matrix.json")
    print(json.dumps(build(json.loads(path.read_text(encoding="utf-8")), sys.argv[1] == "full")))


if __name__ == "__main__":
    main()
