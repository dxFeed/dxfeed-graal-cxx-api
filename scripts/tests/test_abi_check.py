# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Tests of the choice of the reference release by scripts/abi_check.sh (baseline), with a fake gh."""

import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

SCRIPT = pathlib.Path(__file__).resolve().parents[1] / "abi_check.sh"

# A fake gh: FAKE_RELEASES lists the published stable releases as tag:asset or tag:none (gh release list with --jq
# prints the tags that pass the filter), and the download writes the tag into a minimal ABI XML.
FAKE_GH = r"""#!/bin/bash
set -euo pipefail
case "$1 $2" in
    "release list")
        for release in $FAKE_RELEASES; do echo "${release%%:*}"; done ;;
    "release view")
        for release in $FAKE_RELEASES; do
            if [ "${release%%:*}" = "$3" ] && [ "${release#*:}" = asset ]; then echo abi-x86_64-linux.xml; fi
        done
        echo other-asset.zip ;;
    "release download")
        tag="$3"
        out=""
        while [ $# -gt 0 ]; do [ "$1" = --output ] && out="$2"; shift; done
        printf '<abi-corpus-group>\n<abi-corpus tag="%s">\n</abi-corpus>\n</abi-corpus-group>\n' "$tag" > "$out" ;;
    *) echo "fake gh: unexpected $*" >&2; exit 1 ;;
esac
"""


@unittest.skipUnless(os.name == "posix" and shutil.which("bash"), "abi_check.sh runs in bash on Linux and macOS")
class BaselineTest(unittest.TestCase):
    def setUp(self):
        self.directory = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.directory)
        gh = self.directory / "gh"
        gh.write_text(FAKE_GH, encoding="utf-8")
        gh.chmod(0o755)

    def baseline(self, releases, major="9", excluded=None):
        out = self.directory / "reference.xml"
        environment = dict(os.environ, PATH=f"{self.directory}{os.pathsep}{os.environ['PATH']}",
                           FAKE_RELEASES=releases, GITHUB_REPOSITORY="owner/repository")
        environment.pop("GITHUB_ACTIONS", None)
        command = ["bash", str(SCRIPT), "baseline", major, str(out)] + ([excluded] if excluded else [])
        result = subprocess.run(command, capture_output=True, text=True, env=environment)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        tag = None
        if out.exists():
            tag = out.read_text(encoding="utf-8").split('tag="', 1)[1].split('"', 1)[0]
        return tag, result.stdout

    def test_the_highest_version_not_the_first_listed_or_the_lexically_highest(self):
        releases = "v9.0.0:asset v9.0.10:asset v9.2.0:asset v9.0.2:asset v9.1.0:asset"
        self.assertEqual(self.baseline(releases)[0], "v9.2.0")
        self.assertEqual(self.baseline("v9.9.0:asset v9.10.0:asset v9.10.2:asset v9.10.10:asset")[0], "v9.10.10")

    def test_a_release_without_the_asset_is_skipped(self):
        tag, output = self.baseline("v9.0.0:asset v9.1.0:asset v9.2.0:none")
        self.assertEqual(tag, "v9.1.0")
        self.assertIn("the release v9.2.0 has no abi-x86_64-linux.xml, skipped", output)

    def test_the_excluded_tag_is_skipped(self):
        # A run of a moved release tag does not compare the release with its own earlier asset.
        self.assertEqual(self.baseline("v9.0.0:asset v9.1.0:asset", excluded="v9.1.0")[0], "v9.0.0")

    def test_other_majors_and_pre_releases_are_ignored(self):
        self.assertEqual(self.baseline("v8.9.0:asset v10.0.0:asset v9.0.0:asset v9.1.0-rc1:asset")[0], "v9.0.0")

    def test_no_release_of_the_major_is_a_notice(self):
        tag, output = self.baseline("v8.1.0:asset v9.0.0:none")
        self.assertIsNone(tag)
        self.assertIn("nothing to compare the ABI with", output)


if __name__ == "__main__":
    unittest.main()
