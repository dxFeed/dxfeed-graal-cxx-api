# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
"""Prepare validated GitHub release metadata without network access."""
import argparse
import json
from pathlib import Path
import posixpath
import re
import sys
from urllib.parse import urlsplit


VERSION_NUMBER = r"(?:0|[1-9][0-9]*)"
TAG_PATTERN = re.compile(rf"v(?P<base>{VERSION_NUMBER}\.{VERSION_NUMBER}\.{VERSION_NUMBER})(?P<rc>-rc[1-9][0-9]*)?")
HEADING_PATTERN = re.compile(r"^##[ \t]+(.+?)[ \t]*#*[ \t]*$")
FENCE_PATTERN = re.compile(r"^ {0,3}(`{3,}|~{3,})")
LINK_DESTINATION = r"(?:[^\s()]+|\([^\s()]*\))+"
INLINE_LINK_PATTERN = re.compile(rf"(!?\[[^\]\n]*\]\()[ \t]*(<[^>\n]+>|{LINK_DESTINATION})([ \t]+(?:\"[^\"\n]*\"|'[^'\n]*')\s*)?\)")
REFERENCE_LINK_PATTERN = re.compile(r"^( {0,3}\[[^\]\n]+\]:[ \t]*)(<[^>\n]+>|\S+)", re.MULTILINE)


class MetadataError(ValueError):
    """Release inputs are invalid or incomplete."""


def outside_fences(text: str) -> list[tuple[str, bool]]:
    lines: list[tuple[str, bool]] = []
    fence_character = ""
    fence_length = 0
    for line in text.splitlines(keepends=True):
        match = FENCE_PATTERN.match(line)
        if fence_character:
            lines.append((line, False))
            if match and match[1][0] == fence_character and len(match[1]) >= fence_length and not line[match.end():].strip():
                fence_character = ""
        elif match:
            fence_character = match[1][0]
            fence_length = len(match[1])
            lines.append((line, False))
        else:
            lines.append((line, True))
    return lines


def cmake_version(source: Path) -> str:
    text = (source / "CMakeLists.txt").read_text(encoding="utf-8-sig")
    text = re.sub(r"#[^\n]*", "", text)
    projects = re.findall(r"\bproject\s*\(([^)]*)\)", text, flags=re.IGNORECASE)
    versions = [match[1] for project in projects
                if (match := re.search(r"\bVERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\b", project, flags=re.IGNORECASE))]
    if len(versions) != 1:
        raise MetadataError("CMakeLists.txt must contain one project VERSION X.Y.Z")
    return versions[0]


def extract_notes(text: str, tag: str, base: str, prerelease: bool) -> tuple[str, str]:
    sections: list[tuple[str, list[str]]] = []
    for line, outside in outside_fences(text):
        heading = HEADING_PATTERN.fullmatch(line.rstrip("\r\n")) if outside else None
        if heading:
            sections.append((heading[1], []))
        elif sections:
            sections[-1][1].append(line)
    candidates: dict[str, list[tuple[str, list[str]]]] = {}
    for heading, body in sections:
        version = heading.split(maxsplit=1)[0]
        candidates.setdefault(version, []).append((heading, body))
    keys = [tag, f"v{base}"] if prerelease else [tag]
    for key in dict.fromkeys(keys):
        matches = candidates.get(key, [])
        if len(matches) > 1:
            raise MetadataError(f"Duplicate release notes sections for {key}")
    selected = next((candidates[key][0] for key in keys if candidates.get(key)), None)
    if selected is None:
        raise MetadataError(f"Missing release notes section for {tag}")
    heading, body = selected
    if not prerelease and re.search(r"\bUnreleased\b", heading, flags=re.IGNORECASE):
        raise MetadataError("Stable release notes cannot be Unreleased")
    notes = "".join(body).strip()
    if not notes or not any(line.strip() and not line.lstrip().startswith("#") for line in body):
        raise MetadataError(f"Empty release notes section for {tag}")
    return heading, notes


def rewrite_links(notes: str, repository: str, tag: str) -> str:
    prefix = f"https://github.com/{repository}/blob/{tag}/"

    def target_url(target: str) -> str:
        angle = target.startswith("<") and target.endswith(">")
        value = target[1:-1] if angle else target
        parsed = urlsplit(value)
        if parsed.scheme or parsed.netloc or value.startswith("#"):
            return target
        path = posixpath.normpath(parsed.path.lstrip("/"))
        if path == ".." or path.startswith("../"):
            raise MetadataError(f"Release notes link escapes source root: {value}")
        suffix = (f"?{parsed.query}" if parsed.query else "") + (f"#{parsed.fragment}" if parsed.fragment else "")
        rewritten = prefix + path + suffix
        return f"<{rewritten}>" if angle else rewritten

    def inline(match: re.Match[str]) -> str:
        return match[1] + target_url(match[2]) + (match[3] or "") + ")"

    result: list[str] = []
    for line, outside in outside_fences(notes):
        if outside:
            line = INLINE_LINK_PATTERN.sub(inline, line)
            line = REFERENCE_LINK_PATTERN.sub(lambda match: match[1] + target_url(match[2]), line)
        result.append(line)
    return "".join(result)


def prepare(tag: str, repository: str, source: Path, output_dir: Path, github_output: Path | None) -> None:
    match = TAG_PATTERN.fullmatch(tag)
    if match is None:
        raise MetadataError("Tag must be vX.Y.Z or vX.Y.Z-rcN, with no leading zeros and positive N")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository):
        raise MetadataError("Repository must be owner/name")
    base = match["base"]
    if cmake_version(source) != base:
        raise MetadataError(f"Tag base version {base} does not match CMake project VERSION")
    prerelease = bool(match["rc"])
    section, notes = extract_notes((source / "ReleaseNotes.md").read_text(encoding="utf-8-sig"), tag, base, prerelease)
    notes = rewrite_links(notes, repository, tag)
    if prerelease:
        notes = f"> **Release candidate {tag}:** This prerelease is intended for testing.\n\n" + notes
    metadata = {"tag": tag, "version": tag[1:], "prerelease": prerelease, "notes_section": section}
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "release.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    (output_dir / "release-notes.md").write_text(notes + "\n", encoding="utf-8")
    if github_output is not None:
        with github_output.open("a", encoding="utf-8") as output:
            output.write(f"tag={tag}\nversion={tag[1:]}\nprerelease={str(prerelease).lower()}\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser("prepare", help="Prepare release metadata and notes")
    command.add_argument("--tag", required=True)
    command.add_argument("--repository", required=True)
    command.add_argument("--source", required=True, type=Path)
    command.add_argument("--output-dir", required=True, type=Path)
    command.add_argument("--github-output", type=Path)
    args = parser.parse_args()
    try:
        prepare(args.tag, args.repository, args.source, args.output_dir, args.github_output)
    except (MetadataError, OSError, UnicodeError) as error:
        print(f"Release metadata error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
