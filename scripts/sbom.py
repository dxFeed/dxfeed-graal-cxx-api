#!/usr/bin/env python3
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

"""Writes a CycloneDX 1.6 SBOM next to every archive of a release (see HOW_TO_RELEASE.md).

  sbom.py --artifacts-dir <dir> --version vX.Y.Z --pom <pom.xml> --timestamp <ISO 8601>

The archives are found in <dir> and its subdirectories; the SBOM of <name>.zip or <name>.tar.gz is <name>.cdx.json
in the same directory. The versions of the C++ dependencies come from CMakeLists.txt (deps.json must agree with it).
The Graal Native SDK of an archive is described by <archive>.sdk-build-info.properties next to it (the
docs/graal-native-sdk-build-info.properties of the SDK archive that the build used: the GraalVM and the JDK) and by
<pom.xml>, the pom.xml of the SDK at its release tag (the Java libraries compiled into the SDK library). The full
source bundle has no build of its own: it takes the build-info of the x86_64-windows Release library archive.

The output depends only on the inputs: the same archives, files and timestamp give the same SBOMs.
"""

import argparse
import dataclasses
import datetime
import hashlib
import json
import pathlib
import re
import sys
import tarfile
import uuid
import xml.etree.ElementTree as ET
import zipfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
PROJECT = "dxFeedGraalCxxApi"
REPOSITORY = "https://github.com/dxFeed/dxfeed-graal-cxx-api"
SDK_REPOSITORY = "https://github.com/dxFeed/dxfeed-graal-native-sdk"
SDK_PURL = "pkg:github/dxfeed/dxfeed-graal-native-sdk"
SPEC_VERSION = "1.6"
BUILD_INFO_SUFFIX = ".sdk-build-info.properties"
ARCHIVE_EXTENSIONS = (".zip", ".tar.gz")

# The kinds of archives and what each one contains.
LIB, LIB_MT, SAMPLES, TOOLS, DOCS, BUNDLE = "lib", "lib-static-mt", "samples", "tools", "docs", "source-bundle"
WITH_SDK = (LIB, LIB_MT, SAMPLES, TOOLS, BUNDLE)
WITH_LIB = (LIB, LIB_MT, SAMPLES, TOOLS)  # the archives with the built library and its dependencies

ARCHIVE_NAME = re.compile(
    r"^" + PROJECT + r"-(?P<version>v\d+\.\d+\.\d+(?:-[a-z]+\d+)?)"
    r"(?:-(?P<platform>(?:x86_64|aarch64)-(?:windows|linux|osx)))?"
    r"-(?P<rest>(?:Release|Debug|RelWithDebInfo|MinSizeRel)(?:-static-mt)?|Samples|Tools|Docs|Full-Source-Bundle)"
    r"(?P<extension>\.zip|\.tar\.gz)$")

# The platforms of the archives and of the Graal Native SDK archives (CMakeLists.txt: DXFCXX_GRAAL_TARGET_PLATFORM).
SDK_PLATFORMS = {
    "x86_64-windows": "amd64-windows",
    "x86_64-linux": "amd64-linux",
    "aarch64-linux": "aarch64-linux",
    "x86_64-osx": "x86_64-osx",
    "aarch64-osx": "aarch64-osx",
}


@dataclasses.dataclass(frozen=True)
class Dependency:
    """A third-party C++ dependency: its version is set(<variable> ...) in CMakeLists.txt."""

    name: str
    variable: str
    repository: str  # owner/repo on GitHub
    tag: str  # the tag of the version: {v} is the version
    license: str  # SPDX, as in THIRD_PARTY_LICENSES.md
    kinds: tuple
    deps_json: str = ""  # the key in deps.json, if it has one
    program: str = ""  # a Samples archive has the dependency only when it has this program


