# Installed CMake package

Portals installs its C++23 public headers, static core archive, private utf8proc
archive, CMake configuration/version/targets files, and license notices. No
network download or system utf8proc installation is required. The imported
`portals::core` target carries the C++23 requirement and the complete static link
chain; on UNIX the package configuration finds the platform Threads dependency.

The top-level `project(Portals VERSION ...)` declaration is the sole source of
the current library version. CMake uses it for the installed package metadata,
and smoke and installed-consumer checks compare the linked library version with
that metadata.

Build and install from an x64 MSVC developer PowerShell, or use the same CMake
commands with an appropriate C/C++ toolchain on Linux:

```powershell
cmake -S . -B build/package -G Ninja -DCMAKE_BUILD_TYPE=Release -DPORTALS_BUILD_TESTS=OFF -DPORTALS_BUILD_EXAMPLES=OFF -DPORTALS_BUILD_TOOLS=OFF
cmake --build build/package
cmake --install build/package --prefix "C:/sdk/Portals"
```

An external project uses only the installed prefix:

```cmake
cmake_minimum_required(VERSION 3.24)
project(MyApplication LANGUAGES CXX)
find_package(Portals 0.2 CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE portals::core)
```

```powershell
cmake -S my-application -B build/my-application -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/sdk/Portals"
cmake --build build/my-application
```

Move the entire installation directory, including headers, both archives,
CMake metadata and notices, and pass its new location in `CMAKE_PREFIX_PATH`.
The installed targets resolve paths relative to that location. Keep install
directories such as `CMAKE_INSTALL_LIBDIR` and `CMAKE_INSTALL_INCLUDEDIR` relative
when selecting a custom layout with GNUInstallDirs.

## Toolchain and configuration compatibility

The package is a static-library/header distribution. Use a compatible target
architecture, compiler, standard library, C++ configuration and runtime for
Portals, utf8proc and the application. Build/install Debug and Release separately,
or install both configurations into one prefix with a compatible multi-config
build. Archives are stored separately under `lib/Debug` and `lib/Release`
(or the corresponding configured `CMAKE_INSTALL_LIBDIR`), so installing one
configuration preserves the other. CMake's `SameMajorVersion` package-version selection is a discovery rule;
it does not promise a stable ABI or binary portability. This package supplies no
portable module BMI files.

Portals preserves the caller's MSVC runtime selection. CMake's default
`MultiThreaded$<$<CONFIG:Debug>:Debug>DLL` means `/MDd` for Debug and `/MD` for
Release unless the caller explicitly selects another runtime. Match that choice
when building the consumer; never infer `/MT` or absence of Visual C++ runtime
redistributables from the word Release. Rebuild the archives when changing the
runtime or other incompatible parts of the build tuple.

## Relocation test

With `PORTALS_BUILD_TESTS=ON`, build `portals_core` and run:

```powershell
cmake --build build/windows-msvc-debug --target portals_core
ctest --test-dir build/windows-msvc-debug -R "^portals.package-install$" --output-on-failure --no-tests=error
```

The test installs the current build into a fresh directory, moves that directory
to a path containing spaces and Unicode, and copies a small independent consumer
into its own source/build directories. The consumer links imported targets only,
publishes a new immutable model revision, prepares a custom widget and verifies
its rendered total. It does not open a terminal. Windows test processes use the
existing noninteractive startup source copied beside the consumer fixture.
The test rejects source/build paths and private warnings/definitions in installed
CMake files, disables package registries, and retains its directories for inspection.

The same installed consumer also runs the [managed-control before/after forms](managed-controls.md),
including deterministic input replay, exact output comparison and typed callback errors.

Multi-config generators that provide Debug and Release also register
`portals.package-install-multi-config`. This test builds both core/utf8proc
configurations, installs them into one prefix, moves it, then builds and runs
the external consumer in both configurations:

```powershell
cmake -S . -B build/package-multi -G "Ninja Multi-Config" -DPORTALS_BUILD_TESTS=ON -DPORTALS_BUILD_EXAMPLES=OFF -DPORTALS_BUILD_TOOLS=OFF
ctest --test-dir build/package-multi -C Debug -R "^portals.package-install-multi-config$" --output-on-failure --no-tests=error
```

The same test is available with a Visual Studio multi-config generator.

