# Getting started with the Portals runnable bundle

Extract the whole archive into a directory of your choice. Paths with spaces
and Unicode are supported. Keep the directory layout intact.

## Run the examples

On Windows, open PowerShell in the extracted directory in a UTF-8/VT-capable
terminal (for example Windows Terminal). Run an interactive example:

    ./run.ps1 dashboard
    ./run.ps1 gallery
    ./run.ps1 custom_widget
    ./run.ps1 replay

The scripts locate bin relative to their own location, so you can also call
them by absolute path from another working directory. They run in your existing
terminal. Dashboard supports --ansi16 --ascii for conservative terminal
capabilities. Each example displays its keyboard help; use F10 or Ctrl+C to exit.
No installation or network connection is needed to run the four executables.

On Linux, use a VT-capable terminal:

    ./run.sh dashboard
    ./run.sh gallery
    ./run.sh custom_widget
    ./run.sh replay

The Linux binaries require the dynamic libraries and libc listed in
manifest.json. They are built for the recorded Linux x64 toolchain; this archive
does not promise compatibility with every Linux distribution.

For finite checks that do not open a terminal session:

    ./run.ps1 dashboard --check --ansi16 --ascii
    ./run.ps1 gallery --check
    ./run.ps1 custom_widget --check
    ./run.ps1 replay --check --delay-ms 250

Use run.sh with the same arguments on Linux. These checks exercise deterministic
example behavior. They do not replace interactive terminal acceptance.

## Read the documentation and inspect data

Open docs/html/index.html in a browser. All generated HTML, CSS, JavaScript,
images and linked notices are included. Start with the getting-started,
architecture, examples and limitations pages.

The two themes in themes and the fixtures in data are reference material.
The runnable examples compile their default styles and replay prices into the
executable; editing these files does not change the running demonstration.
See data/data-README.md and examples/source for the corresponding source.

## Build an application against the installed SDK

The install directory contains the relocatable C++23 headers, static archives,
CMake package files and license notices. Copy or move the entire install
directory when using another SDK location.

Use the compiler, architecture, Release configuration and runtime recorded in
manifest.json. The default Windows bundle SDK uses the static MSVC CRT (/MT);
its consumer must explicitly use CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded.
The general Portals project still preserves the caller's runtime choice.

From an x64 MSVC developer PowerShell:

    cmake -S examples/external-consumer -B consumer-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<absolute bundle path>/install" -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
    cmake --build consumer-build
    ctest --test-dir consumer-build --output-on-failure --no-tests=error

If manifest.json records MultiThreadedDLL, replace the runtime selection above
with MultiThreadedDLL (/MD) and provide its matching Microsoft runtime.
On Linux omit CMAKE_MSVC_RUNTIME_LIBRARY and use the compatible C++ compiler.
The supplied consumer links portals::core from find_package(Portals CONFIG
REQUIRED); it needs no Portals source tree or package registry.

The sources in examples/source demonstrate the library APIs. The independently
buildable project is examples/external-consumer.

## Verify a moved bundle

CMake 3.24+, Ninja and a compatible C++23 compiler are needed for the complete
verification. Run the copied verification script from any working directory,
using a new absolute work path outside the extracted bundle:

    cmake "-DBUNDLE_DIR=<absolute bundle path>" "-DTEST_WORK_DIR=<new absolute temporary path>" "-DCXX_COMPILER=<compatible compiler>" -P "<absolute bundle path>/verify/bundle_smoke.cmake"

It checks every payload SHA256, runs all four --check scenarios, moves the SDK
again, and builds/runs the external consumer using only that installed prefix.
A Windows consumer receives the manifest's exact MSVC runtime selection.
Set PYTHON_EXECUTABLE to a known Python 3 interpreter to additionally check every
local HTML link and anchor. VERIFY_ONLY=ON checks hashes without a compiler.

manifest.json records the source commit, source archive SHA256, compiler,
OS/runtime tuple, actual executable imports and checksums. It does not hash
itself and is not a signed authenticity guarantee. Keep the separately reported
archive SHA256 when comparing downloaded or copied bytes.

See docs/limitations.md for API and terminal constraints. A successful finite
bundle check does not declare the complete MVP accepted or independently reviewed.

References: https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html
and https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html.
