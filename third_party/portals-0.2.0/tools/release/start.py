# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
"""Prepare a release pull request, or dispatch publication after its merge."""
import argparse
from datetime import date
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import metadata


RELEASE_FILES = ("CMakeLists.txt", "ReleaseNotes.md")
BRANCH_PREFIX = "task/release-"
SHA_PATTERN = re.compile(r"[0-9a-f]{40}")


class StartError(ValueError):
    """Release preparation cannot safely continue."""


def run(command: list[str], root: Path, input_text: str | None = None) -> str:
    result = subprocess.run(command, cwd=root, input=input_text.encode("utf-8") if input_text else None,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
                            env={**os.environ, "GIT_OPTIONAL_LOCKS": "0"})
    if result.returncode:
        raise StartError(f'{command[0]} failed: {result.stderr.decode("utf-8", errors="replace").strip()}')
    return result.stdout.decode("utf-8")


def git(root: Path, *arguments: str) -> str:
    return run(["git", *arguments], root).strip()


def repository_name(url: str) -> str:
    match = re.fullmatch(r"(?:https://github\.com/|ssh://git@github\.com/|git@github\.com:)([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+?)(?:\.git)?/?", url)
    if not match:
        raise StartError("origin must identify a GitHub repository over HTTPS or SSH")
    return match[1]


def validate_tag(tag: str) -> re.Match[str]:
    match = metadata.TAG_PATTERN.fullmatch(tag)
    if match is None:
        raise StartError("Tag must be vX.Y.Z or vX.Y.Z-rcN, without leading zeros")
    return match


def version_span(cmake: str) -> tuple[int, int, str]:
    uncommented = re.sub(r"#[^\n]*", lambda match: " " * len(match[0]), cmake)
    versions = []
    for project in re.finditer(r"\bproject\s*\(([^)]*)\)", uncommented, re.IGNORECASE):
        for version in re.finditer(r"\bVERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\b", project[1], re.IGNORECASE):
            versions.append((project.start(1) + version.start(1), project.start(1) + version.end(1), version[1]))
    if len(versions) != 1:
        raise StartError("CMakeLists.txt must contain one numeric project VERSION")
    return versions[0]


def prepare_files(cmake: str, notes: str, tag: str, release_date: str) -> dict[str, str]:
    match = validate_tag(tag)
    date.fromisoformat(release_date)
    sections = []
    offset = 0
    for line, outside in metadata.outside_fences(notes):
        heading = metadata.HEADING_PATTERN.fullmatch(line.rstrip("\r\n")) if outside else None
        if heading:
            sections.append((offset, offset + len(line), heading[1], line))
        offset += len(line)
    unreleased = [section for section in sections if re.search(r"\bUnreleased\b", section[2], re.IGNORECASE)]
    if len(unreleased) != 1 or sections[0] != unreleased[0]:
        raise StartError("Release notes must have one Unreleased section, before all released sections")
    start, end, heading, line = unreleased[0]
    version_start, version_end, current_version = version_span(cmake)
    generic = heading.lower() == "unreleased"
    source_tag = heading.split(maxsplit=1)[0]
    if not generic:
        source_match = validate_tag(source_tag)
        if source_match["rc"]:
            raise StartError("Unreleased must describe a numeric base version, not an RC")
        if current_version != source_match["base"]:
            raise StartError("CMake project VERSION does not match the Unreleased base version")
        metadata.extract_notes(notes, source_tag, source_match["base"], True)
    body_end = sections[1][0] if len(sections) > 1 else len(notes)
    body = notes[end:body_end]
    if generic:
        metadata.extract_notes(f"## v{current_version}\n" + body, f"v{current_version}", current_version, True)
    target_keys = {tag, f'v{match["base"]}'}
    if any(section[2].split(maxsplit=1)[0] in target_keys for section in sections[1:]):
        raise StartError("The requested release already has a notes section")
    newline = "\r\n" if line.endswith("\r\n") else "\n"
    dated_heading = f"## {tag} ({release_date}){newline}"
    if match["rc"]:
        base_heading = line if generic else line.replace(source_tag, f'v{match["base"]}', 1)
        updated_notes = notes[:start] + base_heading + body + dated_heading + body + notes[body_end:]
    else:
        updated_notes = notes[:start] + f"## Unreleased{newline}{newline}" + dated_heading + notes[end:]
    metadata.extract_notes(updated_notes, tag, match["base"], bool(match["rc"]))
    return {"CMakeLists.txt": cmake[:version_start] + match["base"] + cmake[version_end:],
            "ReleaseNotes.md": updated_notes}


