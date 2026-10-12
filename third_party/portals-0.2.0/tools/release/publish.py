# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
"""Publish verified assets through a resumable GitHub draft release."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import quote

TAG = re.compile(r"v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(?:-rc([1-9][0-9]*))?")


def version(tag):
    match = TAG.fullmatch(tag)
    if not match:
        raise ValueError("Unsupported release tag")
    return tuple(map(int, match.groups()[:3])), match.group(4) is not None


class GitHub:
    def __init__(self, repository):
        if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository):
            raise ValueError("Invalid repository")
        self.repository = repository

    def api(self, endpoint, method="GET", payload=None, missing=False):
        args = ["gh", "api", f"repos/{self.repository}/{endpoint}", "--method", method]
        data = None
        if payload is not None:
            args += ["--input", "-"]
            data = json.dumps(payload).encode()
        result = subprocess.run(args, input=data, capture_output=True, check=False)
        if result.returncode:
            if missing and b"(HTTP 404)" in result.stderr:
                return None
            raise RuntimeError("GitHub API request failed; publication stopped")
        return json.loads(result.stdout) if result.stdout else None

    def resolve_tag(self, tag):
        obj = self.api(f"git/ref/tags/{quote(tag, safe='')}")["object"]
        for _ in range(8):
            if obj["type"] == "commit":
                return obj["sha"]
            if obj["type"] != "tag":
                break
            obj = self.api(f"git/tags/{obj['sha']}")["object"]
        raise ValueError("Tag does not resolve to a commit")

    def get_release(self, tag):
        published = self.api(f"releases/tags/{quote(tag, safe='')}", missing=True)
        if published is not None:
            return published
        # Get-by-tag only promises published releases; drafts need an authenticated listing.
        matches = []
        for page in range(1, 101):
            releases = self.api(f"releases?per_page=100&page={page}")
            matches.extend(release for release in releases if release["tag_name"] == tag)
            if len(releases) < 100:
                break
        else:
            raise ValueError("Release listing exceeded the supported bound")
        if len(matches) > 1:
            raise ValueError("Multiple releases match the requested tag")
        return matches[0] if matches else None

    def get_release_id(self, release_id):
        return self.api(f"releases/{release_id}")

    def get_latest(self):
        return self.api("releases/latest", missing=True)

    def create_draft(self, payload):
        return self.api("releases", "POST", payload)

    def upload(self, release_id, path):
        endpoint = f"https://uploads.github.com/repos/{self.repository}/releases/{release_id}/assets?name={quote(path.name, safe='')}"
        result = subprocess.run(["gh", "api", endpoint, "--method", "POST", "-H",
                                 "Content-Type: application/octet-stream", "--input", str(path)],
                                capture_output=True, check=False)
        if result.returncode:
            raise RuntimeError("Asset upload failed; draft retained")

    def download(self, asset):
        result = subprocess.run(["gh", "api", f"repos/{self.repository}/releases/assets/{asset['id']}",
                                 "-H", "Accept: application/octet-stream", "--allow-escape-sequences"],
                                capture_output=True, check=False)
        if result.returncode:
            raise RuntimeError("Cannot verify existing release asset")
        return result.stdout

    def publish(self, release_id, payload):
        return self.api(f"releases/{release_id}", "PATCH", payload)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def inventory(directory, tag, commit):
    excluded = {"release.json", "release-notes.md", "release-manifest.json", "SHA256SUMS"}
    assets = {}
    for path in sorted(directory.iterdir()):
        if path.name in excluded:
            continue
        if not path.is_file() or path.is_symlink() or not re.fullmatch(r"[A-Za-z0-9_.-]+", path.name):
            raise ValueError("Release directory must contain only safely named regular files")
        assets[path.name] = {"size": path.stat().st_size, "sha256": digest(path.read_bytes())}
    if not assets:
        raise ValueError("No release assets")
    manifest = {"tag": tag, "commit": commit, "assets": assets}
    manifest_path = directory / "release-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    assets[manifest_path.name] = {"size": manifest_path.stat().st_size, "sha256": digest(manifest_path.read_bytes())}
    sums = directory / "SHA256SUMS"
    sums.write_text("".join(f"{item['sha256']}  {name}\n" for name, item in assets.items()), encoding="utf-8")
    assets[sums.name] = {"size": sums.stat().st_size, "sha256": digest(sums.read_bytes())}
    return assets


def validate_builds(directory, tag, commit):
    """Require both verified builders and their complete, unmodified asset inventories."""
    expected = set()
    for platform in ("windows", "linux"):
        path = directory / f"release-manifest-{platform}.json"
        manifest = json.loads(path.read_text(encoding="utf-8"))
        if (manifest.get("tag") != tag or manifest.get("commit") != commit
                or manifest.get("platform") != platform):
            raise ValueError("Build manifest does not match the release commit")
        gates = manifest.get("gates", {})
        for gate in ("debug_headless", "release_headless", "bundle_smoke", "html_links"):
            if gates.get(gate) != "passed":
                raise ValueError(f"Missing successful build gate: {platform}/{gate}")
        for asset in manifest["artifacts"]:
            name = asset["name"]
            if not re.fullmatch(r"[A-Za-z0-9_.-]+", name) or name in expected:
                raise ValueError("Invalid or duplicate build asset")
            item = directory / name
            if (item.is_symlink() or not item.is_file() or item.stat().st_size != asset["size"]
                    or digest(item.read_bytes()) != asset["sha256"]):
                raise ValueError("Build asset failed checksum verification")
            expected.add(name)
        expected.add(path.name)
    required = {f"portals-{tag}-windows-x64-release.zip", f"portals-{tag}-linux-x64-release.tar.gz",
                f"portals-{tag}-source.zip", f"portals-{tag}-source.tar.gz", f"portals-{tag}-docs.zip"}
    if not required.issubset(expected):
        raise ValueError("The release artifact set is incomplete")
    metadata_files = {"release.json", "release-notes.md", "release-manifest.json", "SHA256SUMS"}
    actual = {path.name for path in directory.iterdir()}
    if actual - metadata_files != expected:
        raise ValueError("The release directory has unlisted build assets")


def publish_release(github, directory, tag, commit, notes):
    number, prerelease = version(tag)
    if not re.fullmatch(r"[0-9a-f]{40}", commit) or github.resolve_tag(tag) != commit:
        raise ValueError("Remote tag commit differs from the verified build")
    assets = inventory(directory, tag, commit)
    body = notes.rstrip() + f"\n\n<!-- portals-release-commit:{commit} -->\n"
    release = github.get_release(tag)
    if release is not None:
        if (release.get("body") != body or release.get("prerelease") != prerelease
                or release.get("target_commitish") != commit):
            raise ValueError("Existing release metadata differs; refusing to modify it")
    else:
        release = github.create_draft({"tag_name": tag, "target_commitish": commit,
                                       "name": tag, "body": body, "draft": True,
                                       "prerelease": prerelease})
    remote = {asset["name"]: asset for asset in release["assets"]}
    if len(remote) != len(release["assets"]) or set(remote) - set(assets):
        raise ValueError("Existing release has unexpected or duplicate assets")
    for name, asset in remote.items():
        expected = assets[name]
        if asset["size"] != expected["size"] or digest(github.download(asset)) != expected["sha256"]:
            raise ValueError("Existing asset checksum differs; refusing to overwrite")
    if not release["draft"] and set(remote) != set(assets):
        raise ValueError("Published release has an incomplete asset set")
    latest = github.get_latest()
    latest_number = version(latest["tag_name"])[0] if latest else None
    make_latest = not prerelease and (latest_number is None or number > latest_number)
    if release["draft"]:
        for name in assets.keys() - remote.keys():
            github.upload(release["id"], directory / name)
        # Re-fetch after upload so publication never relies only on transport success.
        complete = github.get_release_id(release["id"])
        uploaded = {asset["name"]: asset for asset in complete["assets"]}
        if set(uploaded) != set(assets):
            raise ValueError("Draft upload inventory is incomplete")
        for name, asset in uploaded.items():
            if digest(github.download(asset)) != assets[name]["sha256"]:
                raise ValueError("Uploaded asset failed checksum verification")
        github.publish(release["id"], {"draft": False, "prerelease": prerelease,
                                      "make_latest": "true" if make_latest else "false"})
    actual_latest = github.get_latest() if not prerelease else None
    return bool(actual_latest and actual_latest["tag_name"] == tag)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--repository", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--github-output", type=Path)
    parser.add_argument("--dry-run", action="store_true", help="Validate and inventory local artifacts without GitHub access")
    args = parser.parse_args()
    metadata = json.loads((args.directory / "release.json").read_text(encoding="utf-8"))
    if metadata["tag"] != args.tag or metadata["prerelease"] != version(args.tag)[1]:
        raise ValueError("Release metadata does not match tag")
    notes = (args.directory / "release-notes.md").read_text(encoding="utf-8")
    validate_builds(args.directory, args.tag, args.commit)
    if args.dry_run:
        inventory(args.directory, args.tag, args.commit)
        return
    latest = publish_release(GitHub(args.repository), args.directory, args.tag, args.commit, notes)
    if args.github_output:
        with args.github_output.open("a", encoding="utf-8") as output:
            output.write(f"latest={str(bool(latest)).lower()}\npages_deploy={str(bool(latest)).lower()}\n")


if __name__ == "__main__":
    main()
