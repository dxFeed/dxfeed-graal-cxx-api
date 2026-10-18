# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
"""Offline release-start contracts, using disposable Git repositories and fake gh."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[1] / "tools/release/start.py"
sys.path.insert(0, str(SCRIPT.parent))
SPEC = importlib.util.spec_from_file_location("release_start", SCRIPT)
START = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(START)
CM = "# VERSION 8.8.8\ncmake_minimum_required(VERSION 3.24)\nproject(Portals VERSION 0.2.0 LANGUAGES CXX)\nset(EXAMPLE_VERSION 0.2.0)\n"
BODY = "\n- Literal v0.2.0 and [link](docs/index.md).\n### Details\n```markdown\n## v9.9.9 (Unreleased)\n```\n\n"
NOTES = "# Release notes\n\n## v0.2.0 (Unreleased)\n" + BODY + "## v0.1.0 (2026-01-01)\n\nOld body.\n"
REPOSITORY = "owner/portals"
SHELL_NAMES = ("powershell.exe", "pwsh") if os.name == "nt" else ("pwsh",)
POWERSHELLS = [shell for name in SHELL_NAMES if (shell := shutil.which(name))
              and (os.name == "nt" or not shell.lower().endswith(".exe"))]


def shell_command(shell: str, runner: Path) -> list[str]:
    options = ["-NoProfile", "-NonInteractive"]
    if os.name == "nt":
        options += ["-ExecutionPolicy", "Bypass"]
    return [shell, *options, "-File", str(runner)]


class TransformTests(unittest.TestCase):
    def test_stable_changes_only_numeric_version_and_heading(self) -> None:
        result = START.prepare_files(CM, NOTES, "v0.3.0", "2026-10-10")
        self.assertEqual(result["CMakeLists.txt"], CM.replace("project(Portals VERSION 0.2.0", "project(Portals VERSION 0.3.0"))
        self.assertEqual(result["ReleaseNotes.md"], NOTES.replace("## v0.2.0 (Unreleased)", "## Unreleased\n\n## v0.3.0 (2026-10-10)", 1))

    def test_second_stable_release_needs_only_manual_body_entry(self) -> None:
        first = START.prepare_files(CM, NOTES, "v0.2.0", "2026-10-09")
        next_body = "\n- Added the next feature manually.\n\n"
        working_notes = first["ReleaseNotes.md"].replace("## Unreleased\n\n", "## Unreleased\n" + next_body, 1)
        second = START.prepare_files(first["CMakeLists.txt"], working_notes, "v0.3.0", "2026-10-10")
        self.assertEqual(second["ReleaseNotes.md"], first["ReleaseNotes.md"].replace(
            "## Unreleased\n\n", "## Unreleased\n\n## v0.3.0 (2026-10-10)\n" + next_body, 1))
        self.assertEqual(START.version_span(second["CMakeLists.txt"])[2], "0.3.0")
        self.assertIn("## v0.2.0 (2026-10-09)\n" + BODY, second["ReleaseNotes.md"])

    def test_generic_unreleased_rc_preserves_working_heading_and_body(self) -> None:
        generic = NOTES.replace("## v0.2.0 (Unreleased)", "## Unreleased", 1)
        result = START.prepare_files(CM, generic, "v0.3.0-rc1", "2026-10-10")
        self.assertEqual(result["ReleaseNotes.md"], generic.replace("## Unreleased\n" + BODY,
            "## Unreleased\n" + BODY + "## v0.3.0-rc1 (2026-10-10)\n" + BODY, 1))

    def test_empty_generic_unreleased_is_rejected(self) -> None:
        first = START.prepare_files(CM, NOTES, "v0.2.0", "2026-10-09")
        with self.assertRaises(ValueError):
            START.prepare_files(first["CMakeLists.txt"], first["ReleaseNotes.md"], "v0.3.0", "2026-10-10")

    def test_rc_copies_body_and_preserves_unreleased(self) -> None:
        result = START.prepare_files(CM, NOTES, "v0.3.0-rc1", "2026-10-10")
        expected = NOTES.replace("## v0.2.0 (Unreleased)\n" + BODY,
                                 "## v0.3.0 (Unreleased)\n" + BODY + "## v0.3.0-rc1 (2026-10-10)\n" + BODY, 1)
        self.assertEqual(result["ReleaseNotes.md"], expected)
        self.assertEqual(START.validate_release(result, "v0.3.0-rc1"), "2026-10-10")

    def test_rc_after_previous_rc_uses_unreleased_body(self) -> None:
        notes = NOTES.replace("## v0.1.0", "## v0.2.0-rc1 (2026-10-09)\n\nPrior RC.\n\n## v0.1.0")
        result = START.prepare_files(CM, notes, "v0.2.0-rc2", "2026-10-10")
        self.assertIn("## v0.2.0-rc2 (2026-10-10)\n" + BODY, result["ReleaseNotes.md"])
        self.assertIn("Prior RC.", result["ReleaseNotes.md"])

    def test_rc_chain_then_stable(self) -> None:
        first = START.prepare_files(CM, NOTES, "v0.3.0-rc1", "2026-10-08")
        second = START.prepare_files(first["CMakeLists.txt"], first["ReleaseNotes.md"], "v0.3.0-rc2", "2026-10-09")
        stable = START.prepare_files(second["CMakeLists.txt"], second["ReleaseNotes.md"], "v0.3.0", "2026-10-10")
        self.assertIn("## v0.3.0 (2026-10-10)\n" + BODY, stable["ReleaseNotes.md"])
        self.assertIn("## v0.3.0-rc1 (2026-10-08)", stable["ReleaseNotes.md"])
        self.assertIn("## v0.3.0-rc2 (2026-10-09)", stable["ReleaseNotes.md"])
        self.assertNotIn("## v0.3.0 (Unreleased)", stable["ReleaseNotes.md"])

    def test_crlf_body_is_preserved(self) -> None:
        result = START.prepare_files(CM.replace("\n", "\r\n"), NOTES.replace("\n", "\r\n"), "v0.2.0", "2026-10-10")
        self.assertEqual(result["ReleaseNotes.md"], NOTES.replace("## v0.2.0 (Unreleased)", "## Unreleased\n\n## v0.2.0 (2026-10-10)", 1).replace("\n", "\r\n"))

    def test_rejects_invalid_missing_duplicate_empty_or_historical_notes(self) -> None:
        for notes in ["", "## v0.2.0 (2026-10-10)\nBody.\n", "## v0.2.0 (Unreleased)\n\n",
                      NOTES + "## v0.3.0 (Unreleased)\nSecond.\n", NOTES + "## v0.2.0\nDuplicate.\n",
                      "## v0.1.0\nHistory.\n" + NOTES, "```\n## v0.2.0 (Unreleased)\nBody.\n```\n",
                      NOTES + "## v0.3.0 (2026-10-01)\nAlready released.\n"]:
            with self.subTest(notes=notes), self.assertRaises(ValueError):
                START.prepare_files(CM, notes, "v0.3.0", "2026-10-10")

    def test_rejects_source_version_mismatch_and_ambiguous_cmake(self) -> None:
        for cmake in [CM.replace("project(Portals VERSION 0.2.0", "project(Portals VERSION 0.1.0"), CM + "project(Other VERSION 1.0.0)\n"]:
            with self.subTest(cmake=cmake), self.assertRaises(START.StartError):
                START.prepare_files(cmake, NOTES, "v0.3.0", "2026-10-10")

    def test_strict_tags_and_github_origin(self) -> None:
        for tag in ["0.2.0", "v00.2.0", "v0.2.0-rc0", "v0.2.0-rc01", "v0.2.0-beta1", "v0.2.0\n"]:
            with self.subTest(tag=tag), self.assertRaises(START.StartError):
                START.validate_tag(tag)
        for url in ["https://github.com/owner/portals.git", "git@github.com:owner/portals.git", "ssh://git@github.com/owner/portals"]:
            self.assertEqual(START.repository_name(url), REPOSITORY)
        for url in ["https://evil.invalid/owner/portals", "https://github.com/owner/portals/extra", "https://github.com.evil.invalid/owner/portals"]:
            with self.subTest(url=url), self.assertRaises(START.StartError):
                START.repository_name(url)

    def test_prepared_rc_requires_exact_dated_heading(self) -> None:
        for notes in [NOTES, NOTES.replace("(Unreleased)", "(2026-10-10)", 1)]:
            with self.assertRaises(START.StartError):
                START.validate_release({"CMakeLists.txt": CM, "ReleaseNotes.md": notes}, "v0.2.0-rc1")


class GitFixtureTests(unittest.TestCase):
    def setUp(self) -> None:
        fixture_parent = SCRIPT.parents[2] / "build/release-start-tests"
        fixture_parent.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(prefix="fixture-", dir=fixture_parent)
        self.addCleanup(temporary.cleanup)
        self.container = Path(temporary.name)
        self.root = self.container / "source"
        self.remote = self.container / "remote.git"
        self.root.mkdir()
        self.git("init", "-b", "main")
        self.git("config", "user.name", "Fixture User")
        self.git("config", "user.email", "fixture@example.invalid")
        self.git("config", "core.autocrlf", "false")
        (self.root / "CMakeLists.txt").write_text(CM, encoding="utf-8")
        (self.root / "ReleaseNotes.md").write_text(NOTES, encoding="utf-8")
        self.git("add", "CMakeLists.txt", "ReleaseNotes.md")
        self.git("commit", "-m", "Fixture base")
        subprocess.run(["git", "init", "--bare", str(self.remote)], check=True, capture_output=True)
        self.git("remote", "add", "origin", str(self.remote))
        self.git("push", "-u", "origin", "main")
        self.commands = []
        self.prs = []
        self.fail_create = False
        self.original_run = START.run
        self.addCleanup(patch.stopall)
        patch.object(START.tempfile, "tempdir", str(self.container)).start()
        patch.object(START, "repository_name", return_value=REPOSITORY).start()
        patch.object(START, "run", side_effect=self.fake_gh).start()

    def git(self, *arguments: str) -> str:
        result = subprocess.run(["git", *arguments], cwd=self.root, capture_output=True, check=False)
        if result.returncode:
            raise RuntimeError(result.stderr.decode("utf-8", errors="replace"))
        return result.stdout.decode("utf-8").strip()

    def fake_gh(self, command: list[str], root: Path, input_text: str | None = None) -> str:
        self.commands.append(command)
        if command[0] != "gh":
            return self.original_run(command, root, input_text)
        if command[1:3] == ["auth", "status"]:
            return ""
        if command[1:3] == ["pr", "list"]:
            branch = command[command.index("--head") + 1]
            return json.dumps([pr for pr in self.prs if pr.get("headRefName", branch) == branch])
        if command[1:3] == ["pr", "create"]:
            if self.fail_create:
                raise START.StartError("fixture PR failure")
            body = Path(command[command.index("--body-file") + 1]).read_text(encoding="utf-8")
            self.assertIn("Merging this PR authorizes tag creation and release publication", body)
            self.prs = [{"url": "https://github.com/owner/portals/pull/1", "state": "OPEN",
                         "headRefOid": self.git("rev-parse", command[command.index("--head") + 1]),
                         "headRefName": command[command.index("--head") + 1], "mergeCommit": None, "isCrossRepository": False}]
            return self.prs[0]["url"] + "\n"
        if command[1:3] == ["workflow", "run"]:
            return ""
        self.fail(f"Unexpected external command: {command}")

    def test_dirty_rejected_before_external_commands(self) -> None:
        (self.root / "untracked.txt").write_text("User data", encoding="utf-8")
        with self.assertRaisesRegex(START.StartError, "clean"):
            START.start(self.root, "v0.3.0")
        self.assertFalse(any(command[0] == "gh" or "ls-remote" in command for command in self.commands))

    def test_different_push_repository_rejected_before_any_write(self) -> None:
        self.git("remote", "set-url", "--push", "origin", "https://github.com/another/project.git")
        with patch.object(START, "repository_name", side_effect=[REPOSITORY, "another/project"]), self.assertRaisesRegex(START.StartError, "push URL"):
            START.start(self.root, "v0.3.0")
        self.assertFalse(any(command[0] == "gh" or "fetch" in command or "ls-remote" in command for command in self.commands))

    def test_multiple_push_urls_rejected(self) -> None:
        self.git("remote", "set-url", "--push", "--add", "origin", str(self.remote))
        self.git("remote", "set-url", "--push", "--add", "origin", str(self.remote))
        with self.assertRaisesRegex(START.StartError, "push URL"):
            START.start(self.root, "v0.3.0")

    def test_dry_run_does_not_change_git_or_source(self) -> None:
        before = {str(path.relative_to(self.root)): path.read_bytes() for path in self.root.rglob("*") if path.is_file()}
        remote_before = {str(path.relative_to(self.remote)): path.read_bytes() for path in self.remote.rglob("*") if path.is_file()}
        preview = json.loads(START.start(self.root, "v0.3.0", True))
        self.assertEqual(preview["changed_files"], list(START.RELEASE_FILES))
        self.assertEqual(before, {str(path.relative_to(self.root)): path.read_bytes() for path in self.root.rglob("*") if path.is_file()})
        self.assertEqual(remote_before, {str(path.relative_to(self.remote)): path.read_bytes() for path in self.remote.rglob("*") if path.is_file()})
        self.assertFalse(any(command[0] == "gh" for command in self.commands))

    def test_real_git_prepare_push_and_resume_pr(self) -> None:
        self.assertEqual(START.start(self.root, "v0.3.0"), "https://github.com/owner/portals/pull/1")
        self.assertEqual(self.git("branch", "--show-current"), "main")
        self.assertEqual(self.git("status", "--porcelain"), "")
        commit = self.git("rev-parse", "task/release-v0.3.0")
        changed = self.git("diff-tree", "--no-commit-id", "--name-only", "-r", commit).splitlines()
        self.assertEqual(set(changed), set(START.RELEASE_FILES))
        self.assertNotIn("Co-authored-by", self.git("log", "-1", "--format=%B", commit))
        self.assertEqual(len(self.git("worktree", "list", "--porcelain").split("worktree ")) - 1, 1)
        creates = sum(command[0:3] == ["gh", "pr", "create"] for command in self.commands)
        self.assertEqual(START.start(self.root, "v0.3.0"), self.prs[0]["url"])
        self.assertEqual(creates, sum(command[0:3] == ["gh", "pr", "create"] for command in self.commands))

    def test_real_git_second_stable_release_after_only_manual_body_entry(self) -> None:
        START.start(self.root, "v0.2.0")
        self.git("merge", "--no-ff", "task/release-v0.2.0", "-m", "Merge first stable release")
        first_raw = (self.root / "ReleaseNotes.md").read_bytes().decode("utf-8")
        first_notes = (self.root / "ReleaseNotes.md").read_text(encoding="utf-8")
        entry = "\n- Added a new feature by hand.\n\n"
        (self.root / "ReleaseNotes.md").write_text(first_notes.replace("## Unreleased\n\n", "## Unreleased\n" + entry, 1), encoding="utf-8")
        self.git("add", "ReleaseNotes.md")
        self.git("commit", "-m", "Document the next feature")
        self.git("push", "origin", "main")
        START.start(self.root, "v0.3.0")
        prepared = START.files_at(self.root, self.git("rev-parse", "task/release-v0.3.0"))
        self.assertEqual(START.version_span(prepared["CMakeLists.txt"])[2], "0.3.0")
        self.assertIn("## v0.3.0 (", prepared["ReleaseNotes.md"])
        self.assertIn("- Added a new feature by hand.", prepared["ReleaseNotes.md"])
        historical = first_raw[first_raw.index("## v0.2.0 ("):]
        self.assertTrue(prepared["ReleaseNotes.md"].endswith(historical))

    def test_pr_failure_preserves_branch_for_safe_retry(self) -> None:
        self.fail_create = True
        with self.assertRaisesRegex(START.StartError, "PR failure"):
            START.start(self.root, "v0.3.0")
        commit = self.git("rev-parse", "task/release-v0.3.0")
        self.fail_create = False
        START.start(self.root, "v0.3.0")
        self.assertEqual(self.git("rev-parse", "task/release-v0.3.0"), commit)

    def test_remote_resume_without_local_branch(self) -> None:
        START.start(self.root, "v0.3.0")
        commit = self.git("rev-parse", "task/release-v0.3.0")
        self.git("branch", "-D", "task/release-v0.3.0")
        self.assertEqual(START.start(self.root, "v0.3.0"), self.prs[0]["url"])
        self.assertEqual(self.prs[0]["headRefOid"], commit)

    def test_resume_refuses_different_local_hash(self) -> None:
        START.start(self.root, "v0.3.0")
        self.git("update-ref", "refs/heads/task/release-v0.3.0", self.git("rev-parse", "main"))
        with self.assertRaisesRegex(START.StartError, "branches differ"):
            START.start(self.root, "v0.3.0")

    def test_matching_branch_name_in_fork_pr_is_rejected(self) -> None:
        self.prs = [{"url": "https://github.com/owner/portals/pull/2", "state": "OPEN", "isCrossRepository": True}]
        with self.assertRaisesRegex(START.StartError, "same repository"):
            START.start(self.root, "v0.3.0")

    def test_resume_rejects_changed_prose_even_if_metadata_valid(self) -> None:
        prepared = START.prepare_files(CM, NOTES, "v0.3.0", "2026-10-10")
        self.git("checkout", "-b", "task/release-v0.3.0")
        for name, content in prepared.items():
            (self.root / name).write_text(content.replace("Literal", "Rewritten"), encoding="utf-8")
        self.git("add", *START.RELEASE_FILES)
        self.git("commit", "-m", "Wrong body")
        self.git("checkout", "main")
        with self.assertRaisesRegex(START.StartError, "unchanged notes body"):
            START.start(self.root, "v0.3.0")

    def event(self, commit: str) -> dict:
        return {"action": "closed", "pull_request": {"merged": True, "merge_commit_sha": commit,
                "base": {"ref": "main", "repo": {"full_name": REPOSITORY}},
                "head": {"ref": "task/release-v0.3.0", "repo": {"full_name": REPOSITORY}}}}

    def merged_source(self) -> tuple[str, dict]:
        START.start(self.root, "v0.3.0")
        self.git("merge", "--no-ff", "task/release-v0.3.0", "-m", "Merge release")
        self.git("push", "origin", "main")
        commit = self.git("rev-parse", "HEAD")
        return commit, self.event(commit)

    def test_dispatch_creates_annotated_tag_and_typed_publication_input(self) -> None:
        commit, event = self.merged_source()
        with patch.object(START, "existing_tag_commit", return_value=None), patch.object(START, "api", return_value={"sha": "b" * 40}) as api:
            START.dispatch_merged(self.root, event, REPOSITORY)
        self.assertEqual(api.call_args_list[0].args[2], {"tag": "v0.3.0", "message": "Release v0.3.0", "object": commit, "type": "commit"})
        self.assertEqual(api.call_args_list[1].args[2], {"ref": "refs/tags/v0.3.0", "sha": "b" * 40})
        self.assertEqual(self.commands[-1], ["gh", "workflow", "run", "release.yml", "--repo", REPOSITORY,
                                          "--ref", "main", "-f", "tag=v0.3.0", "-F", "publish=true"])

    def test_dispatch_repeat_uses_matching_tag_and_rejects_mismatch(self) -> None:
        commit, event = self.merged_source()
        with patch.object(START, "existing_tag_commit", return_value=commit), patch.object(START, "api") as api:
            START.dispatch_merged(self.root, event, REPOSITORY)
            api.assert_not_called()
        count = len(self.commands)
        with patch.object(START, "existing_tag_commit", return_value="a" * 40), self.assertRaisesRegex(START.StartError, "another commit"):
            START.dispatch_merged(self.root, event, REPOSITORY)
        self.assertFalse(any(command[0:3] == ["gh", "workflow", "run"] for command in self.commands[count:]))

    def test_dispatch_requires_main_ancestry(self) -> None:
        START.start(self.root, "v0.3.0")
        self.git("checkout", "task/release-v0.3.0")
        commit = self.git("rev-parse", "HEAD")
        with patch.object(START, "api") as api, self.assertRaises(START.StartError):
            START.dispatch_merged(self.root, self.event(commit), REPOSITORY)
        api.assert_not_called()

    def test_merged_pr_resume_validates_existing_tag(self) -> None:
        commit, _ = self.merged_source()
        self.prs[0]["state"] = "MERGED"
        self.prs[0]["mergeCommit"] = {"oid": commit}
        self.git("tag", "v0.3.0", "HEAD~1")
        self.git("push", "origin", "refs/tags/v0.3.0")
        with self.assertRaisesRegex(START.StartError, "merge commit"):
            START.start(self.root, "v0.3.0")

    @unittest.skipUnless(POWERSHELLS, "PowerShell is unavailable")
    def test_powershell_real_git_prepare_and_retry(self) -> None:
        runner = self.container / "powershell-fixture.ps1"
        prs = self.container / "prs.json"
        prs.write_text("[]", encoding="utf-8")
        runner.write_text(r'''
. $env:RELEASE_PS_SCRIPT
$script:ActualProcess = ${function:Invoke-ReleaseProcess}
function Get-ReleaseRepository { param([string]$Url) return 'owner/portals' }
function Invoke-ReleaseProcess {
    param([string[]]$Command, [string]$Root)
    if ($Command[0] -ne 'gh') { return (& $script:ActualProcess -Command $Command -Root $Root) }
    if ($Command[1] -eq 'auth') { return '' }
    if ($Command[1] -eq 'pr' -and $Command[2] -eq 'list') {
        $json = [IO.File]::ReadAllText($env:RELEASE_PRS)
        $parsed = ConvertFrom-Json $json
        if (@($parsed).Count -eq 1 -and $parsed.headRefName -and $parsed.headRefName -ne $Command[[array]::IndexOf($Command, '--head') + 1]) { return '[]' }
        return $json
    }
    if ($Command[1] -eq 'pr' -and $Command[2] -eq 'create') {
        $branch = $Command[[array]::IndexOf($Command, '--head') + 1]
        $oid = (Invoke-ReleaseGit $Root @('rev-parse', $branch))
        $pr = @{ url = 'https://github.com/owner/portals/pull/1'; state = 'OPEN'; headRefOid = $oid; headRefName = $branch; mergeCommit = $null; isCrossRepository = $false }
        [IO.File]::WriteAllText($env:RELEASE_PRS, ('[' + ($pr | ConvertTo-Json -Compress) + ']'))
        return $pr.url
    }
    throw 'Unexpected fixture gh command'
}
Start-Release -Root $env:RELEASE_SOURCE -ReleaseTag $env:RELEASE_TAG -Preview:($env:RELEASE_PREVIEW -eq 'true')
''', encoding="utf-8")
        initial_root = self.root
        for index, shell in enumerate(POWERSHELLS):
            remote = self.container / f"powershell-remote-{index}.git"
            source = self.container / f"powershell-source-{index}"
            subprocess.run(["git", "clone", "--bare", str(initial_root), str(remote)], check=True, capture_output=True)
            subprocess.run(["git", "clone", "--branch", "main", str(remote), str(source)], check=True, capture_output=True)
            self.root = source
            self.git("config", "user.name", "Fixture User")
            self.git("config", "user.email", "fixture@example.invalid")
            self.git("config", "core.autocrlf", "false")
            prs.write_text("[]", encoding="utf-8")
            env = {**os.environ, "RELEASE_PS_SCRIPT": str(SCRIPT.with_suffix(".ps1")), "RELEASE_PRS": str(prs),
                   "RELEASE_SOURCE": str(source), "TEMP": str(self.container), "TMP": str(self.container), "RELEASE_PREVIEW": "true", "RELEASE_TAG": "v0.3.0"}
            before = {str(path.relative_to(source)): path.read_bytes() for path in source.rglob("*") if path.is_file()}
            preview = subprocess.run(shell_command(shell, runner), env=env, capture_output=True, text=True, encoding="utf-8", check=False)
            self.assertEqual(preview.returncode, 0, preview.stderr)
            self.assertEqual(json.loads(preview.stdout)["changed_files"], list(START.RELEASE_FILES))
            self.assertEqual(before, {str(path.relative_to(source)): path.read_bytes() for path in source.rglob("*") if path.is_file()})
            env["RELEASE_PREVIEW"] = "false"
            for attempt in range(2):
                with self.subTest(shell=shell, attempt=attempt):
                    result = subprocess.run(shell_command(shell, runner), env=env, capture_output=True, text=True, encoding="utf-8", check=False)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout.strip(), "https://github.com/owner/portals/pull/1")
            commit = self.git("rev-parse", "task/release-v0.3.0")
            START.validate_branch(self.root, commit, self.git("rev-parse", "main"), "v0.3.0")
            self.assertEqual(self.git("status", "--porcelain"), "")
            prs.write_text(json.dumps([{"url": "a"}, {"url": "b"}]), encoding="utf-8")
            result = subprocess.run(shell_command(shell, runner), env=env, capture_output=True, text=True, encoding="utf-8", check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Multiple release preparation PRs", result.stderr)
            prs.write_text("[]", encoding="utf-8")
            self.git("merge", "--no-ff", "task/release-v0.3.0", "-m", "Merge first stable release")
            first_raw = (source / "ReleaseNotes.md").read_bytes().decode("utf-8")
            notes = (source / "ReleaseNotes.md").read_text(encoding="utf-8")
            (source / "ReleaseNotes.md").write_text(notes.replace("## Unreleased\n\n", "## Unreleased\n\n- Manually documented the next feature.\n\n", 1), encoding="utf-8")
            self.git("add", "ReleaseNotes.md")
            self.git("commit", "-m", "Document the next feature")
            self.git("push", "origin", "main")
            env["RELEASE_TAG"] = "v0.4.0"
            result = subprocess.run(shell_command(shell, runner), env=env, capture_output=True, text=True, encoding="utf-8", check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            next_commit = self.git("rev-parse", "task/release-v0.4.0")
            START.validate_branch(self.root, next_commit, self.git("rev-parse", "main"), "v0.4.0")
            self.assertTrue(START.files_at(self.root, next_commit)["ReleaseNotes.md"].endswith(first_raw[first_raw.index("## v0.3.0 ("):]))
        self.root = initial_root


class PowerShellParityTests(unittest.TestCase):
    @unittest.skipUnless(POWERSHELLS, "PowerShell is unavailable")
    def test_shared_transform_fixtures_in_powershell_and_python(self) -> None:
        cases = [{"cmake": CM, "notes": NOTES, "tag": tag, "date": "2026-10-10"}
                 for tag in ("v0.2.0", "v0.3.0", "v0.3.0-rc1")]
        first = START.prepare_files(CM, NOTES, "v0.3.0-rc1", "2026-10-08")
        second = START.prepare_files(first["CMakeLists.txt"], first["ReleaseNotes.md"], "v0.3.0-rc2", "2026-10-09")
        cases.extend([{"cmake": first["CMakeLists.txt"], "notes": first["ReleaseNotes.md"], "tag": "v0.3.0-rc2", "date": "2026-10-09"},
                      {"cmake": second["CMakeLists.txt"], "notes": second["ReleaseNotes.md"], "tag": "v0.3.0", "date": "2026-10-10"},
                      {"cmake": CM.replace("\n", "\r\n"), "notes": NOTES.replace("\n", "\r\n"), "tag": "v0.2.0", "date": "2026-10-10"}])
        for notes in ["", "## v0.2.0 (Unreleased)\n\n", NOTES + "## v0.2.0\nDuplicate.\n",
                      "```\n## v0.2.0 (Unreleased)\nBody.\n```\n", NOTES + "## v0.3.0 (Unreleased)\nOther.\n"]:
            cases.append({"cmake": CM, "notes": notes, "tag": "v0.3.0", "date": "2026-10-10"})
        cases.append({"cmake": CM, "notes": NOTES, "tag": "v0.3.0-rc0", "date": "2026-10-10"})
        generic = NOTES.replace("## v0.2.0 (Unreleased)", "## Unreleased", 1)
        for tag in ("v0.3.0", "v0.3.0-rc1"):
            cases.append({"cmake": CM, "notes": generic, "tag": tag, "date": "2026-10-10"})
        stable = START.prepare_files(CM, NOTES, "v0.2.0", "2026-10-09")
        cases.append({"cmake": stable["CMakeLists.txt"], "notes": stable["ReleaseNotes.md"], "tag": "v0.3.0", "date": "2026-10-10"})
        working = stable["ReleaseNotes.md"].replace("## Unreleased\n\n", "## Unreleased\n\n- Added another feature.\n\n", 1)
        cases.append({"cmake": stable["CMakeLists.txt"], "notes": working, "tag": "v0.3.0", "date": "2026-10-10"})
        expected = []
        for case in cases:
            try:
                expected.append({"ok": True, "files": START.prepare_files(case["cmake"], case["notes"], case["tag"], case["date"])})
            except ValueError:
                expected.append({"ok": False})
        parent = SCRIPT.parents[2] / "build/release-start-tests"
        parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=parent) as temporary:
            root = Path(temporary)
            inputs = root / "inputs.json"
            inputs.write_text(json.dumps(cases), encoding="utf-8")
            runner = root / "parity.ps1"
            runner.write_text(r'''
. $env:RELEASE_PS_SCRIPT
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$cases = [IO.File]::ReadAllText($env:RELEASE_CASES) | ConvertFrom-Json
$results = foreach ($case in $cases) {
    try {
        $files = Get-PreparedReleaseFiles $case.cmake $case.notes $case.tag $case.date
        @{ ok = $true; files = $files }
    } catch { @{ ok = $false } }
}
ConvertTo-Json -InputObject @($results) -Depth 8 -Compress
''', encoding="utf-8")
            for shell in POWERSHELLS:
                result = subprocess.run(shell_command(shell, runner), capture_output=True,
                                        text=True, encoding="utf-8", check=False,
                                        env={**os.environ, "RELEASE_PS_SCRIPT": str(SCRIPT.with_suffix(".ps1")), "RELEASE_CASES": str(inputs)})
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(json.loads(result.stdout), expected, shell)


class DispatchInputTests(unittest.TestCase):
    def test_rejects_unmerged_fork_wrong_branch_or_checkout(self) -> None:
        sha = "a" * 40
        event = {"action": "closed", "pull_request": {"merged": True, "merge_commit_sha": sha,
                 "base": {"ref": "main", "repo": {"full_name": REPOSITORY}},
                 "head": {"ref": "task/release-v0.3.0", "repo": {"full_name": REPOSITORY}}}}
        self.assertEqual(START.dispatch_inputs(event, REPOSITORY, sha), ("v0.3.0", sha))
        mutations = [("merged", False), ("head_ref", "task/release-v0.3.0;evil"), ("head_repo", "fork/portals"),
                     ("base_ref", "development"), ("action", "opened"), ("merge_commit_sha", "b" * 40)]
        for key, value in mutations:
            changed = json.loads(json.dumps(event))
            if key == "head_ref":
                changed["pull_request"]["head"]["ref"] = value
            elif key == "head_repo":
                changed["pull_request"]["head"]["repo"]["full_name"] = value
            elif key == "base_ref":
                changed["pull_request"]["base"]["ref"] = value
            elif key == "action":
                changed["action"] = value
            else:
                changed["pull_request"][key] = value
            with self.subTest(key=key), self.assertRaises(START.StartError):
                START.dispatch_inputs(changed, REPOSITORY, sha)

    def test_tag_resolution_peels_annotated_tag_and_ignores_prefix_match(self) -> None:
        root = Path.cwd()
        responses = [[{"ref": "refs/tags/v0.3.01", "object": {"type": "commit", "sha": "c" * 40}},
                      {"ref": "refs/tags/v0.3.0", "object": {"type": "tag", "sha": "b" * 40}}],
                     {"object": {"type": "commit", "sha": "a" * 40}}]
        with patch.object(START, "api", side_effect=responses):
            self.assertEqual(START.existing_tag_commit(root, REPOSITORY, "v0.3.0"), "a" * 40)


if __name__ == "__main__":
    unittest.main()