# Test-only dependencies (doctest in the libraries, nanobench) are not in the archives; the source bundle carries the
# sources of third_party/, CPM.cmake, GLFW and Dear ImGui, Boost is downloaded by its build. MarketDepthModelSample
# (Portals) is built only with a toolchain that can build Portals (cmake/Portals.cmake).
DEPENDENCIES = (
    Dependency("Boost", "BOOST_VERSION", "boostorg/boost", "boost-{v}", "BSL-1.0", WITH_LIB),
    Dependency("utfcpp", "UTFCPP_VERSION", "nemtrif/utfcpp", "v{v}", "BSL-1.0", WITH_LIB + (BUNDLE,), "utfcpp"),
    Dependency("fmt", "FMTLIB_VERSION", "fmtlib/fmt", "{v}", "MIT", WITH_LIB + (BUNDLE,), "fmt"),
    Dependency("date", "DATE_VERSION", "HowardHinnant/date", "v{v}", "MIT", WITH_LIB + (BUNDLE,), "date"),
    Dependency("range-v3", "RANGE_VERSION", "ericniebler/range-v3", "{v}.0", "BSL-1.0", WITH_LIB + (BUNDLE,),
               "range-v3"),
    Dependency("config", "CONFIG_VERSION", "ttldtor/config", "v{v}", "BSL-1.0", WITH_LIB + (BUNDLE,), "config"),
    Dependency("bits", "BITS_VERSION", "ttldtor/bits", "v{v}", "BSL-1.0", WITH_LIB + (BUNDLE,), "bits"),
    Dependency("Process", "PROCESS_VERSION", "ttldtor/Process", "v{v}", "BSL-1.0", (TOOLS, BUNDLE), "Process"),
    Dependency("Console", "CONSOLE_VERSION", "ttldtor/Console", "v{v}", "BSL-1.0", (TOOLS, BUNDLE), "Console"),
    Dependency("Portals", "PORTALS_VERSION", "ttldtor/portals", "v{v}", "BSL-1.0", (SAMPLES, BUNDLE), "portals",
               "MarketDepthModelSample"),
    Dependency("GLFW", "GLFW_VERSION", "glfw/glfw", "{v}", "Zlib", (SAMPLES, BUNDLE), "glfw"),
    Dependency("Dear ImGui", "IMGUI_VERSION", "ocornut/imgui", "v{v}", "MIT", (SAMPLES, BUNDLE), "imgui"),
    Dependency("doctest", "DOCTEST_VERSION", "doctest/doctest", "v{v}", "MIT", (BUNDLE,), "doctest"),
    Dependency("CPM.cmake", "CPM_CMAKE_VERSION", "cpm-cmake/CPM.cmake", "v{v}", "MIT", (BUNDLE,)),
)

# The Java dependencies of the SDK pom.xml that are not compiled into the library.
EXCLUDED_SCOPES = ("test", "provided", "system", "import")


class SbomError(Exception):
    """A release archive or an input file that the SBOM cannot be made from."""


@dataclasses.dataclass(frozen=True)
class Archive:
    path: pathlib.Path
    kind: str
    platform: str  # empty for the documentation
    build_type: str  # empty for the archives without one

    @property
    def base_name(self):
        name = self.path.name
        return name[:-len(next(e for e in ARCHIVE_EXTENSIONS if name.endswith(e)))]

    @property
    def sbom_path(self):
        return self.path.with_name(self.base_name + ".cdx.json")


def parse_archive(path, version):
    match = ARCHIVE_NAME.match(path.name)
    if not match:
        raise SbomError(f"Not a release archive name: {path.name}")
    if match["version"] != version:
        raise SbomError(f"The archive {path.name} is not of the version {version}")
    rest, platform = match["rest"], match["platform"] or ""
    if rest == "Docs":
        kind, build_type = DOCS, ""
    elif rest == "Full-Source-Bundle":
        kind, build_type = BUNDLE, ""
    elif rest in ("Samples", "Tools"):
        kind, build_type = (SAMPLES if rest == "Samples" else TOOLS), ""
    elif rest.endswith("-static-mt"):
        kind, build_type = LIB_MT, rest[:-len("-static-mt")]
    else:
        kind, build_type = LIB, rest
    if (kind == DOCS) != (platform == ""):
        raise SbomError(f"The archive {path.name} has no platform" if platform == "" else
                        f"The documentation archive {path.name} has a platform")
    if kind == LIB_MT and not platform.endswith("-windows"):
        raise SbomError(f"The archive {path.name}: the static runtime archives are made on Windows only")
    if kind == BUNDLE and platform != "x86_64-windows":
        raise SbomError(f"The source bundle {path.name} is expected for x86_64-windows only")
    return Archive(path, kind, platform, build_type)


