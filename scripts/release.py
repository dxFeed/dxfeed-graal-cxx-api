#!/usr/bin/env python3
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Prepares a release of the dxFeed Graal C++ API (see HOW_TO_RELEASE.md). scripts/release.cmake does the same with
CMake only (cmake -P scripts/release.cmake <arguments>).

  release.py vX.Y.Z[-alphaN|-betaN|-preN|-rcN]  sets the version (CMakeLists.txt, docs/Doxyfile, the inline namespace
                                                of Conf.hpp) and the heading of ReleaseNotes.md, commits "vX.Y.Z..."
                                                and tags it
  release.py vX.Y.Z-draftN                      a trial run of release.yml: only the tag on the current commit
  release.py <version> --push                   the same, then pushes the branch and the tag (the tag starts
                                                release.yml)
  release.py <version> --dry-run                prints what would change and changes nothing
  release.py <version> --notes [--ref <ref>]    prints the notes of the version, the text of its GitHub release (a
                                                draft: the unreleased items at the top of ReleaseNotes.md)
  release.py vX.Y.Z-draftN --cleanup            deletes the draft release and its tag (needs the GitHub CLI)
"""

import argparse
import dataclasses
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
NOTES = "ReleaseNotes.md"
CMAKE = "CMakeLists.txt"
DOXYFILE = "docs/Doxyfile"
CONF = "include/dxfeed_graal_cpp_api/internal/Conf.hpp"

PRE_RELEASE_ORDER = ("alpha", "beta", "pre", "rc")
VERSION = re.compile(r"^v(\d+)\.(\d+)\.(\d+)(?:-(alpha|beta|pre|rc|draft)(\d+))?$")
HEADING = re.compile(r"^## (\S+)\s*$")

# The places of the version: (file, name in the messages, pattern with the value as group 2, the value of a version).
PLACES = (
    (CMAKE, "DXFCXX_VERSION", r'(set\(DXFCXX_VERSION ")(v[^"]*)(")', str),
    (DOXYFILE, "PROJECT_NUMBER", r"(?m)^(PROJECT_NUMBER\s*=\s*)(\S+)()", str),
    (CONF, "inline namespace", r"(inline namespace )(v\d+)( \{)", lambda version: f"v{version.major}"),
)


class ReleaseError(Exception):
    pass


@dataclasses.dataclass(frozen=True)
class Version:
    major: int
    minor: int
    patch: int
    kind: str = ""  # "" for a release, a pre-release kind or "draft"
    number: int = 0

    @staticmethod
    def parse(text):
        match = VERSION.match(text)

        if not match:
            raise ReleaseError(f"{text!r} is not a version: vX.Y.Z or vX.Y.Z-<alpha|beta|pre|rc|draft><N>")

        major, minor, patch, kind, number = match.groups()

        return Version(int(major), int(minor), int(patch), kind or "", int(number) if number else 0)

    @property
    def base(self):
        return self.major, self.minor, self.patch

    @property
    def is_release(self):
        return not self.kind

    @property
    def is_pre_release(self):
        return self.kind in PRE_RELEASE_ORDER

    @property
    def is_draft(self):
        return self.kind == "draft"

    def pre_release_key(self):
        return PRE_RELEASE_ORDER.index(self.kind), self.number

    def __str__(self):
        suffix = f"-{self.kind}{self.number}" if self.kind else ""

        return f"v{self.major}.{self.minor}.{self.patch}{suffix}"


def _first_heading(lines):
    """The index and the version of the first "## " heading, or (None, None)."""
    for index, line in enumerate(lines):
        match = HEADING.match(line)

        if match:
            try:
                return index, Version.parse(match.group(1))
            except ReleaseError as error:
                raise ReleaseError(f"{NOTES}:{index + 1}: the heading {line!r}: {error}") from None

    return None, None


def update_notes(text, version):
    """(ReleaseNotes.md with the heading of the version, what was done): see HOW_TO_RELEASE.md; a draft changes
    nothing.

    The unreleased items are at the top, above the first heading. A release or a pre-release puts its heading above
    them; if the first heading is an earlier pre-release of the same version, that heading is removed, so the items
    added since that pre-release and its own items form one section.
    """
    if version.is_draft:
        return text, "unchanged (a draft)"

    lines = text.split("\n")
    index, first = _first_heading(lines)
    unreleased = lines[:index] if index is not None else lines
    replaces = (first is not None and first.base == version.base and first.is_pre_release
                and (version.is_release or first.pre_release_key() < version.pre_release_key()))

    if first is not None and (first.base > version.base or first.base == version.base and not replaces):
        raise ReleaseError(f"{NOTES} already has {first}: {version} cannot follow it")

    if not replaces and not any(line.strip() for line in unreleased):
        raise ReleaseError(f"{NOTES} has no unreleased items at the top" +
                           (f" (the first heading is {first})" if first else ""))

    if replaces:
        end = index + 1

        if end < len(lines) and not lines[end].strip():
            end += 1  # the empty line after the removed heading

        lines = lines[:index] + lines[end:]

    while lines and not lines[0].strip():
        lines = lines[1:]

    action = f"## {first} replaced by ## {version}" if replaces else f"## {version} inserted above the unreleased items"

    return "\n".join([f"## {version}", ""] + lines), action


def notes_of(text, version):
    """The notes of the version for its GitHub release (a draft: the unreleased items at the top)."""
    lines = text.split("\n")

    if version.is_draft:
        index, _ = _first_heading(lines)
        section = lines[:index] if index is not None else lines
    else:
        heading = f"## {version}"
        stripped = [line.rstrip() for line in lines]

        if heading not in stripped:
            raise ReleaseError(f"{NOTES} has no heading {heading!r}")

        start = stripped.index(heading) + 1
        end = next((i for i in range(start, len(lines)) if HEADING.match(lines[i])), len(lines))
        section = lines[start:end]

    return "\n".join(section).strip() + "\n"


def set_versions(files, version):
    """(The texts of the files with the version, the messages). Every place must be found once."""
    result = {}
    messages = []

    for path, name, pattern, value_of in PLACES:
        matches = list(re.finditer(pattern, files[path]))

        if len(matches) != 1:
            raise ReleaseError(f"{path}: expected one place of the version, found {len(matches)}")

        old, new = matches[0].group(2), value_of(version)
        result[path] = re.sub(pattern, lambda m: m.group(1) + new + m.group(3), files[path])
        messages.append(f"{path}: {name} {old} -> {new}" if old != new else f"{path}: {name} {old} (unchanged)")

    return result, messages


def _read(path):
    raw = (ROOT / path).read_bytes().decode("utf-8")

    return raw.replace("\r\n", "\n"), "\r\n" in raw


def _write(path, text, crlf):
    (ROOT / path).write_bytes((text.replace("\n", "\r\n") if crlf else text).encode("utf-8"))


def _git(*args, check=True):
    return subprocess.run(["git", *args], cwd=ROOT, check=check, capture_output=True, text=True).stdout.strip()


def _tag_exists(tag):
    local = _git("tag", "--list", tag)
    remote = _git("ls-remote", "--tags", "origin", f"refs/tags/{tag}", check=False)

    return bool(local or remote)


def _push(*refs):
    for ref in refs:
        print(f"git push origin {ref}")
        _git("push", "origin", ref)


def prepare(version, dry_run, push):
    if _tag_exists(str(version)):
        raise ReleaseError(f"the tag {version} already exists")

    if version.is_draft:
        head = _git("rev-parse", "--short", "HEAD")
        print(f"{version}: a trial run of release.yml, no commit, only the tag on {head}")

        if dry_run:
            print(f"(dry run: no tag {version})")
            return

        _git("tag", str(version))
        print(f"Tagged {version}")

        if push:
            _push(str(version))
        else:
            print(f"Push it: git push origin {version}")

        print(f"Remove it afterwards: release.py {version} --cleanup")
        return

    if not dry_run and _git("status", "--porcelain"):
        raise ReleaseError("the working tree has changes: commit or stash them first")

    old = {path: _read(path) for path in (NOTES, CMAKE, DOXYFILE, CONF)}
    new, messages = set_versions({path: old[path][0] for path in (CMAKE, DOXYFILE, CONF)}, version)
    new[NOTES], action = update_notes(old[NOTES][0], version)

    for message in [f"{NOTES}: {action}"] + messages:
        print(message)

    if dry_run:
        print(f"(dry run: no files changed, no commit {version}, no tag {version})")
        return

    for path in old:
        _write(path, new[path], old[path][1])

    _git("add", *old)
    _git("commit", "-q", "-m", str(version))
    _git("tag", str(version))
    branch = _git("rev-parse", "--abbrev-ref", "HEAD")
    print(f"Committed and tagged {version}")

    if push:
        _push(branch, str(version))
    else:
        print(f"Push them: git push origin {branch} && git push origin {version}")


def cleanup(version):
    if not version.is_draft:
        raise ReleaseError(f"--cleanup removes only drafts, not {version}")

    subprocess.run(["gh", "release", "delete", str(version), "--cleanup-tag", "--yes"], cwd=ROOT, check=False)
    _git("tag", "-d", str(version), check=False)
    print(f"Removed the draft release and the tag {version} (if they existed)")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--push", action="store_true", help="push the branch and the tag after the commit and the tag")
    mode.add_argument("--dry-run", action="store_true", help="print what would change, change nothing")
    mode.add_argument("--notes", action="store_true", help="print the notes of the version")
    mode.add_argument("--cleanup", action="store_true", help="delete the draft release and its tag")
    parser.add_argument("--ref", help="with --notes: read ReleaseNotes.md of this git ref (for example, the tag)")
    try:
        args = parser.parse_args(argv)
    except SystemExit as exit_:  # wrong arguments: argparse printed the usage
        return exit_.code

    try:
        version = Version.parse(args.version)

        if args.notes:
            if args.ref:
                text = _git("show", f"{args.ref}:{NOTES}").replace("\r\n", "\n") + "\n"
            else:
                text = _read(NOTES)[0]

            sys.stdout.write(notes_of(text, version))
        elif args.cleanup:
            cleanup(version)
        else:
            prepare(version, args.dry_run, args.push)
    except (ReleaseError, subprocess.CalledProcessError) as error:
        message = error.stderr.strip() if isinstance(error, subprocess.CalledProcessError) else error
        print(f"release.py: {message}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