References: [CMake importing/exporting](https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html),
[GNUInstallDirs](https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html),
[package configuration helpers](https://cmake.org/cmake/help/latest/module/CMakePackageConfigHelpers.html),
[MSVC runtime selection](https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html).

## Runnable Release bundles

The packaging pipeline builds one committed Git revision in a fresh source
archive. Working-tree edits and ignored/private files are excluded. It builds
the four examples, complete Doxygen HTML and the static SDK from that same
Release build, then tests a freshly unpacked archive before publishing it.
It never removes or reuses a staging directory and never overwrites an archive.
Choose new absolute paths; the staging path must be outside the source checkout.

The complete reference requires Doxygen 1.18 or newer and Mermaid CLI on PATH;
1.18 is the verified Doxygen minimum. Node.js 24 LTS and Mermaid CLI 12 are the
verified diagram toolchain. See [documentation setup](getting-started.md) for
installation. Diagrams are packaged as SVG, so readers need no generator or
network connection. Ordinary library builds do not require these tools. A locally unpacked
[official Doxygen release](https://www.doxygen.nl/download.html) can be placed on
PATH for the packaging process without a system-wide installation.

From an x64 MSVC developer PowerShell, with Git, CMake 3.24+, Ninja and Doxygen
1.18 or newer, Node.js and `mmdc` on PATH:

    ./tools/package/bundle.ps1 -WorkDir "C:/temp/portals bundle run-001" -OutputArchive "C:/artifacts/portals-mvp-windows-x64-release.zip"

Use -Revision to select a committed revision, -Parallel to set build parallelism,
and -PythonExecutable with a known Python 3 interpreter to enable the existing
HTML local-link/anchor checker. Without Python that optional gate is recorded as
not_run. Windows bundle builds explicitly select MultiThreaded (/MT) by default;
this changes neither the project's general CRT default nor another build tree.
-Runtime MultiThreadedDLL selects /MD deliberately and is recorded in the
manifest. dumpbin /DEPENDENTS inspects each actual example; a dynamic CRT import
is rejected when /MT was requested. System Windows DLLs are not redistributed.

On Linux, run the same pipeline directly:

    cmake "-DSOURCE_DIR=/absolute/portals" "-DWORK_DIR=/tmp/portals-bundle-run-001" "-DOUTPUT_ARCHIVE=/absolute/artifacts/portals-mvp-linux-x64-release.tar.gz" "-DCXX_COMPILER=/usr/bin/g++" -P tools/package/bundle.cmake

The Linux archive records ldd output for each executable and the host's ldd/libc
version. It supplies no universal-distribution binary compatibility promise.
The pipeline supports native x64 Windows/MSVC and Linux toolchains, using
single-config Ninja by default; no cross compilation is performed.

Each archive contains bin, themes, data, docs/html, install, example source,
an independent installed-package consumer, launchers, GETTING-STARTED.md,
license notices, verification scripts and manifest.json. Themes and replay
fixtures are useful source/reference inputs; the example binaries compile their
defaults in and need no current-directory data files. The complete HTML tree is
copied, including local resources and linked notices.

The manifest records the source commit/archive checksum, OS/compiler/runtime
tuple, executable imports and SHA256 of every payload file. manifest.json is
excluded from its own inventory; the separately reported archive SHA256 covers
the full archive. Checksums detect accidental corruption, not authenticity.
The relocated consumer receives the manifest's runtime selection and uses only
the installed prefix, with package registries disabled.

To check an existing trusted archive from a new path:

    cmake "-DARCHIVE=<absolute archive path>" "-DTEST_WORK_DIR=<new absolute temporary path>" "-DCXX_COMPILER=<compatible compiler>" -P tests/bundle_smoke.cmake

The test unpacks into a path with spaces/Unicode, checks inventory and hashes,
runs dashboard/gallery/custom-widget/replay with finite --check arguments from
a foreign working directory, relocates the SDK again and builds/runs the external
consumer. Its evidence directories are retained. This proves the finite bundle
scenario; interactive terminal acceptance and the full MVP gates are separate.
The script can also verify a moved extracted bundle using BUNDLE_DIR and the
copied verify/bundle_smoke.cmake. VERIFY_ONLY=ON checks hashes without building.

With tests enabled, portals.bundle-scripts covers payload corruption, unlisted
files, manifest path traversal and refusal to modify existing staging/output
paths. It runs without a C++ build or terminal.

References: [CMake archive commands](https://cmake.org/cmake/help/latest/manual/cmake.1.html#command-line-tool),
[MSVC runtime selection](https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html),
[dumpbin DEPENDENTS](https://learn.microsoft.com/en-us/cpp/build/reference/dependents?view=msvc-170).
