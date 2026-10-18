#!/usr/bin/env python3
"""Check project C++ names with clang-tidy and a configured compile database."""

import argparse
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, help="Directory containing compile_commands.json")
    parser.add_argument("--clang-tidy", default="clang-tidy", help="clang-tidy executable")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    build = args.build.resolve()
    entries = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
    files = set()
    for entry in entries:
        source = Path(entry["file"])
        if not source.is_absolute():
            source = Path(entry["directory"]) / source
        source = source.resolve()
        if not source.is_relative_to(root):
            continue
        relative = source.relative_to(root)
        if relative.parts[0] in {"src", "tests", "tools", "examples", "experiments", "fuzz"}:
            files.add(source)
    if not files:
        parser.error("The compilation database contains no project C++ sources")
    failed = []
    for source in sorted(files):
        print(f"Checking {source.relative_to(root)}", flush=True)
        result = subprocess.run([
            args.clang_tidy, str(source), "-p", str(build), "--quiet",
            f"--config-file={root / '.clang-tidy'}",
        ], cwd=root, check=False)
        if result.returncode:
            failed.append(source)
    print(f"Checked {len(files)} translation units; {len(failed)} failed.")
    return bool(failed)


if __name__ == "__main__":
    sys.exit(main())
