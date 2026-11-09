# How To Build

## Prerequisites

- CMake
- clang\gcc\Visual Studio

## Build

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release 

```

or use the `scripts/build.sh` or `scripts/build.cmd`. Make sure that you use `command prompt for VS` on Win.

## Full Source Bundles (Windows x64, offline)

A release has two source bundles for Windows x64. Each has the sources and all the dependencies, the Graal Native SDK
too, so it builds without network access. They differ in the C runtime of the SDK:

| Bundle                                                                        | C runtime        | Graal Native SDK in `third_party`                                                                    |
|-------------------------------------------------------------------------------|------------------|------------------------------------------------------------------------------------------------------|
| `dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle.zip`           | dynamic (`/MD`)  | `amd64-windows`, `amd64-windows-debug`                                                               |
| `dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle-static-mt.zip` | static (`/MT`)   | `amd64-windows-static-crt`, `amd64-windows-debug-static-crt`, `amd64-windows-static-lib`, `amd64-windows-debug-static-lib` |

The SDK of a bundle is in `third_party/graal-native-sdk-<SDK version>-<variant>`, and CMake takes the variant that the
options select, as it does when it downloads the SDK:

- `DXFCXX_LINK_STATIC_RUNTIME` - the static C runtime: the SDK DLL with the static C runtime (`-static-crt`) and the
  static library of the SDK (`-static-lib`). It is `ON` by default with the static-mt bundle and `OFF` otherwise.
- `DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK=ON` - the Debug SDK (`-debug`). It does not depend on the build type.

An option that needs the SDK of the other bundle fails the configuration, and the message names that bundle. Without
the SDK in `third_party` (a git checkout), CMake downloads it.

In the x64 Native Tools Command Prompt for VS 2022:

```shell
tar -xf dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle.zip
cd dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle
cmake -B build -A x64 -DFETCHCONTENT_FULLY_DISCONNECTED=ON -DDXFCXX_BUILD_DOC=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix install
```

The static-mt bundle builds the libraries the same way; this installs the content of a `-static-mt` archive (see
"Windows without the Visual C++ Redistributable" in `README.md`):

```shell
tar -xf dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle-static-mt.zip
cd dxFeedGraalCxxApi-<version>-x86_64-windows-Full-Source-Bundle-static-mt
cmake -B build -A x64 -DFETCHCONTENT_FULLY_DISCONNECTED=ON -DDXFCXX_BUILD_DOC=OFF -DDXFCXX_BUILD_UNIT_TESTS=OFF ^
    -DDXFCXX_BUILD_SAMPLES=OFF -DDXFCXX_BUILD_TOOLS=OFF -DDXFCXX_INSTALL_SAMPLES=OFF -DDXFCXX_INSTALL_TOOLS=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix install