def find_archives(artifacts_dir, version):
    paths = sorted(p for p in artifacts_dir.rglob("*") if p.is_file() and p.name.endswith(ARCHIVE_EXTENSIONS))
    if not paths:
        raise SbomError(f"No archives in {artifacts_dir}")
    return [parse_archive(p, version) for p in paths]


def cmake_variables(text):
    """The set(NAME "value") of CMakeLists.txt with a literal value."""
    return dict(re.findall(r'^\s*set\(\s*([A-Za-z0-9_.-]+)\s+"([^"$]*)"', text, re.MULTILINE))


def read_project(root):
    """The versions of CMakeLists.txt, checked against deps.json, and the versions that are kept elsewhere."""
    cmake = cmake_variables((root / "CMakeLists.txt").read_text(encoding="utf-8"))
    deps = json.loads((root / "deps.json").read_text(encoding="utf-8"))
    pairs = [(d.deps_json, d.variable) for d in DEPENDENCIES if d.deps_json]
    pairs.append(("graal-native-sdk", "DXFEED_GRAAL_NATIVE_SDK_VERSION"))
    for key, variable in pairs:
        if variable not in cmake:
            raise SbomError(f"CMakeLists.txt does not set {variable}")
        if key in deps and deps[key] != cmake[variable]:
            raise SbomError(f"deps.json has {key} {deps[key]}, CMakeLists.txt has {variable} {cmake[variable]}")

    portals = root / "third_party" / f"portals-{cmake['PORTALS_VERSION']}" / "vendor" / "utf8proc" / "utf8proc.h"
    parts = re.findall(r"#define UTF8PROC_VERSION_(?:MAJOR|MINOR|PATCH) (\d+)", portals.read_text(encoding="utf-8"))
    if len(parts) != 3:
        raise SbomError(f"No version of utf8proc in {portals}")
    cmake["UTF8PROC_VERSION"] = ".".join(parts)

    match = re.search(r"\[Doxygen Awesome\]\([^)]*\) v(\S+?),", (root / "DEPENDENCIES.md").read_text(encoding="utf-8"))
    if not match:
        raise SbomError("No version of Doxygen Awesome in DEPENDENCIES.md")
    cmake["DOXYGEN_AWESOME_VERSION"] = match[1]
    return cmake


def read_build_info(path):
    if not path.is_file():
        raise SbomError(f"No build-info of the Graal Native SDK: {path}")
    info = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip() and not line.lstrip().startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            info[key.strip()] = value.strip()
    for key in ("sdk.version", "target", "graalvm.implementor", "graalvm.version", "graalvm.java-version"):
        if not info.get(key):
            raise SbomError(f"The build-info {path} has no {key}")
    return info


def build_info_path(archive, archives):
    if archive.kind != BUNDLE:
        return archive.path.with_name(archive.path.name + BUILD_INFO_SUFFIX)
    for other in archives:
        if other.kind == LIB and other.platform == archive.platform and other.build_type == "Release":
            return other.path.with_name(other.path.name + BUILD_INFO_SUFFIX)
    raise SbomError(f"No {archive.platform} Release library archive for the build-info of {archive.path.name}")


