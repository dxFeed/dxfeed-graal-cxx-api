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

## Header Check

```shell
cmake -DDXFCXX_BUILD_HEADER_CHECK=ON -DDXFCXX_USE_PRECOMPILED_HEADERS=OFF ..
cmake --build . --target dxfcxx_header_check
```

`DXFCXX_BUILD_HEADER_CHECK` compiles every public header on its own to check that it includes everything it uses.
Build with `DXFCXX_USE_PRECOMPILED_HEADERS=OFF`, since precompiled headers hide missing includes.
