# Getting started {#getting-started}

Portals provides a C++23 static library and four runnable terminal examples.
Start with the gallery, then use the dashboard or custom widget to explore the
same [Application runner](application.md). The [architecture](architecture.md)
explains ownership and data flow; [limitations](limitations.md) defines the
boundaries of this release.

The [documentation guide](index.md) gives the reading order and separates
application guides from core contracts and contributor material.

## Build from source

Requirements are CMake 3.24+, Ninja, a C99 compiler for the private vendored
utf8proc dependency, and a C++23 compiler **and standard library** with
`<expected>` and `<format>`. No dependency download is needed. Use an x64
Developer PowerShell for Visual Studio with the C++ desktop workload on Windows:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

On Linux with GCC and a compatible standard library. The preset uses the
`g++` on PATH; select an installed supported compiler explicitly when that
default is older, for example `cmake --preset linux-gcc-debug
-DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14` for GCC 14:

```sh
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

Presets build Debug under `build/<preset>`. For a separate Release build:

```powershell
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
ctest --test-dir build/release --output-on-failure --no-tests=error
```

The same Release commands work in a Linux shell. Keep compiler, architecture,
standard library and runtime compatible with consumers; see [package](package.md).
`PORTALS_BUILD_EXAMPLES`, `PORTALS_BUILD_TESTS` and `PORTALS_BUILD_TOOLS`
default to ON for top-level builds and OFF for embedded builds.

## Try the examples

In a terminal with native input/output attached, run:

```powershell
./build/windows-msvc-debug/examples/gallery/portals_gallery
./build/windows-msvc-debug/examples/dashboard/portals_dashboard --rate 10000 --fps 30 --rows 1000000
./build/windows-msvc-debug/examples/custom_widget/portals_custom_widget
./build/windows-msvc-debug/examples/replay/portals_replay_example
```

On Linux substitute `build/linux-gcc-debug`. F10 or Ctrl+C exits. Gallery:
Tab changes focus, Enter commits/activates, F2 changes theme. Dashboard: arrows,
Home and End navigate the table, Enter commits the note, F2 changes theme.
Custom widget: Left/Right adjusts the meter. [Examples](examples.md) describes
synthetic/replay sources, delay, ASCII and ANSI16 options.

Each executable also accepts `--check`. These finite checks use an in-memory
terminal and logical clock, exit automatically, and print results after close.
They verify runner behavior without requiring a native terminal; they do not
establish physical display compatibility or native restoration.

## Your own main

Start with [declarative composition](declarative-ui.md) for an ordinary form.
The gallery's `Form` owns a `ManagedScene`, which retains its state, standard
bindings and presentation adapters. The `Application` must outlive that form.
The interactive path in `examples/gallery/main.cpp` is:

```cpp
auto app = example::take(portals::Application::create());
gallery::Form form(*app);

return example::report(form.run());
```

This excerpt uses example helpers, not additional public APIs:
`example::take` throws on a failed `expected`; main catches construction
failures. `example::report` handles the run result after restoration. See
`examples/gallery/form.hpp` for the complete form description. Its `onInput`
adapter handles global commands; the scene manages standard presentation and
bindings. [Managed controls](managed-controls.md) explains custom adapters.

For explicit low-level ownership, [Application](application.md) shows a small
button program with an `ApplicationScene` and a retained scoped binding.
Derive `ApplicationScene` when managing commands and preparation yourself. In
`prepare`, synchronize bindings, pin each model once, prepare its sibling
views at their final extents, then handle returned provider requests outside
rendering. A custom widget implements `measure/arrange/paint`; prepare its
owning presentation before paint. `examples/custom_widget/meter.hpp` shows
an independent immutable model and a managed preparation adapter.

## Diagnose an action

```text
portals_dashboard --rate 0 --trace trace.json
```

Tab to Inspect and press Enter. Inspect assigns the note its existing value.
After exit, the metadata export can distinguish `handler_returned`,
`value_unchanged` and `frame_not_requested`. Export occurs after restoration;
file failures are reported separately. See [diagnostics](diagnostics.md) for
capacity/loss reporting and why an absent record proves nothing. Diagnostic
metadata differs from the versioned semantic replay format in [testing](testing.md).

## Installed package and HTML help

Use `add_subdirectory` and `portals::core` when embedding the source, or
`find_package(Portals CONFIG REQUIRED)` for an installed package.
[Package](package.md) supplies install, relocation and external consumer commands.

For HTML documentation, install Doxygen 1.18 or newer and Mermaid CLI. The
verified diagram toolchain is Node.js 24 LTS with Mermaid CLI 12. In PowerShell:

```powershell
npm.cmd install --global @mermaid-js/mermaid-cli
mmdc.cmd --version
```

Mermaid CLI uses a headless browser supplied by Puppeteer. Its first installation
normally downloads that browser; subsequent documentation builds render diagrams
locally. Ordinary library builds do not need Node.js, Mermaid or Doxygen.

With `doxygen` and `mmdc` available on PATH:

```powershell
cmake -S . -B build/docs -G Ninja -DPORTALS_BUILD_DOCS=ON
cmake --build build/docs --target portals_docs
```

If Mermaid is installed outside PATH, pass its executable explicitly with
`-DPORTALS_MERMAID_EXECUTABLE=C:/path/to/mmdc.cmd` (or the `mmdc` executable on
POSIX). CMake uses its directory for Doxygen's native Mermaid integration.

Open `build/docs/docs/html/index.html`. Start with the reading guide; use the
sidebar's API reference when you need a specific type or function. The HTML
directory includes example source listings and license notices for offline
reading. Mermaid blocks are rendered to SVG at build time; reading diagrams
requires neither internet access nor an installed generator. Generated help
needs no private work files.

When changing documentation, check its local file and anchor links with
`python tools/docs/check_links.py build/docs/docs/html` (Python 3 is optional
for ordinary builds). This page covers source builds and installation;
the [package guide](package.md) describes distribution and consumer checks.

References: [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html),
[CMake build tool mode](https://cmake.org/cmake/help/latest/manual/cmake.1.html),
[Doxygen configuration](https://www.doxygen.nl/manual/config.html).
