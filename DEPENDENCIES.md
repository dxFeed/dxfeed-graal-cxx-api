# Dependencies

## Compile-time

- [dxFeed Graal Native SDK](https://github.com/dxFeed/dxfeed-graal-native-sdk) v3.6.0
  - [Bundles](https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-sdk/) 
- \[opt] [Boost](https://github.com/boostorg/boost) v1.84.0
  - Boost.Stacktrace 1.0
- [utfcpp](https://github.com/nemtrif/utfcpp) v3.2.3
- [fmt](https://github.com/fmtlib/fmt) v12.1.0
- [GLFW](https://github.com/glfw/glfw) v3.4 (UI Samples)
- [Dear ImGui](https://github.com/ocornut/imgui) v1.92.9b (UI Samples)
- [doctest](https://github.com/doctest/doctest) v2.4.11 (Tests)
- [range-v3](https://github.com/ericniebler/range-v3) v0.12
- [date](https://github.com/HowardHinnant/date) v3.0.1
- [Process](https://github.com/ttldtor/Process) v3.0.1 (Tools)
- [Console](https://github.com/ttldtor/Console) v1.0.1 (Tools)
- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) v0.40.2
- [portals](https://github.com/ttldtor/portals) v0.1.1 (Samples)
  - [bits](https://github.com/ttldtor/bits) v1.0.0
- [config](https://github.com/ttldtor/config) v1.0.3
- [nanobench](https://github.com/martinus/nanobench) v4.3.11 (Tests::Benchmarks)


- On Windows:
    - [Visual C++ Redistributable for Visual Studio 2015](https://www.microsoft.com/en-us/download/details.aspx?id=48145)
    - ole32 \[opt] (Diagnostic backtraces)
    - dbgeng \[opt] (Diagnostic backtraces)
- On Linux \ MacOS
    - libbacktrace \[opt] (Diagnostic backtraces)
    - libdl \[opt] (Diagnostic backtraces)
    - addr2line \[opt] (Diagnostic backtraces)
## Updating a dependency

The archives that CMake downloads are verified with SHA-256 (`URL_HASH`); the hashes are next to the versions at the
top of `CMakeLists.txt`. After changing a version, update its hash as well:

- a GitHub release asset: `gh api repos/<owner>/<repo>/releases/tags/<tag> --jq '.assets[] | "\(.name) \(.digest)"'`;
- any other archive: `sha256sum` (or `certutil -hashfile <file> SHA256` on Windows) of the downloaded file;
- the Graal Native SDK: `DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION` and the hashes of all platforms; with another
  `DXFEED_GRAAL_NATIVE_SDK_VERSION`, the archive is downloaded without verification and a warning is printed.

Git dependencies are pinned by tag or commit. GitHub Actions in `.github/workflows` are pinned by commit SHA with the
version in a comment; Dependabot (`.github/dependabot.yml`) proposes their updates weekly.

## Run-time

- [dxFeed Graal Native SDK](https://github.com/dxFeed/dxfeed-graal-native-sdk) v3.6.0
- [doctest](https://github.com/doctest/doctest) v2.4.11 (Tests)
- [nanobench](https://github.com/martinus/nanobench) v4.3.11 (Tests::Benchmarks)


- On Windows: 
  - [Visual C++ Redistributable for Visual Studio 2015](https://www.microsoft.com/en-us/download/details.aspx?id=48145)
  - ole32 \[opt] (Diagnostic backtraces)
  - dbgeng \[opt] (Diagnostic backtraces)
- On Linux \ MacOS
  - libbacktrace \[opt] (Diagnostic backtraces)
  - libdl \[opt] (Diagnostic backtraces)
  - addr2line \[opt] (Diagnostic backtraces)