def read_pom(path, sdk_version):
    """The Java libraries of the SDK: the compile and runtime dependencies of the project and the versions that its
    dependencyManagement pins (the QD modules come in through the dependencies). Profiles are not read."""
    try:
        project = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as e:
        raise SbomError(f"Cannot read the SDK pom.xml {path}: {e}") from e
    ns = {"m": project.tag[1:].split("}")[0]} if project.tag.startswith("{") else {"m": ""}
    prefix = "m:" if ns["m"] else ""

    def text(element, name):
        child = element.find(prefix + name, ns)
        return child.text.strip() if child is not None and child.text else ""

    properties = {"project.version": text(project, "version"), "project.groupId": text(project, "groupId")}
    props = project.find(prefix + "properties", ns)
    for child in props if props is not None else ():
        properties[child.tag.split("}")[-1]] = (child.text or "").strip()

    def resolve(value):
        for _ in range(10):
            resolved = re.sub(r"\$\{([^}]+)\}", lambda m: properties.get(m[1], m[0]), value)
            if resolved == value:
                break
            value = resolved
        if "${" in value:
            raise SbomError(f"Unresolved property in the SDK pom.xml: {value}")
        return value

    if resolve(properties["project.version"]) != sdk_version:
        raise SbomError(f"The SDK pom.xml is of the version {properties['project.version']}, not {sdk_version}")

    def dependencies(parent):
        found = []
        for dependency in project.findall(f"{prefix}{parent}/{prefix}dependency", ns):
            if text(dependency, "scope") in EXCLUDED_SCOPES:
                continue
            found.append((resolve(text(dependency, "groupId")), resolve(text(dependency, "artifactId")),
                          resolve(text(dependency, "version")) if text(dependency, "version") else ""))
        return found

    managed = {(g, a): v for g, a, v in dependencies(f"dependencyManagement/{prefix}dependencies")}
    libraries = {}
    for group, artifact, version in dependencies("dependencies"):
        version = version or managed.get((group, artifact), "")
        if not version:
            raise SbomError(f"No version of {group}:{artifact} in the SDK pom.xml")
        libraries[(group, artifact)] = (version, "dependencies")
    for (group, artifact), version in managed.items():
        libraries.setdefault((group, artifact), (version, "dependencyManagement"))
    return [(g, a, v, source) for (g, a), (v, source) in sorted(libraries.items())]


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as file:
        for chunk in iter(lambda: file.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def hashes(value):
    return [{"alg": "SHA-256", "content": value}]


def properties(pairs):
    return [{"name": name, "value": value} for name, value in sorted(pairs) if value]


def without_empty(component):
    """The component without its empty lists (no properties, no nested components)."""
    return {key: value for key, value in component.items() if value != []}


def github_component(name, version, repository, tag, licenses, kind="library"):
    purl = f"pkg:github/{repository.lower()}@{tag}"
    return {
        "type": kind,
        "bom-ref": purl,
        "name": name,
        "version": version,
        "licenses": licenses,
        "purl": purl,
        "externalReferences": [{"type": "vcs", "url": f"https://github.com/{repository}"}],
    }


def spdx(identifier):
    return [{"license": {"id": identifier}}]


def archive_programs(path):
    """The names of the files of an archive without directories and .exe: the programs of a Samples archive."""
    try:
        if path.name.endswith(".zip"):
            with zipfile.ZipFile(path) as archive:
                names = archive.namelist()
        else:
            with tarfile.open(path, "r:gz") as archive:
                names = archive.getnames()
    except (zipfile.BadZipFile, tarfile.TarError, EOFError) as e:
        raise SbomError(f"Cannot read the archive {path.name}: {e}") from e
    return {re.sub(r"\.exe$", "", n.rstrip("/").rsplit("/", 1)[-1]) for n in names}


def cxx_components(kind, versions, programs=frozenset()):
    components = []
    for dependency in DEPENDENCIES:
        if kind not in dependency.kinds:
            continue
        if kind == SAMPLES and dependency.program and dependency.program not in programs:
            continue
        version = versions[dependency.variable].lstrip("v")
        component = github_component(dependency.name, version, dependency.repository,
                                     dependency.tag.format(v=version), spdx(dependency.license))
        if dependency.name == "Portals":
            utf8proc = versions["UTF8PROC_VERSION"]
            component["components"] = [github_component(
                "utf8proc", utf8proc, "JuliaStrings/utf8proc", f"v{utf8proc}",
                spdx("MIT") + [{"license": {"name": "Unicode data license"}}])]
        components.append(component)
    if kind == DOCS:
        version = versions["DOXYGEN_AWESOME_VERSION"]
        components.append(github_component("Doxygen Awesome", version, "jothepro/doxygen-awesome-css", f"v{version}",
                                           spdx("MIT")))
    return components


def sdk_archives(archive, versions):
    """The names and SHA-256 of the SDK archives that the build of the archive downloads (CMakeLists.txt)."""
    sdk_version = versions["DXFEED_GRAAL_NATIVE_SDK_VERSION"]
    platform = SDK_PLATFORMS.get(archive.platform)
    if platform is None:
        raise SbomError(f"No Graal Native SDK archive for the platform {archive.platform}")
    names = [platform + "-static-crt", platform + "-static-lib"] if archive.kind == LIB_MT else [platform]
    if versions.get("DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION") != sdk_version:
        raise SbomError(f"CMakeLists.txt has no SHA-256 of the Graal Native SDK {sdk_version} archives")
    result = []
    for name in names:
        digest = versions.get(f"DXFEED_GRAAL_NATIVE_SDK_SHA256_{name}")
        if not digest:
            raise SbomError(f"CMakeLists.txt has no SHA-256 of the Graal Native SDK archive {name}")
        result.append((f"graal-native-sdk-{sdk_version}-{name}.zip", digest))
    return platform, result


def sdk_component(archive, versions, build_info, java_libraries, pom_sha256):
    sdk_version = versions["DXFEED_GRAAL_NATIVE_SDK_VERSION"]
    platform, archives = sdk_archives(archive, versions)
    if build_info["sdk.version"] != sdk_version or build_info["target"] != platform:
        raise SbomError(f"The build-info of {archive.path.name} is of the SDK {build_info['sdk.version']} "
                        f"{build_info['target']}, not {sdk_version} {platform}")

    graalvm_version, java_version = build_info["graalvm.version"], build_info["graalvm.java-version"]
    jdk = without_empty({
        "type": "platform",
        "bom-ref": f"pkg:generic/jdk@{java_version}",
        "name": "JDK",
        "version": java_version,
        "purl": f"pkg:generic/jdk@{java_version}",
        "properties": properties([("dxfcxx:native-image.java-version", build_info.get("native-image.java-version"))]),
    })
    graalvm = without_empty({
        "type": "platform",
        "bom-ref": f"pkg:generic/graalvm@{graalvm_version}",
        "name": build_info["graalvm.implementor"],
        "version": graalvm_version,
        "purl": f"pkg:generic/graalvm@{graalvm_version}",
        "properties": properties([
            ("dxfcxx:native-image.vendor-version", build_info.get("native-image.vendor-version")),
            ("dxfcxx:native-image.garbage-collector", build_info.get("native-image.garbage-collector")),
        ]),
        "components": [jdk],
    })
    java = []
    for group, artifact, version, source in java_libraries:
        purl = f"pkg:maven/{group}/{artifact}@{version}"
        java.append(without_empty({
            "type": "library",
            "bom-ref": purl,
            "group": group,
            "name": artifact,
            "version": version,
            "purl": purl,
            "properties": properties([("dxfcxx:pinned-by", "dependencyManagement" if source != "dependencies"
                                       else "")]),
        }))
    tag = f"v{sdk_version}"
    return {
        "type": "library",
        "bom-ref": f"{SDK_PURL}@{tag}",
        "group": "com.dxfeed",
        "name": "graal-native-sdk",
        "version": sdk_version,
        "description": "dxFeed Graal Native SDK: the dxFeed Java API compiled by GraalVM native-image into a native "
                       "library. The Java components are the dependencies of its pom.xml and the versions that its "
                       "dependencyManagement pins, not the full transitive set.",
        "hashes": hashes(archives[0][1]),
        "licenses": spdx("MPL-2.0"),
        "purl": f"{SDK_PURL}@{tag}",
        "externalReferences": [{"type": "vcs", "url": SDK_REPOSITORY}] + [
            {"type": "distribution", "url": f"{SDK_REPOSITORY}/releases/download/{tag}/{name}", "hashes": hashes(h)}
            for name, h in archives],
        "properties": properties([
            ("dxfcxx:sdk.build-info", "docs/graal-native-sdk-build-info.properties of the SDK archive"),
            ("dxfcxx:sdk.target", build_info["target"]),
            ("dxfcxx:sdk.variant", build_info.get("variant")),
            ("dxfcxx:sdk.pom.sha256", pom_sha256),
        ]),
        "components": [graalvm] + java,
    }


def refs(components):
    return sorted(c["bom-ref"] for c in components)


def dependency_graph(root_ref, components):
    graph = [{"ref": root_ref, "dependsOn": refs(components)}]
    pending = list(components)
    while pending:
        component = pending.pop()
        nested = component.get("components", [])
        graph.append({"ref": component["bom-ref"], "dependsOn": refs(nested)})
        pending.extend(nested)
    return sorted(graph, key=lambda d: d["ref"])


def make_sbom(archive, version, versions, timestamp, build_info=None, java_libraries=(), pom_sha256=""):
    archive_sha256 = sha256(archive.path)
    programs = archive_programs(archive.path) if archive.kind == SAMPLES else frozenset()
    components = cxx_components(archive.kind, versions, programs)
    if archive.kind in WITH_SDK:
        components.append(sdk_component(archive, versions, build_info, java_libraries, pom_sha256))
    components.sort(key=lambda c: c["bom-ref"])

    root_ref = f"pkg:github/dxfeed/dxfeed-graal-cxx-api@{version}"
    serial = uuid.uuid5(uuid.NAMESPACE_URL, f"{REPOSITORY}/releases/{version}/{archive.path.name}#{archive_sha256}")
    return {
        "$schema": "http://cyclonedx.org/schema/bom-1.6.schema.json",
        "bomFormat": "CycloneDX",
        "specVersion": SPEC_VERSION,
        "serialNumber": f"urn:uuid:{serial}",
        "version": 1,
        "metadata": {
            "timestamp": timestamp,
            "tools": {"components": [{
                "type": "application",
                "name": "scripts/sbom.py",
                "version": version,
                "externalReferences": [{"type": "vcs", "url": REPOSITORY}],
            }]},
            "component": {
                "type": "application" if archive.kind in (SAMPLES, TOOLS) else "library",
                "bom-ref": root_ref,
                "name": PROJECT,
                "version": version,
                "description": f"The release archive {archive.path.name}",
                "hashes": hashes(archive_sha256),
                "licenses": spdx("MPL-2.0"),
                "purl": root_ref,
                "externalReferences": [
                    {"type": "vcs", "url": REPOSITORY},
                    {"type": "distribution", "url": f"{REPOSITORY}/releases/download/{version}/{archive.path.name}",
                     "hashes": hashes(archive_sha256)},
                ],
                "properties": properties([
                    ("dxfcxx:archive.kind", archive.kind),
                    ("dxfcxx:archive.platform", archive.platform),
                    ("dxfcxx:archive.build-type", archive.build_type),
                ]),
            },
        },
        "components": components,
        "dependencies": dependency_graph(root_ref, components),
    }


def normalize_timestamp(value):
    try:
        moment = datetime.datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as e:
        raise SbomError(f"Not an ISO 8601 timestamp: {value}") from e
    if moment.tzinfo is None:
        raise SbomError(f"The timestamp has no time zone: {value}")
    return moment.astimezone(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def generate(artifacts_dir, version, pom, timestamp, root=ROOT):
    """Writes the SBOMs and returns their paths."""
    timestamp = normalize_timestamp(timestamp)
    versions = read_project(root)
    archives = find_archives(artifacts_dir, version)
    java_libraries, pom_sha256 = [], ""
    if any(a.kind in WITH_SDK for a in archives):
        java_libraries = read_pom(pom, versions["DXFEED_GRAAL_NATIVE_SDK_VERSION"])
        pom_sha256 = sha256(pom)

    written = []
    for archive in archives:
        build_info = read_build_info(build_info_path(archive, archives)) if archive.kind in WITH_SDK else None
        sbom = make_sbom(archive, version, versions, timestamp, build_info, java_libraries, pom_sha256)
        archive.sbom_path.write_text(json.dumps(sbom, indent=2, ensure_ascii=False) + "\n", encoding="utf-8",
                                     newline="\n")
        written.append(archive.sbom_path)
    if len(set(written)) != len(archives):
        raise SbomError("Two archives have the same SBOM name")
    return written


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--artifacts-dir", type=pathlib.Path, required=True, help="the archives of the release")
    parser.add_argument("--version", required=True, help="the release tag, vX.Y.Z[-suffixN]")
    parser.add_argument("--pom", type=pathlib.Path, required=True, help="pom.xml of the SDK at its release tag")
    parser.add_argument("--timestamp", required=True, help="ISO 8601, e.g. the date of the tag commit")
    parser.add_argument("--root", type=pathlib.Path, default=ROOT, help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    try:
        for path in generate(args.artifacts_dir, args.version, args.pom, args.timestamp, args.root):
            print(path)
    except (SbomError, OSError) as e:
        print(f"sbom.py: error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