def files_at(root: Path, commit: str) -> dict[str, str]:
    return {name: run(["git", "show", f"{commit}:{name}"], root) for name in RELEASE_FILES}


def validate_release(files: dict[str, str], tag: str) -> str:
    match = validate_tag(tag)
    if version_span(files["CMakeLists.txt"])[2] != match["base"]:
        raise StartError("Prepared CMake project VERSION does not match the requested tag")
    heading, _ = metadata.extract_notes(files["ReleaseNotes.md"], tag, match["base"], bool(match["rc"]))
    dated = re.fullmatch(re.escape(tag) + r" \((\d{4}-\d{2}-\d{2})\)", heading)
    if dated is None:
        raise StartError("Prepared notes must have an exact dated section for the requested tag")
    date.fromisoformat(dated[1])
    return dated[1]


def validate_branch(root: Path, commit: str, main_commit: str, tag: str) -> None:
    parents = git(root, "rev-list", "--parents", "-n", "1", commit).split()
    if len(parents) != 2:
        raise StartError("Preparation branch must contain a single release preparation commit")
    parent = parents[1]
    git(root, "merge-base", "--is-ancestor", parent, main_commit)
    changed = set(git(root, "diff", "--name-only", parent, commit).splitlines())
    if not changed or not changed.issubset(RELEASE_FILES):
        raise StartError("Preparation commit may change only CMakeLists.txt and ReleaseNotes.md")
    actual = files_at(root, commit)
    release_date = validate_release(actual, tag)
    original = files_at(root, parent)
    expected = prepare_files(original["CMakeLists.txt"], original["ReleaseNotes.md"], tag, release_date)
    if expected != actual:
        raise StartError("Existing preparation branch differs from the requested version and unchanged notes body")


def remote_refs(root: Path, branch: str, tag: str) -> dict[str, str]:
    output = git(root, "ls-remote", "origin", "refs/heads/main", f"refs/heads/{branch}",
                 f"refs/tags/{tag}", f"refs/tags/{tag}^{{}}")
    return {ref: sha for sha, ref in (line.split() for line in output.splitlines())}


def local_branch(root: Path, branch: str) -> str | None:
    result = git(root, "for-each-ref", "--format=%(objectname)", f"refs/heads/{branch}")
    return result or None


def pull_requests(root: Path, repository: str, branch: str) -> list[dict]:
    return json.loads(run(["gh", "pr", "list", "--repo", repository, "--head", branch, "--base", "main",
                           "--state", "all", "--json", "url,state,headRefOid,mergeCommit,isCrossRepository"], root))