```

Add `-DDXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK=ON` and build with `--config Debug` for the Debug SDK and C runtime.

With the static C runtime, `dxFeedGraalCxxApi.dll` is built but not installed: the DLL has its own C runtime, and the
C++ API passes standard library objects (`std::string`, `std::shared_ptr`, etc.) between the DLL and the code that uses
it. Link `dxFeedGraalCxxApi_static.lib` instead. Only the C API of the DLL (`include/dxfeed_graal_c_api/api.h`) can be
used across this boundary; take the DLL and its import library from the build directory if you need it.

## Run Tests

```shell
cd build
ctest -C Release --extra-verbose --parallel 4
```

The unit tests do not need external network: they use a `LOCAL_HUB` endpoint or a loopback TCP server. The smoke tests
against `demo.dxfeed.com` are built only with `-DDXFCXX_ENABLE_NETWORK_TESTS=ON` and have the CTest label `network`:

```shell
ctest -C Release -L network
```

With `-DDXFCXX_DYNAMICALLY_LINK_UNIT_TESTS=ON` the tests are linked with the shared library instead of the static one,
as an application that uses the DLL. This finds what works only with the static library: a function that is not
exported (a link error) and state that the public headers keep outside the library (on Windows, the DLL and each
executable have their own copies of it). The rules for the public headers are in `include/dxfeed_graal_cpp_api/internal/Conf.hpp`
next to `DXFCPP_EXPORT`.

## UI Samples

`DXFCXX_BUILD_UI_SAMPLES` controls the UI samples (Dear ImGui, GLFW, OpenGL):

- `AUTO` (default) - build them when their dependencies are found: always on Windows and macOS; on Linux, the OpenGL
  and the X11 or Wayland development packages. Otherwise the configuration prints one line with the missing packages
  and goes on without the UI samples.
- `ON` - build them; the configuration fails when the dependencies are missing.
- `OFF` - do not build them.

On Linux, GLFW is built with every window system backend that is found, X11, Wayland or both, and selects one at run
time: the one of `XDG_SESSION_TYPE` (`x11` or `wayland`), otherwise the first that connects. With
`-DDXFCXX_SAMPLE_GLFW_BUILD_WAYLAND=ON`, the configuration fails when the Wayland backend cannot be built.

On Debian/Ubuntu, install OpenGL and **one** of the backends (or both, then GLFW gets both):

```shell
# Wayland
sudo apt-get install libgl1-mesa-dev libwayland-dev libxkbcommon-dev pkg-config
```

```shell
# X11
sudo apt-get install libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev
```

`libgl1-mesa-dev` brings the core X11 headers (`libx11-dev`) with it, but the X11 backend also needs the extension
headers, so the Wayland set builds only the Wayland backend.

## Sanitizers

```shell
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDXFCXX_ENABLE_ASAN=ON -DDXFCXX_ENABLE_UBSAN=ON ..
```

- `DXFCXX_ENABLE_ASAN` - AddressSanitizer (GCC, Clang, AppleClang, MSVC, MinGW Clang; MinGW GCC has no sanitizer
  runtime, the options are ignored there with a warning).
- `DXFCXX_ENABLE_UBSAN` - UndefinedBehaviorSanitizer, can be combined with ASan (not supported by MSVC).
- `DXFCXX_ENABLE_TSAN` - ThreadSanitizer, cannot be combined with ASan (not supported by MSVC). The unit tests use the
  suppressions from `tests/sanitizers/tsan.supp`.

Both library targets and all their consumers (tests, samples, tools) are instrumented. With MSVC, the ASan runtime DLL
is copied next to the unit tests, so they can be run outside the Visual Studio developer prompt.

Sanitized builds also add the `dxFeedGraalCxxApi_SanitizerCanary_<asan|ubsan|tsan>` tests: they call a deliberately
defective library function and pass only if the sanitizer reports it, so a sanitizer that is silently not applied to the
library makes them fail.

## Warnings

This project's own code (the libraries, tests, samples, tools, fuzzers) is compiled with `-Wshadow`,
`-Wsign-conversion`, `-Wfloat-equal` and `-Wextra-semi` (MSVC: C4365) on top of `-Wall -Wextra -pedantic` (`/W4`).
These warnings are not passed to the projects that use the library, and the headers of the dependencies are included
as system headers, so their warnings are not reported. With `-DDXFCXX_WARNINGS_AS_ERRORS=ON` (the CI builds) the
warnings of this project's code are errors (`-Werror`, `/WX`).

```shell
cmake -DDXFCXX_WARNINGS_AS_ERRORS=ON ..
```

## Header Check

```shell
cmake -DDXFCXX_BUILD_HEADER_CHECK=ON -DDXFCXX_USE_PRECOMPILED_HEADERS=OFF ..
cmake --build . --target dxfcxx_header_check
```

`DXFCXX_BUILD_HEADER_CHECK` compiles every public header on its own to check that it includes everything it uses.
Build with `DXFCXX_USE_PRECOMPILED_HEADERS=OFF`, since precompiled headers hide missing includes.
