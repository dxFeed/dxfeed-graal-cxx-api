# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
"""Publication lifecycle checks using an in-memory GitHub boundary."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("release_publish", ROOT / "tools/release/publish.py")
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class GitHub:
    def __init__(self):
        self.release = None
        self.latest = None
        self.data = {}
        self.writes = []

    def resolve_tag(self, tag):
        return "a" * 40

    def get_release(self, tag):
        return self.release

    def get_release_id(self, release_id):
        return self.release

    def get_latest(self):
        return self.latest

    def create_draft(self, payload):
        self.writes.append("create")
        self.release = {**payload, "id": 1, "assets": []}
        return self.release

    def upload(self, release_id, path):
        self.writes.append(path.name)
        asset_id = len(self.data) + 1
        self.data[asset_id] = path.read_bytes()
        self.release["assets"].append({"id": asset_id, "name": path.name, "size": path.stat().st_size})

    def download(self, asset):
        return self.data[asset["id"]]

    def publish(self, release_id, payload):
        self.writes.append("publish")
        self.release.update(payload)
        if payload["make_latest"] == "true":
            self.latest = self.release


class Publication(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.directory = Path(self.temp.name)
        (self.directory / "bundle.zip").write_bytes(b"verified payload")
        self.github = GitHub()

    def tearDown(self):
        self.temp.cleanup()

    def run_release(self, tag="v0.2.0", commit="a" * 40):
        return MODULE.publish_release(self.github, self.directory, tag, commit, "Release notes")

    def test_rc_never_latest(self):
        result = self.run_release("v0.2.0-rc1")
        self.assertFalse(result)
        self.assertTrue(self.github.release["prerelease"])
        self.assertEqual(self.github.release["make_latest"], "false")

    def test_stable_and_published_retry(self):
        self.assertTrue(self.run_release())
        writes = list(self.github.writes)
        self.assertTrue(self.run_release())
        self.assertEqual(self.github.writes, writes)

    def test_old_patch_preserves_latest(self):
        self.github.latest = {"tag_name": "v0.3.0"}
        self.assertFalse(self.run_release())
        self.assertEqual(self.github.latest["tag_name"], "v0.3.0")

    def test_published_retry_reports_actual_latest(self):
        self.run_release()
        self.github.latest = {"tag_name": "v0.1.0"}
        writes = list(self.github.writes)
        self.assertFalse(self.run_release())
        self.assertEqual(self.github.writes, writes)

    def test_commit_mismatch_rejects_before_write(self):
        with self.assertRaises(ValueError):
            self.run_release(commit="b" * 40)
        self.assertEqual(self.github.writes, [])

    def test_changed_published_asset_rejected(self):
        self.run_release()
        writes = list(self.github.writes)
        (self.directory / "bundle.zip").write_bytes(b"different payload")
        with self.assertRaises(ValueError):
            self.run_release()
        self.assertEqual(self.github.writes, writes)

    def test_partial_draft_resumes_without_overwrite(self):
        self.github.publish = lambda *_: (_ for _ in ()).throw(RuntimeError("interrupted"))
        with self.assertRaises(RuntimeError):
            self.run_release()
        self.assertTrue(self.github.release["draft"])
        writes = list(self.github.writes)
        self.github.publish = GitHub.publish.__get__(self.github)
        self.assertTrue(self.run_release())
        self.assertEqual(self.github.writes, writes + ["publish"])

    def test_api_draft_lookup_when_tag_endpoint_is_not_found(self):
        client = MODULE.GitHub("ttldtor/portals")
        calls = []

        def api(endpoint, **kwargs):
            calls.append(endpoint)
            return None if endpoint.startswith("releases/tags/") else [{"id": 12, "tag_name": "v0.2.0", "draft": True}]

        client.api = api
        self.assertEqual(client.get_release("v0.2.0")["id"], 12)
        self.assertEqual(len(calls), 2)


class BuildInventory(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.directory = Path(self.temp.name)
        self.tag = "v0.2.0-rc1"
        self.commit = "a" * 40
        names = {
            "windows": [f"portals-{self.tag}-windows-x64-release.zip"],
            "linux": [f"portals-{self.tag}-linux-x64-release.tar.gz",
                      f"portals-{self.tag}-source.zip", f"portals-{self.tag}-source.tar.gz",
                      f"portals-{self.tag}-docs.zip"],
        }
        for platform, assets in names.items():
            artifacts = []
            for name in assets:
                path = self.directory / name
                path.write_bytes(name.encode())
                artifacts.append({"name": name, "size": path.stat().st_size,
                                  "sha256": MODULE.digest(path.read_bytes())})
            manifest = {"tag": self.tag, "commit": self.commit, "platform": platform,
                        "configuration": "Release", "architecture": "x64", "schema_version": 1,
                        "artifacts": artifacts,
                        "gates": {name: "passed" for name in
                                  ("debug_headless", "release_headless", "bundle_smoke", "html_links")}}
            (self.directory / f"release-manifest-{platform}.json").write_text(json.dumps(manifest), encoding="utf-8")

    def tearDown(self):
        self.temp.cleanup()

    def validate(self):
        MODULE.validate_builds(self.directory, self.tag, self.commit)

    def test_complete_candidates_can_be_assembled_without_github(self):
        self.validate()
        assets = MODULE.inventory(self.directory, self.tag, self.commit)
        self.assertEqual(len(assets), 9)
        self.assertIn("SHA256SUMS", assets)
        # A local retry verifies the same builder inventories, including generated files.
        self.validate()

    def test_modified_bundle_is_rejected(self):
        (self.directory / f"portals-{self.tag}-windows-x64-release.zip").write_bytes(b"changed")
        with self.assertRaisesRegex(ValueError, "checksum"):
            self.validate()

    def test_missing_gate_is_rejected(self):
        path = self.directory / "release-manifest-linux.json"
        manifest = json.loads(path.read_text(encoding="utf-8"))
        manifest["gates"]["debug_headless"] = "failed"
        path.write_text(json.dumps(manifest), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "gate"):
            self.validate()

    def test_unlisted_asset_is_rejected(self):
        (self.directory / "private.txt").write_text("Must never be uploaded", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "unlisted"):
            self.validate()


if __name__ == "__main__":
    unittest.main()