def start(root: Path, tag: str, dry_run: bool = False) -> str:
    validate_tag(tag)
    root = Path(git(root, "rev-parse", "--show-toplevel"))
    if git(root, "status", "--porcelain", "--untracked-files=all"):
        raise StartError("Working tree must be clean before release preparation")
    repository = repository_name(git(root, "remote", "get-url", "origin"))
    push_urls = git(root, "remote", "get-url", "--push", "--all", "origin").splitlines()
    if len(push_urls) != 1 or repository_name(push_urls[0]).lower() != repository.lower():
        raise StartError("origin must have exactly one push URL for the same GitHub repository")
    branch = BRANCH_PREFIX + tag
    refs = remote_refs(root, branch, tag)
    main_commit = refs.get("refs/heads/main")
    if main_commit is None:
        raise StartError("origin has no main branch")
    tag_commit = refs.get(f"refs/tags/{tag}^{{}}", refs.get(f"refs/tags/{tag}"))
    remote_branch = refs.get(f"refs/heads/{branch}")
    existing_local = local_branch(root, branch)
    if remote_branch and existing_local and remote_branch != existing_local:
        raise StartError("Local and remote preparation branches differ; refusing to overwrite either")
    if dry_run:
        if remote_branch or existing_local or tag_commit:
            raise StartError("Release refs already exist; use the normal command to validate and resume")
        source = files_at(root, main_commit)
        prepared = prepare_files(source["CMakeLists.txt"], source["ReleaseNotes.md"], tag, date.today().isoformat())
        return json.dumps({"tag": tag, "branch": branch, "base": "main", "main_commit": main_commit,
                           "changed_files": [name for name in RELEASE_FILES if source[name] != prepared[name]],
                           "dry_run": True}, indent=2)
    run(["gh", "auth", "status", "--hostname", "github.com"], root)
    git(root, "fetch", "--no-tags", "origin", "refs/heads/main")
    if git(root, "rev-parse", "FETCH_HEAD") != main_commit:
        raise StartError("origin/main changed during preparation; retry the command")
    prs = pull_requests(root, repository, branch)
    if len(prs) > 1:
        raise StartError("Multiple release preparation PRs exist; inspect them before retrying")
    pr = prs[0] if prs else None
    if pr and pr.get("isCrossRepository") is not False:
        raise StartError("Preparation PR must originate from the same repository")
    if pr and pr["state"] == "CLOSED":
        raise StartError("Preparation PR was closed without merging; inspect it before retrying")
    if pr and pr["state"] == "MERGED":
        merged = pr["mergeCommit"]["oid"]
        if tag_commit and tag_commit != merged:
            raise StartError("Existing release tag does not point to the PR merge commit")
        git(root, "fetch", "--no-tags", "origin", merged)
        git(root, "merge-base", "--is-ancestor", merged, main_commit)
        validate_release(files_at(root, merged), tag)
        return pr["url"]
    if tag_commit:
        raise StartError("Release tag already exists without a matching merged preparation PR")
    if remote_branch:
        git(root, "fetch", "--no-tags", "origin", f"refs/heads/{branch}")
        if git(root, "rev-parse", "FETCH_HEAD") != remote_branch:
            raise StartError("Remote preparation branch changed; retry the command")
    commit = remote_branch or existing_local
    if commit:
        validate_branch(root, commit, main_commit, tag)
    if pr:
        if pr["headRefOid"] != commit:
            raise StartError("Preparation PR head does not match the validated branch")
        return pr["url"]
    prepared = None
    if not commit:
        source = files_at(root, main_commit)
        prepared = prepare_files(source["CMakeLists.txt"], source["ReleaseNotes.md"], tag, date.today().isoformat())
    temporary = Path(tempfile.mkdtemp(prefix="portals-release-"))
    worktree = temporary / "worktree"
    try:
        if existing_local:
            git(root, "worktree", "add", str(worktree), branch)
        else:
            git(root, "worktree", "add", "-b", branch, str(worktree), commit or main_commit)
        if not commit:
            for name, content in prepared.items():
                (worktree / name).write_bytes(content.encode("utf-8"))
            git(worktree, "add", "--", *RELEASE_FILES)
            git(worktree, "commit", "--only", "-m", f"Prepare release {tag}", "--", *RELEASE_FILES)
            commit = git(worktree, "rev-parse", "HEAD")
            validate_branch(root, commit, main_commit, tag)
        git(worktree, "push", "origin", f"HEAD:refs/heads/{branch}")
        body = (f"Prepare {tag}: update the numeric CMake project version and date the release notes. "
                "The release notes body is preserved.\n\n"
                "Review and merge this PR into main to create the release tag and start the verified release publication workflow. "
                "Merging this PR authorizes tag creation and release publication; this command does not merge it.\n")
        body_path = temporary / "pr-body.md"
        body_path.write_text(body, encoding="utf-8")
        return run(["gh", "pr", "create", "--repo", repository, "--head", branch, "--base", "main",
                    "--title", f"Prepare release {tag}", "--body-file", str(body_path)], root).strip()
    finally:
        if worktree.exists():
            try:
                if git(worktree, "status", "--porcelain", "--untracked-files=all") or git(worktree, "ls-files", "--others", "--ignored", "--exclude-standard"):
                    print(f"Recovery worktree preserved: {worktree}", file=sys.stderr)
                else:
                    git(root, "worktree", "remove", str(worktree))
            except StartError:
                print(f"Recovery worktree preserved: {worktree}", file=sys.stderr)
        body_path = temporary / "pr-body.md"
        if body_path.exists():
            body_path.unlink()
        if not any(temporary.iterdir()):
            temporary.rmdir()


