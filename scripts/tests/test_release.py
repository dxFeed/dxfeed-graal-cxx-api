# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Tests of scripts/release.py and scripts/release.cmake: python3 -m unittest discover -s scripts/tests
(the functions of release.cmake: cmake -P scripts/tests/test_release.cmake)."""

import contextlib
import io
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

SCRIPTS = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPTS))

import release  # noqa: E402
from release import ReleaseError, Version  # noqa: E402

ITEMS = "* New item.\n    * Detail; with a semicolon.\n"
OLD = "## v8.1.0\n\n* Old item.\n"


def notes(*parts):
    return "\n".join(parts)


class VersionTest(unittest.TestCase):
    def test_parse(self):
        self.assertEqual(Version.parse("v8.2.0"), Version(8, 2, 0))
        self.assertEqual(Version.parse("v8.2.0-rc2"), Version(8, 2, 0, "rc", 2))
        self.assertTrue(Version.parse("v9.0.0-draft1").is_draft)
        self.assertEqual(str(Version.parse("v10.0.1-beta3")), "v10.0.1-beta3")

        for text in ("8.2.0", "v8.2", "v8.2.0-rc", "v8.2.0-gamma1", "v8.2.0-RC1"):
            with self.subTest(text), self.assertRaises(ReleaseError):
                Version.parse(text)

    def test_pre_release_order(self):
        keys = [Version.parse(f"v8.2.0-{s}").pre_release_key() for s in ("alpha1", "alpha2", "beta1", "pre1", "rc1",
                                                                          "rc10")]
        self.assertEqual(keys, sorted(keys))


class UpdateNotesTest(unittest.TestCase):
    def update(self, text, version):
        return release.update_notes(text, Version.parse(version))[0]

    def test_pre_release_over_unreleased_items_inserts_the_heading(self):
        self.assertEqual(self.update(notes(ITEMS, OLD), "v8.2.0-rc1"), notes("## v8.2.0-rc1\n", ITEMS, OLD))

    def test_later_pre_release_replaces_the_heading(self):
        text = notes("## v8.2.0-rc1\n", ITEMS, OLD)

        self.assertEqual(self.update(text, "v8.2.0-rc2"), notes("## v8.2.0-rc2\n", ITEMS, OLD))
        self.assertEqual(self.update(notes("## v8.2.0-beta3\n", ITEMS, OLD), "v8.2.0-rc1"),
                         notes("## v8.2.0-rc1\n", ITEMS, OLD))

    def test_release_replaces_the_pre_release_heading(self):
        self.assertEqual(self.update(notes("## v8.2.0-rc3\n", ITEMS, OLD), "v8.2.0"), notes("## v8.2.0\n", ITEMS, OLD))

    def test_release_over_unreleased_items_inserts_the_heading(self):
        self.assertEqual(self.update(notes(ITEMS, OLD), "v8.2.0"), notes("## v8.2.0\n", ITEMS, OLD))

    def test_items_added_after_a_pre_release_join_its_section(self):
        text = notes("* Added after rc1.\n", "## v8.2.0-rc1\n", ITEMS, OLD)

        self.assertEqual(self.update(text, "v8.2.0-rc2"), notes("## v8.2.0-rc2\n", "* Added after rc1.\n", ITEMS, OLD))
        self.assertEqual(self.update(text, "v8.2.0"), notes("## v8.2.0\n", "* Added after rc1.\n", ITEMS, OLD))

    def test_draft_changes_nothing(self):
        for text in (notes(ITEMS, OLD), notes("## v8.2.0-rc1\n", ITEMS, OLD), OLD):
            with self.subTest(text):
                self.assertEqual(self.update(text, "v8.2.0-draft1"), text)

    def test_errors(self):
        cases = {
            "an earlier pre-release": (notes("## v8.2.0-rc2\n", ITEMS, OLD), "v8.2.0-rc1"),
            "the same pre-release": (notes("## v8.2.0-rc2\n", ITEMS, OLD), "v8.2.0-rc2"),
            "alpha after rc": (notes("## v8.2.0-rc1\n", ITEMS, OLD), "v8.2.0-alpha1"),
            "a released version": (notes("## v8.2.0\n", ITEMS, OLD), "v8.2.0"),
            "a pre-release after its release": (notes("## v8.2.0\n", ITEMS, OLD), "v8.2.0-rc1"),
            "no unreleased items": (OLD, "v8.2.0"),
            "an older version": (notes(ITEMS, OLD), "v8.0.5"),
            "a heading that is not a version": (notes(ITEMS, "## Unreleased\n"), "v8.2.0"),
        }

        for name, (text, version) in cases.items():
            with self.subTest(name), self.assertRaises(ReleaseError):
                self.update(text, version)


class NotesOfTest(unittest.TestCase):
    def test_release_section(self):
        text = notes("## v8.2.0\n", ITEMS, OLD)

        self.assertEqual(release.notes_of(text, Version.parse("v8.2.0")), ITEMS)
        self.assertEqual(release.notes_of(text, Version.parse("v8.1.0")), "* Old item.\n")

    def test_draft_previews_the_unreleased_items(self):
        self.assertEqual(release.notes_of(notes(ITEMS, OLD), Version.parse("v8.2.0-draft1")), ITEMS)

    def test_missing_section(self):
        with self.assertRaises(ReleaseError):
            release.notes_of(notes(ITEMS, OLD), Version.parse("v8.2.0"))


class SetVersionsTest(unittest.TestCase):
    FILES = {
        release.CMAKE: 'set(DXFCXX_VERSION "v8.1.0" CACHE STRING "The package version")\n',
        release.DOXYFILE: "PROJECT_NAME = x\nPROJECT_NUMBER         = v8.1.0\n",
        release.CONF: ("#    define DXFCPP_BEGIN_NAMESPACE \\\n"
                       "        namespace dxfcpp { \\\n"
                       "        inline namespace v8 {\n"),
    }

    def test_all_places(self):
        result, _ = release.set_versions(self.FILES, Version.parse("v9.0.0-rc1"))

        self.assertIn('set(DXFCXX_VERSION "v9.0.0-rc1" CACHE', result[release.CMAKE])
        self.assertIn("PROJECT_NUMBER         = v9.0.0-rc1\n", result[release.DOXYFILE])
        self.assertIn("inline namespace v9 {", result[release.CONF])

    def test_missing_place(self):
        with self.assertRaises(ReleaseError):
            release.set_versions({**self.FILES, release.DOXYFILE: "PROJECT_NAME = x\n"}, Version.parse("v8.2.0"))


@unittest.skipUnless(shutil.which("git"), "git is needed")
class PrepareTest(unittest.TestCase):
    """The whole release in a temporary git repository with release.py (the subclass: with release.cmake). "origin" is
    a local bare repository, so --push is tested too."""

    NOTES = notes(ITEMS, OLD)
    LINE_END = "\r\n"  # CRLF is kept (the agreement test also uses LF)

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = pathlib.Path(self.directory.name) / "repository"
        self.remote = pathlib.Path(self.directory.name) / "origin.git"
        self.root.mkdir()

        for path, text in {**SetVersionsTest.FILES, release.NOTES: self.NOTES}.items():
            (self.root / path).parent.mkdir(parents=True, exist_ok=True)
            (self.root / path).write_bytes(text.replace("\n", self.LINE_END).encode())

        (self.root / "scripts").mkdir(exist_ok=True)
        shutil.copy(SCRIPTS / "release.cmake", self.root / "scripts" / "release.cmake")
        subprocess.run(["git", "init", "-q", "--bare", str(self.remote)], check=True)
        self.git("init", "-q", "-b", "main")
        self.git("config", "user.name", "Test")
        self.git("config", "user.email", "test@example.com")
        self.git("config", "commit.gpgsign", "false")
        self.git("config", "core.autocrlf", "false")
        self.git("remote", "add", "origin", str(self.remote))
        self.git("add", ".")
        self.git("commit", "-q", "-m", "initial")

    def tearDown(self):
        self.directory.cleanup()

    def git(self, *args, cwd=None):
        return subprocess.run(["git", *args], cwd=cwd or self.root, check=True, capture_output=True,
                              text=True).stdout.strip()

    def run_release(self, *args):
        """(exit code, stdout) of release.py in the repository."""
        saved = release.ROOT
        release.ROOT = self.root
        output = io.StringIO()

        try:
            with contextlib.redirect_stdout(output), contextlib.redirect_stderr(io.StringIO()):
                return release.main(list(args)), output.getvalue()
        finally:
            release.ROOT = saved

    def test_release_commits_and_tags(self):
        self.assertEqual(self.run_release("v8.2.0")[0], 0)
        self.assertEqual(self.git("log", "-1", "--format=%s"), "v8.2.0")
        self.assertEqual(self.git("tag"), "v8.2.0")
        self.assertTrue((self.root / release.NOTES).read_bytes().startswith(b"## v8.2.0\r\n\r\n* New item."))
        self.assertIn(b'"v8.2.0"', (self.root / release.CMAKE).read_bytes())
        self.assertEqual(self.git("ls-remote", "--tags", str(self.remote)), "")  # nothing is pushed

    def test_push(self):
        self.assertEqual(self.run_release("v8.2.0", "--push")[0], 0)
        self.assertEqual(self.git("rev-parse", "main", cwd=self.remote), self.git("rev-parse", "HEAD"))
        self.assertIn("refs/tags/v8.2.0", self.git("ls-remote", "--tags", str(self.remote)))

    def test_dry_run_changes_nothing(self):
        head = self.git("rev-parse", "HEAD")
        code, output = self.run_release("v8.2.0", "--dry-run")

        self.assertEqual(code, 0)
        self.assertIn("ReleaseNotes.md: ## v8.2.0 inserted above the unreleased items", output)
        self.assertIn("CMakeLists.txt: DXFCXX_VERSION v8.1.0 -> v8.2.0", output)
        self.assertEqual(self.git("rev-parse", "HEAD"), head)
        self.assertEqual(self.git("status", "--porcelain"), "")
        self.assertEqual(self.git("tag"), "")

    def test_draft_only_tags(self):
        head = self.git("rev-parse", "HEAD")

        self.assertEqual(self.run_release("v8.2.0-draft1", "--push")[0], 0)
        self.assertEqual(self.git("rev-parse", "HEAD"), head)
        self.assertEqual(self.git("status", "--porcelain"), "")
        self.assertEqual(self.git("tag"), "v8.2.0-draft1")
        self.assertIn("refs/tags/v8.2.0-draft1", self.git("ls-remote", "--tags", str(self.remote)))

    def test_refuses_a_dirty_tree_an_existing_tag_and_bad_arguments(self):
        (self.root / "new.txt").write_text("x")
        self.assertNotEqual(self.run_release("v8.2.0")[0], 0)

        (self.root / "new.txt").unlink()
        self.git("tag", "v8.2.0")
        self.assertNotEqual(self.run_release("v8.2.0")[0], 0)
        self.assertNotEqual(self.run_release("v8.2.1", "--push", "--dry-run")[0], 0)
        self.assertNotEqual(self.run_release("8.2.1")[0], 0)

    def test_long_release_notes(self):
        # Hundreds of kilobytes, as ReleaseNotes.md grows: a recursive regular expression of release.cmake overflowed
        # the stack on the real file.
        unreleased = "".join(f"* Item {i}; with a semicolon.\n    * Detail {i}.\n" for i in range(100))
        old = "".join(f"## v7.{i}.0\n\n" + "* Old item, long enough to make the file big.\n" * 40 + "\n"
                      for i in range(500, 0, -1))
        (self.root / release.NOTES).write_bytes(notes(unreleased, OLD, old).replace("\n", self.LINE_END).encode())
        self.git("commit", "-q", "-am", "long notes")

        self.assertEqual(self.run_release("v8.2.0", "--dry-run")[0], 0)
        self.assertEqual(self.run_release("v8.2.0")[0], 0)
        code, output = self.run_release("v8.2.0", "--notes")

        self.assertEqual(code, 0)
        self.assertEqual(output.replace("\r\n", "\n"), unreleased)

    def test_notes_of_a_ref(self):
        self.run_release("v8.2.0")
        code, output = self.run_release("v8.2.0", "--notes", "--ref", "v8.2.0")

        self.assertEqual(code, 0)
        self.assertEqual(output.replace("\r\n", "\n"), ITEMS)


@unittest.skipUnless(shutil.which("git") and shutil.which("cmake"), "git and cmake are needed")
class CMakePrepareTest(PrepareTest):
    """The same with scripts/release.cmake."""

    def run_release(self, *args):
        result = subprocess.run(["cmake", "-P", "scripts/release.cmake", *args], cwd=self.root, capture_output=True,
                                text=True)

        return result.returncode, result.stdout


@unittest.skipUnless(shutil.which("git") and shutil.which("cmake"), "git and cmake are needed")
class AgreementTest(unittest.TestCase):
    """release.py and release.cmake give the same files, commits, tags and notes."""

    CASES = {
        "rc1": (notes(ITEMS, OLD), "v8.2.0-rc1"),
        "rc2 after rc1 with new items": (notes("* Added after rc1.\n", "## v8.2.0-rc1\n", ITEMS, OLD), "v8.2.0-rc2"),
        "release after rc": (notes("## v8.2.0-rc3\n", ITEMS, OLD), "v8.2.0"),
        "a new major release": (notes(ITEMS, OLD), "v9.0.0"),
        "draft": (notes(ITEMS, OLD), "v8.2.0-draft1"),
        "error: an earlier pre-release": (notes("## v8.2.0-rc2\n", ITEMS, OLD), "v8.2.0-rc1"),
    }

    @staticmethod
    def results(implementation, text, version, line_end):
        test = implementation("run_release")
        test.NOTES = text
        test.LINE_END = line_end
        test.setUp()

        try:
            code, _ = test.run_release(version)
            files = {path: (test.root / path).read_bytes() for path in
                     (release.NOTES, release.CMAKE, release.DOXYFILE, release.CONF)}
            notes_output = test.run_release(version, "--notes")[1].replace("\r\n", "\n") if code == 0 else ""

            return code == 0, files, test.git("log", "-1", "--format=%s"), test.git("tag"), notes_output
        finally:
            test.tearDown()

    def test_same_results(self):
        for name, (text, version) in self.CASES.items():
            for line_end in ("\r\n", "\n"):
                with self.subTest(name, line_end=repr(line_end)):
                    python = self.results(PrepareTest, text, version, line_end)
                    cmake = self.results(CMakePrepareTest, text, version, line_end)

                    self.assertEqual(python, cmake)

                    if python[0]:
                        self.assertEqual(b"\r\n" in python[1][release.NOTES], line_end == "\r\n")


if __name__ == "__main__":
    unittest.main()
