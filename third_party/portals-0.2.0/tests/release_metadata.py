# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
"""Offline contract tests for release metadata preparation."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "tools/release/metadata.py"


class ReleaseMetadataTests(unittest.TestCase):
    def prepare(self, notes: str, tag: str = "v0.2.0", cmake: str = "0.2.0") -> tuple[subprocess.CompletedProcess[str], Path]:
        temporary = tempfile.TemporaryDirectory(dir=SCRIPT.parents[2])
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        (root / "CMakeLists.txt").write_text(f"project(Portals VERSION {cmake} LANGUAGES C CXX)\n", encoding="utf-8")
        (root / "ReleaseNotes.md").write_text(notes, encoding="utf-8")
        result = subprocess.run(
            [sys.executable, str(SCRIPT), "prepare", "--tag", tag,
             "--repository", "owner/portals", "--source", str(root),
             "--output-dir", str(root / "out"), "--github-output", str(root / "outputs")],
            capture_output=True, text=True, check=False,
        )
        return result, root

    def test_stable_metadata_and_outputs(self) -> None:
        result, root = self.prepare("## v0.2.0 (2026-10-10)\n\n- Added features.\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads((root / "out/release.json").read_text()), {
            "tag": "v0.2.0", "version": "0.2.0", "prerelease": False,
            "notes_section": "v0.2.0 (2026-10-10)",
        })
        self.assertEqual((root / "outputs").read_text(), "tag=v0.2.0\nversion=0.2.0\nprerelease=false\n")

    def test_exact_rc_has_priority_and_banner(self) -> None:
        result, root = self.prepare("## v0.2.0 (Unreleased)\nBase body.\n## v0.2.0-rc2 (2026-10-10)\nExact body.\n", "v0.2.0-rc2")
        self.assertEqual(result.returncode, 0, result.stderr)
        notes = (root / "out/release-notes.md").read_text()
        self.assertIn("release candidate", notes.lower())
        self.assertIn("Exact body.", notes)
        self.assertNotIn("Base body.", notes)

    def test_rc_falls_back_to_unreleased_base(self) -> None:
        result, root = self.prepare("## v0.2.0 (Unreleased)\nBase body.\n", "v0.2.0-rc1")
        self.assertEqual(result.returncode, 0, result.stderr)
        metadata = json.loads((root / "out/release.json").read_text())
        self.assertTrue(metadata["prerelease"])
        self.assertEqual(metadata["notes_section"], "v0.2.0 (Unreleased)")

    def test_rc_does_not_fall_back_from_invalid_exact_section(self) -> None:
        for exact in ["", "Body.\n## v0.2.0-rc1\nDuplicate."]:
            with self.subTest(exact=exact):
                result, _ = self.prepare(f"## v0.2.0\nBase body.\n## v0.2.0-rc1\n{exact}\n", "v0.2.0-rc1")
                self.assertNotEqual(result.returncode, 0)

    def test_rc_uses_published_base(self) -> None:
        result, root = self.prepare("## v0.2.0 (2026-10-10)\nBase body.\n", "v0.2.0-rc12")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Base body.", (root / "out/release-notes.md").read_text())

    def test_relative_link_parentheses_and_title(self) -> None:
        result, root = self.prepare('## v0.2.0\n[Guide](docs/guide(test).md "Guide title")\n')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('https://github.com/owner/portals/blob/v0.2.0/docs/guide(test).md "Guide title"', (root / "out/release-notes.md").read_text())

    def test_invalid_tags(self) -> None:
        for tag in ["0.2.0", "v00.2.0", "v0.02.0", "v0.2.00", "v0.2.0-rc0", "v0.2.0-rc01", "v0.2.0-beta1", "v0.2.0\n"]:
            with self.subTest(tag=tag):
                result, _ = self.prepare("## v0.2.0 (2026-10-10)\nBody.\n", tag)
                self.assertNotEqual(result.returncode, 0)

    def test_version_mismatch(self) -> None:
        result, _ = self.prepare("## v0.2.0 (2026-10-10)\nBody.\n", cmake="0.3.0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("CMake", result.stderr)

    def test_rejects_missing_duplicate_empty_and_stable_unreleased(self) -> None:
        for notes in ["## v0.1.0\nOther.\n", "## v0.2.0\nOne.\n## v0.2.0\nTwo.\n", "## v0.2.0\n\n", "## v0.2.0 (Unreleased)\nBody.\n"]:
            with self.subTest(notes=notes):
                result, _ = self.prepare(notes)
                self.assertNotEqual(result.returncode, 0)

    def test_nested_headings_links_and_fences(self) -> None:
        body = """## v0.2.0 (2026-10-10)
[Guide](docs/guide.md#start) [Home](./README.md) [Site](https://example.com) [Anchor](#start)
### Limitations
Keep nested headings.
[ref]: docs/limits.md
```markdown
## v0.2.0
[Code](docs/code.md)
```
## v0.1.0
Old body.
"""
        result, root = self.prepare(body)
        self.assertEqual(result.returncode, 0, result.stderr)
        notes = (root / "out/release-notes.md").read_text()
        self.assertIn("https://github.com/owner/portals/blob/v0.2.0/docs/guide.md#start", notes)
        self.assertIn("https://github.com/owner/portals/blob/v0.2.0/README.md", notes)
        self.assertIn("[ref]: https://github.com/owner/portals/blob/v0.2.0/docs/limits.md", notes)
        self.assertIn("### Limitations", notes)
        self.assertIn("[Code](docs/code.md)", notes)
        self.assertIn("[Site](https://example.com) [Anchor](#start)", notes)
        self.assertNotIn("Old body.", notes)


if __name__ == "__main__":
    unittest.main()