def dispatch_inputs(event: dict, repository: str, checked_commit: str) -> tuple[str, str]:
    pr = event.get("pull_request", {})
    base = pr.get("base", {})
    head = pr.get("head", {})
    commit = pr.get("merge_commit_sha", "")
    if (event.get("action") != "closed" or pr.get("merged") is not True or base.get("ref") != "main"
            or base.get("repo", {}).get("full_name") != repository
            or head.get("repo", {}).get("full_name") != repository):
        raise StartError("Only merged same-repository release preparation PRs into main may publish")
    branch = head.get("ref", "")
    if not branch.startswith(BRANCH_PREFIX):
        raise StartError("Unexpected release preparation branch")
    tag = branch[len(BRANCH_PREFIX):]
    validate_tag(tag)
    if not SHA_PATTERN.fullmatch(commit) or commit != checked_commit:
        raise StartError("Checkout must match the trusted PR merge commit")
    return tag, commit


def api(root: Path, endpoint: str, payload: dict | None = None) -> dict | list:
    command = ["gh", "api", endpoint]
    if payload is not None:
        command += ["--method", "POST", "--input", "-"]
    return json.loads(run(command, root, json.dumps(payload) if payload is not None else None))


def existing_tag_commit(root: Path, repository: str, tag: str) -> str | None:
    refs = api(root, f"repos/{repository}/git/matching-refs/tags/{tag}")
    exact = [ref for ref in refs if ref["ref"] == f"refs/tags/{tag}"]
    if not exact:
        return None
    target = exact[0]["object"]
    while target["type"] == "tag":
        target = api(root, f'repos/{repository}/git/tags/{target["sha"]}')["object"]
    if target["type"] != "commit":
        raise StartError("Existing tag does not point to a commit")
    return target["sha"]


def dispatch_merged(root: Path, event: dict, repository: str) -> None:
    repository_name(f"https://github.com/{repository}")
    tag, commit = dispatch_inputs(event, repository, git(root, "rev-parse", "HEAD"))
    git(root, "merge-base", "--is-ancestor", commit, "refs/remotes/origin/main")
    validate_release(files_at(root, commit), tag)
    with tempfile.TemporaryDirectory(prefix="portals-release-metadata-") as output:
        metadata.prepare(tag, repository, root, Path(output), None)
    existing = existing_tag_commit(root, repository, tag)
    if existing is not None and existing != commit:
        raise StartError("Existing release tag points to another commit; refusing to overwrite it")
    if existing is None:
        annotation = api(root, f"repos/{repository}/git/tags", {
            "tag": tag, "message": f"Release {tag}", "object": commit, "type": "commit"})
        api(root, f"repos/{repository}/git/refs", {"ref": f"refs/tags/{tag}", "sha": annotation["sha"]})
    run(["gh", "workflow", "run", "release.yml", "--repo", repository, "--ref", "main",
         "-f", f"tag={tag}", "-F", "publish=true"], root)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", nargs="?")
    parser.add_argument("--dry-run", action="store_true", help="Validate and preview without writing repository or remote state")
    parser.add_argument("--dispatch-merged", type=Path, metavar="EVENT_JSON", help="Actions-only: validate a merged PR event and start publication")
    args = parser.parse_args()
    try:
        if args.dispatch_merged:
            if args.tag or args.dry_run:
                raise StartError("Dispatch does not accept a tag or --dry-run")
            dispatch_merged(Path.cwd(), json.loads(args.dispatch_merged.read_text(encoding="utf-8")), os.environ["GITHUB_REPOSITORY"])
        else:
            if not args.tag:
                raise StartError("A release tag is required")
            print(start(Path.cwd(), args.tag, args.dry_run))
    except (StartError, metadata.MetadataError, OSError, ValueError, KeyError) as error:
        print(f"Release start error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
