# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Tests of scripts/sbom.py: python3 -m unittest discover -s scripts/tests"""

import contextlib
import io
import json
import pathlib
import re
import shutil
import sys
import tarfile
import tempfile
import unittest
import zipfile

SCRIPTS = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPTS))

import sbom  # noqa: E402
from sbom import SbomError  # noqa: E402

VERSION = "v9.0.0"
TIMESTAMP = "2026-11-18T10:00:00+03:00"
CMAKE = sbom.cmake_variables((sbom.ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
SDK = CMAKE["DXFEED_GRAAL_NATIVE_SDK_VERSION"]

POM = f"""<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0">
  <groupId>com.dxfeed</groupId>
  <artifactId>graal-native-sdk</artifactId>
  <version>{SDK}</version>
  <properties>
    <graalvm.version>99.0.0</graalvm.version>
    <qd.version>3.357</qd.version>
    <qd.alias>${{qd.version}}</qd.alias>
    <orcs-api.version>549</orcs-api.version>
  </properties>
  <dependencyManagement>
    <dependencies>
      <dependency><groupId>com.devexperts.qd</groupId><artifactId>qds</artifactId><version>${{qd.alias}}</version></dependency>
      <dependency><groupId>com.devexperts.qd</groupId><artifactId>dxfeed-api</artifactId><version>${{qd.version}}</version></dependency>
      <dependency><groupId>com.example</groupId><artifactId>bom</artifactId><version>1</version><scope>import</scope></dependency>
    </dependencies>
  </dependencyManagement>
  <dependencies>
    <dependency><groupId>com.devexperts.qd</groupId><artifactId>dxlink-websocket</artifactId><version>${{qd.version}}</version></dependency>
    <dependency><groupId>com.dxfeed</groupId><artifactId>orcs-api</artifactId><version>${{orcs-api.version}}</version></dependency>
    <dependency><groupId>com.devexperts.qd</groupId><artifactId>qds</artifactId></dependency>
    <dependency><groupId>junit</groupId><artifactId>junit</artifactId><version>4.13.2</version><scope>test</scope></dependency>
    <dependency><groupId>org.graalvm.sdk</groupId><artifactId>graal-sdk</artifactId><version>${{graalvm.version}}</version><scope>provided</scope></dependency>
    <dependency><groupId>org.graalvm.nativeimage</groupId><artifactId>svm</artifactId><version>1</version><scope>system</scope></dependency>
  </dependencies>
  <repositories>
    <repository><id>internal</id><url>https://repo.example.invalid/maven</url></repository>
  </repositories>
  <profiles>
    <profile>
      <id>extra</id>
      <dependencies>
        <dependency><groupId>com.example</groupId><artifactId>profile-only</artifactId><version>1</version></dependency>
      </dependencies>
    </profile>
  </profiles>
</project>
"""


def build_info(target, sdk_version=SDK, graalvm="25.5.5.0.3", java="25.0.5.0.3"):
    return (f"# comment\nsdk.version={sdk_version}\nvariant=\ntarget={target}\n"
            f"graalvm.implementor=GraalVM Community\ngraalvm.version={graalvm}\ngraalvm.java-version={java}\n"
            f"native-image.vendor-version=GraalVM CE {graalvm}+1.1\nnative-image.garbage-collector=Serial GC\n")


def name(rest, platform="x86_64-windows", version=VERSION, extension=".zip"):
    platform = f"-{platform}" if platform else ""
    return f"dxFeedGraalCxxApi-{version}{platform}-{rest}{extension}"


def write_archive(path, members):
    if path.name.endswith(".zip"):
        with zipfile.ZipFile(path, "w") as archive:
            for member in members:
                archive.writestr(member, member)
    else:
        with tarfile.open(path, "w:gz") as archive:
            for member in members:
                data = member.encode()
                info = tarfile.TarInfo(member)
                info.size = len(data)
                archive.addfile(info, io.BytesIO(data))


def component_names(components):
    return {c["name"] for c in components}


def walk(components):
    for component in components:
        yield component
        yield from walk(component.get("components", []))


class ParseArchiveTest(unittest.TestCase):
    def parse(self, file_name, version=VERSION):
        return sbom.parse_archive(pathlib.Path(file_name), version)

    def test_kinds(self):
        cases = {
            name("Release"): (sbom.LIB, "x86_64-windows", "Release"),
            name("Debug-static-mt"): (sbom.LIB_MT, "x86_64-windows", "Debug"),
            name("RelWithDebInfo", "aarch64-linux", extension=".tar.gz"): (sbom.LIB, "aarch64-linux", "RelWithDebInfo"),
            name("Samples", "x86_64-osx", extension=".tar.gz"): (sbom.SAMPLES, "x86_64-osx", ""),
            name("Tools", "aarch64-osx", extension=".tar.gz"): (sbom.TOOLS, "aarch64-osx", ""),
            name("Docs", platform=""): (sbom.DOCS, "", ""),
            name("Full-Source-Bundle"): (sbom.BUNDLE, "x86_64-windows", ""),
        }
        for file_name, (kind, platform, build_type) in cases.items():
            with self.subTest(file_name):
                archive = self.parse(file_name)
                self.assertEqual((archive.kind, archive.platform, archive.build_type), (kind, platform, build_type))

    def test_version_with_a_suffix(self):
        archive = self.parse(name("Release", "x86_64-linux", "v9.0.0-rc1", ".tar.gz"), "v9.0.0-rc1")
        self.assertEqual((archive.kind, archive.platform), (sbom.LIB, "x86_64-linux"))
        self.assertEqual(archive.base_name, "dxFeedGraalCxxApi-v9.0.0-rc1-x86_64-linux-Release")
        self.assertEqual(archive.sbom_path.name, "dxFeedGraalCxxApi-v9.0.0-rc1-x86_64-linux-Release.cdx.json")

    def test_rejected_names(self):
        for file_name, version in (
                (name("Release"), "v9.0.1"),  # another version
                (name("Release", "x86_64-freebsd"), VERSION),
                (name("Docs"), VERSION),  # the documentation has no platform
                (name("Release", platform=""), VERSION),
                (name("Release-static-mt", "x86_64-linux", extension=".tar.gz"), VERSION),
                (name("Full-Source-Bundle", "x86_64-linux"), VERSION),
                ("abi-x86_64-linux.xml", VERSION),
        ):
            with self.subTest(file_name), self.assertRaises(SbomError):
                self.parse(file_name, version)


class GenerateTest(unittest.TestCase):
    def setUp(self):
        self.dir = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.dir)
        self.artifacts = self.dir / "artifacts"
        (self.artifacts / "build-Samples").mkdir(parents=True)
        (self.artifacts / "build-Tools").mkdir()
        self.pom = self.dir / "pom.xml"
        self.pom.write_text(POM, encoding="utf-8")

    def archive(self, file_name, target=None, subdir="", info=None,
                programs=("bin/DxFeedConnect", "bin/MarketDepthModelSample")):
        path = self.artifacts / subdir / file_name
        if "-Samples." in file_name:
            write_archive(path, programs)
        else:
            path.write_bytes(file_name.encode())
        if target or info:
            path.with_name(file_name + sbom.BUILD_INFO_SUFFIX).write_text(info or build_info(target), encoding="utf-8")
        return path

    def release(self):
        self.archive(name("Release"), "amd64-windows")
        self.archive(name("Debug"), "amd64-windows")
        self.archive(name("Release-static-mt"), "amd64-windows")
        self.archive(name("Release", "x86_64-osx", extension=".tar.gz"), "x86_64-osx", info=build_info(
            "x86_64-osx", graalvm="25.0.1", java="25.0.1"))
        self.archive(name("Samples", "x86_64-linux", extension=".tar.gz"), "amd64-linux", "build-Samples")
        self.archive(name("Tools", "x86_64-linux", extension=".tar.gz"), "amd64-linux", "build-Tools")
        self.archive(name("Docs", platform=""))
        self.archive(name("Full-Source-Bundle"))

    def generate(self, version=VERSION):
        return sbom.generate(self.artifacts, version, self.pom, TIMESTAMP)

    def load(self, file_name, subdir=""):
        stem = file_name[:-len(".tar.gz")] if file_name.endswith(".tar.gz") else file_name[:-len(".zip")]
        return json.loads((self.artifacts / subdir / (stem + ".cdx.json")).read_text(encoding="utf-8"))

    def sdk(self, document):
        return next(c for c in document["components"] if c["name"] == "graal-native-sdk")

    def test_one_sbom_per_archive_next_to_it(self):
        self.release()
        written = self.generate()
        self.assertEqual(len(written), 8)
        self.assertTrue((self.artifacts / "build-Samples" /
                         "dxFeedGraalCxxApi-v9.0.0-x86_64-linux-Samples.cdx.json").is_file())
        self.assertTrue((self.artifacts / "build-Tools" / "dxFeedGraalCxxApi-v9.0.0-x86_64-linux-Tools.cdx.json").is_file())

        document = self.load(name("Release"))
        self.assertEqual((document["bomFormat"], document["specVersion"]), ("CycloneDX", "1.6"))
        self.assertEqual(document["metadata"]["timestamp"], "2026-11-18T07:00:00Z")
        root = document["metadata"]["component"]
        self.assertEqual(root["hashes"][0]["content"], sbom.sha256(self.artifacts / name("Release")))
        self.assertEqual(root["version"], VERSION)

    def test_components_of_each_kind(self):
        self.release()
        self.generate()
        lib = component_names(self.load(name("Release"))["components"])
        samples = component_names(self.load(name("Samples", "x86_64-linux", extension=".tar.gz"), "build-Samples")
                                  ["components"])
        tools = component_names(self.load(name("Tools", "x86_64-linux", extension=".tar.gz"), "build-Tools")
                                ["components"])
        docs = component_names(self.load(name("Docs", platform=""))["components"])
        bundle = component_names(self.load(name("Full-Source-Bundle"))["components"])

        self.assertTrue({"fmt", "Boost", "utfcpp", "date", "range-v3", "config", "bits", "graal-native-sdk"} <= lib)
        self.assertFalse({"GLFW", "Dear ImGui", "Portals", "Process", "Console", "doctest", "nanobench"} & lib)
        self.assertTrue({"GLFW", "Dear ImGui", "Portals", "fmt", "graal-native-sdk"} <= samples)
        self.assertFalse({"Process", "Console", "doctest"} & samples)
        self.assertTrue({"Process", "Console", "fmt", "graal-native-sdk"} <= tools)
        self.assertFalse({"GLFW", "Dear ImGui", "doctest"} & tools)
        self.assertEqual(docs, {"Doxygen Awesome"})
        self.assertTrue({"doctest", "CPM.cmake", "GLFW", "Process", "graal-native-sdk"} <= bundle)
        self.assertNotIn("Boost", bundle)

    def test_versions_and_purls(self):
        self.release()
        self.generate()
        components = {c["name"]: c for c in walk(self.load(name("Samples", "x86_64-linux", extension=".tar.gz"),
                                                           "build-Samples")["components"])}
        self.assertEqual(components["fmt"]["version"], CMAKE["FMTLIB_VERSION"])
        self.assertEqual(components["fmt"]["purl"], f"pkg:github/fmtlib/fmt@{CMAKE['FMTLIB_VERSION']}")
        self.assertEqual(components["range-v3"]["purl"], f"pkg:github/ericniebler/range-v3@{CMAKE['RANGE_VERSION']}.0")
        self.assertEqual(components["Boost"]["purl"], f"pkg:github/boostorg/boost@boost-{CMAKE['BOOST_VERSION']}")
        utf8proc = sbom.read_project(sbom.ROOT)["UTF8PROC_VERSION"]  # bundled with Portals
        self.assertEqual(components["utf8proc"]["purl"], f"pkg:github/juliastrings/utf8proc@v{utf8proc}")

    def test_portals_only_with_the_sample_that_uses_it(self):
        without = name("Samples", "x86_64-osx", extension=".tar.gz")
        self.archive(without, "x86_64-osx", "build-Samples", programs=("bin/DxFeedConnect",))
        with_exe = name("Samples")
        self.archive(with_exe, "amd64-windows", "build-Samples",
                     programs=("bin/DxFeedConnect.exe", "bin/MarketDepthModelSample.exe"))
        self.generate()
        self.assertFalse({"Portals", "utf8proc"} & component_names(walk(self.load(without, "build-Samples")
                                                                        ["components"])))
        self.assertTrue({"Portals", "utf8proc", "GLFW"} <= component_names(walk(self.load(with_exe, "build-Samples")
                                                                                ["components"])))

    def test_unreadable_samples_archive(self):
        path = self.archive(name("Samples"), "amd64-windows", "build-Samples")
        path.write_bytes(b"not a zip")
        with self.assertRaisesRegex(SbomError, "Cannot read the archive"):
            self.generate()

    def test_sdk_archives_by_platform_not_by_build_type(self):
        self.release()
        self.generate()
        release = self.sdk(self.load(name("Release")))
        debug = self.sdk(self.load(name("Debug")))
        expected = CMAKE["DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows"]
        self.assertEqual(release["hashes"][0]["content"], expected)
        self.assertEqual(debug["hashes"], release["hashes"])

        mt = self.sdk(self.load(name("Release-static-mt")))
        self.assertEqual(mt["hashes"][0]["content"], CMAKE["DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-static-crt"])
        distributions = {r["url"].rsplit("/", 1)[1]: r["hashes"][0]["content"] for r in mt["externalReferences"]
                         if r["type"] == "distribution"}
        self.assertEqual(distributions, {
            f"graal-native-sdk-{SDK}-amd64-windows-static-crt.zip":
                CMAKE["DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-static-crt"],
            f"graal-native-sdk-{SDK}-amd64-windows-static-lib.zip":
                CMAKE["DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-static-lib"],
        })

        bundle = self.sdk(self.load(name("Full-Source-Bundle")))
        self.assertEqual(bundle["hashes"][0]["content"], expected)

    def test_graalvm_and_jdk_from_the_build_info(self):
        self.release()
        self.generate()
        osx = {c["name"]: c for c in walk([self.sdk(self.load(name("Release", "x86_64-osx", extension=".tar.gz")))])}
        self.assertEqual(osx["GraalVM Community"]["version"], "25.0.1")  # not graalvm.version of the pom (99.0.0)
        self.assertEqual(osx["JDK"]["version"], "25.0.1")
        windows = {c["name"]: c for c in walk([self.sdk(self.load(name("Release")))])}
        self.assertEqual(windows["GraalVM Community"]["version"], "25.5.5.0.3")
        self.assertEqual(windows["JDK"]["purl"], "pkg:generic/jdk@25.0.5.0.3")

    def test_java_libraries_from_the_pom(self):
        self.release()
        self.generate()
        java = {c["name"]: c for c in self.sdk(self.load(name("Release")))["components"] if c["purl"].startswith(
            "pkg:maven/")}
        self.assertEqual(set(java), {"dxlink-websocket", "orcs-api", "qds", "dxfeed-api"})
        self.assertEqual(java["qds"]["purl"], "pkg:maven/com.devexperts.qd/qds@3.357")
        self.assertEqual(java["orcs-api"]["version"], "549")
        self.assertNotIn("properties", java["qds"])  # declared in the dependencies, the version from the management
        self.assertEqual(java["dxfeed-api"]["properties"], [{"name": "dxfcxx:pinned-by",
                                                             "value": "dependencyManagement"}])

    def test_no_repository_of_the_pom_in_the_output(self):
        self.release()
        for path in self.generate():
            text = path.read_text(encoding="utf-8")
            self.assertNotIn("example.invalid", text)
            self.assertNotIn("profile-only", text)
            self.assertNotIn("junit", text)

    def test_the_same_inputs_give_the_same_files(self):
        self.release()
        first = {p: p.read_bytes() for p in self.generate()}
        second = {p: p.read_bytes() for p in self.generate()}
        self.assertEqual(first, second)
        self.assertTrue(all(b.endswith(b"}\n") and b"\r\n" not in b for b in first.values()))

    def test_serial_number_changes_with_the_archive(self):
        self.archive(name("Docs", platform=""))
        self.generate()
        first = self.load(name("Docs", platform=""))["serialNumber"]
        (self.artifacts / name("Docs", platform="")).write_bytes(b"another build")
        self.generate()
        second = self.load(name("Docs", platform=""))["serialNumber"]
        self.assertRegex(first, r"^urn:uuid:[0-9a-f-]{36}$")
        self.assertNotEqual(first, second)

    def test_unique_refs_and_complete_dependency_graph(self):
        self.release()
        self.generate()
        document = self.load(name("Release-static-mt"))
        refs = [c["bom-ref"] for c in walk(document["components"])]
        self.assertEqual(len(refs), len(set(refs)))
        graph = {d["ref"] for d in document["dependencies"]}
        self.assertEqual(graph, set(refs) | {document["metadata"]["component"]["bom-ref"]})

    def test_missing_build_info(self):
        self.archive(name("Release"))
        with self.assertRaisesRegex(SbomError, re.escape(name("Release") + sbom.BUILD_INFO_SUFFIX)):
            self.generate()

    def test_build_info_of_another_platform_or_sdk(self):
        self.archive(name("Release"), "amd64-linux")
        with self.assertRaisesRegex(SbomError, "amd64-linux"):
            self.generate()
        self.archive(name("Release"), info=build_info("amd64-windows", sdk_version="3.16.0"))
        with self.assertRaisesRegex(SbomError, "3.16.0"):
            self.generate()

    def test_source_bundle_needs_the_windows_release_archive(self):
        self.archive(name("Full-Source-Bundle"))
        with self.assertRaisesRegex(SbomError, "Release library archive"):
            self.generate()

    def test_pom_of_another_version(self):
        self.archive(name("Release"), "amd64-windows")
        self.pom.write_text(POM.replace(f"<version>{SDK}</version>", "<version>0.0.1</version>", 1), encoding="utf-8")
        with self.assertRaisesRegex(SbomError, "0.0.1"):
            self.generate()

    def test_no_archives(self):
        with self.assertRaisesRegex(SbomError, "No archives"):
            self.generate()

    def test_main_reports_an_error(self):
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            code = sbom.main(["--artifacts-dir", str(self.artifacts), "--version", VERSION, "--pom", str(self.pom),
                              "--timestamp", TIMESTAMP])
        self.assertEqual(code, 1)
        self.assertIn("sbom.py: error: No archives", stderr.getvalue())

    def test_timestamp_without_a_time_zone(self):
        self.archive(name("Docs", platform=""))
        with self.assertRaisesRegex(SbomError, "time zone"):
            sbom.generate(self.artifacts, VERSION, self.pom, "2026-11-18T10:00:00")


class ProjectTest(unittest.TestCase):
    def test_versions_of_the_repository_agree(self):
        versions = sbom.read_project(sbom.ROOT)
        self.assertEqual(versions["UTF8PROC_VERSION"].count("."), 2)
        self.assertTrue(versions["DOXYGEN_AWESOME_VERSION"])

    def test_deps_json_disagrees_with_cmake(self):
        root = pathlib.Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, root)
        for file in ("CMakeLists.txt", "DEPENDENCIES.md"):
            shutil.copy(sbom.ROOT / file, root / file)
        utf8proc = pathlib.Path("third_party", f"portals-{CMAKE['PORTALS_VERSION']}", "vendor", "utf8proc", "utf8proc.h")
        (root / utf8proc).parent.mkdir(parents=True)
        shutil.copy(sbom.ROOT / utf8proc, root / utf8proc)
        deps = json.loads((sbom.ROOT / "deps.json").read_text(encoding="utf-8"))
        (root / "deps.json").write_text(json.dumps(deps), encoding="utf-8")
        sbom.read_project(root)

        deps["fmt"] = "0.0.1"
        (root / "deps.json").write_text(json.dumps(deps), encoding="utf-8")
        with self.assertRaisesRegex(SbomError, "fmt 0.0.1"):
            sbom.read_project(root)

    def test_every_dependency_of_deps_json_is_in_the_sbom_table(self):
        deps = json.loads((sbom.ROOT / "deps.json").read_text(encoding="utf-8"))
        # The SDK is a component of its own; nanobench is used by the tests only.
        expected = set(deps) - {"graal-native-sdk", "nanobench"}
        self.assertEqual({d.deps_json for d in sbom.DEPENDENCIES if d.deps_json}, expected,
                         "a dependency of deps.json needs a row in DEPENDENCIES of scripts/sbom.py")

    def test_every_platform_has_the_sha256_of_its_sdk_archives(self):
        versions = sbom.read_project(sbom.ROOT)
        for platform in sbom.SDK_PLATFORMS:
            kinds = (sbom.LIB, sbom.LIB_MT) if platform.endswith("-windows") else (sbom.LIB,)
            for kind in kinds:
                with self.subTest(platform=platform, kind=kind):
                    archive = sbom.Archive(pathlib.Path("archive.zip"), kind, platform, "Release")
                    platform_name, archives = sbom.sdk_archives(archive, versions)
                    self.assertEqual(platform_name, sbom.SDK_PLATFORMS[platform])
                    self.assertTrue(all(re.fullmatch(r"[0-9a-f]{64}", digest) for _, digest in archives))

    def test_licenses_agree_with_third_party_licenses(self):
        text = (sbom.ROOT / "THIRD_PARTY_LICENSES.md").read_text(encoding="utf-8")
        licenses = dict(re.findall(r"^\d+\. (.+?) (?:\(.*?\) )?- https://\S+\s*\n\s*SPDX-License-Identifier: (\S+)",
                                   text, re.MULTILINE))
        for dependency in sbom.DEPENDENCIES:
            with self.subTest(dependency.name):
                self.assertEqual(licenses.get(dependency.name), dependency.license)


if __name__ == "__main__":
    unittest.main()
