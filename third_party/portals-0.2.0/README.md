# Portals

[![Build and test](https://github.com/ttldtor/portals/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ttldtor/portals/actions/workflows/ci.yml)
![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=c%2B%2B)
[![License: BSL 1.0](https://img.shields.io/badge/License-BSL--1.0-blue.svg)](LICENSE.txt)

Portals is a **C++23 retained terminal UI library**, currently under development.
It provides a static core for applications with retained widgets, layouts,
managed controls, virtual lists and tables, and charts. The API and ABI are
evolving; see the [current limitations](docs/limitations.md) before depending
on platform or lifecycle guarantees.

## Quick start

Requirements and compiler details are in [Getting started](docs/getting-started.md).
Build the examples with the supplied preset, then launch the gallery in a terminal.

**Windows — x64 Developer PowerShell for Visual Studio:**

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
./build/windows-msvc-debug/examples/gallery/portals_gallery
```

**Linux — GCC, CMake and Ninja:**

```sh
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
./build/linux-gcc-debug/examples/gallery/portals_gallery
```

## A first look

This function mounts a small UI into an existing `ManagedScene`. The caller must
check the result before calling `scene.run()`. See the [gallery form source](examples/gallery/form.hpp)
and [Getting started](docs/getting-started.md) for a complete setup.

```cpp
#include <portals/ui.hpp>

using namespace portals;

std::expected<Node<Column>, ManagedError> mountGreeting(Application &app, ManagedScene &scene) {
  return ui::mount(scene, ui::column(
    ui::label("Hello, Portals!"),
    ui::button("Quit", [&app]() -> std::expected<void, TreeError> {
      app.requestStop();

      return {};
    })));
}
```

## Documentation

- [Documentation guide](docs/index.md) — reading order and topic map.
- [Getting started](docs/getting-started.md) — build, run examples, and generate local HTML help.
- [Runnable examples](docs/examples.md) — gallery, dashboard, custom widgets, and options.
- [Architecture](docs/architecture.md) — ownership and event flow.
- [CMake package](docs/package.md) — embed from source or install for consumers.
- [Limitations](docs/limitations.md) — supported platforms and runtime boundaries.
- [Development guide](docs/development.md) — testing, design material, and documentation upkeep.
- [Release notes](ReleaseNotes.md) — implemented changes and release status.

## License and notices

Portals is licensed under the [Boost Software License 1.0](LICENSE.txt).
Notices for vendored utf8proc, Unicode test data, and adapted theme colors are
listed in [dependency provenance](vendor/utf8proc/README.md),
[Unicode test data](tests/fixtures/unicode/Unicode-LICENSE.txt), and
[Textual palette attribution](vendor/textual/README.md).
