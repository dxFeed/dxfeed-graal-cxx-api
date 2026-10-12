# Standalone header / module / shared-library spike

This independent C++23 toy tests an ABI-preserving named-module facade over a
header-defined C++ API. It implements no UI, is not linked into Portals core,
and promises no stable ABI or production module support. Copy this directory
alone to build it; no private plans, scripts or root build are inputs.

`include/toy/api.hpp` is the single source of declarations, value type and inline
template. `src/api.cpp` is the only out-of-line implementation in `toy_shared`
(DLL/.so). `src/toy.cppm` includes the header in its global module fragment and
re-exports its names with `using`; it belongs to the separate static
`toy_module` facade. That facade links to the shared library and sees the same
`dllimport` declarations as Windows consumers. Only `api.cpp` gets `dllexport`.
C++ `export` alone does not export a DLL symbol. ELF symbols use explicit default
visibility over a hidden target. BMI files are built locally per compiler/build;
none are shipped or reused across toolchains.

Three default runtime tests exercise owning value transfer, inline-template
instantiation, empty/negative/wide sums and saturation. `mixed_consumer` includes
the API header before importing the module, and compares the shared function
address with a separate header-only TU using the same value type. Checks return
nonzero exit codes rather than `assert`, so `NDEBUG` does not disable them.
Tests do no I/O, open no windows and have a ten-second timeout. Textual standard
headers precede imports in these consumers; this order matters in this spike.

## Build and inspect

Requirements for this experiment only: CMake 4.4+, Ninja 1.11+ and a named-module
scanner supported by CMake. There are no downloads or extra runtime dependencies.
From an x64 VS 2026 Developer PowerShell, in this directory:

```powershell
cmake -S . -B build/msvc -G Ninja -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Debug
cmake --build build/msvc --verbose
ctest --test-dir build/msvc --output-on-failure --no-tests=error
dumpbin /EXPORTS build/msvc/toy_shared.dll
dumpbin /DEPENDENTS build/msvc/import_consumer.exe
```

Linux, substituting `g++-14`, `g++-16` or `clang++-20` and a distinct build tree:

```sh
cmake -S . -B build/gcc14 -G Ninja -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gcc14 --verbose
ctest --test-dir build/gcc14 --output-on-failure --no-tests=error
nm -D --defined-only build/gcc14/libtoy_shared.so
ldd build/gcc14/import_consumer
```

Header/shared baseline, independent of any module failure:

```powershell
cmake -S . -B build/header -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSPIKE_BUILD_MODULES=OFF
cmake --build build/header --verbose
ctest --test-dir build/header --output-on-failure --no-tests=error
```

For the toy's Doxygen API reference (Doxygen is optional): configure with
`-DSPIKE_BUILD_DOCS=ON`, then `cmake --build build/header --target spike_docs`.
This documents the actual toy declarations, independently of the design-only UI
sketches. Doxygen warnings are treated as errors.

## Observed combinations, 2026-10-07

Results are scoped to x64 Debug, Ninja 1.11.1, CMake 4.4.2 on Windows and 4.4.3
on Ubuntu 24.04 WSL2. They are exploratory evidence, not platform qualification.

| Compiler / standard library | Header + shared baseline | Default header / import / mixed | Optional import then header |
|---|---|---|---|
| MSVC 19.51.36257 / MSVC STL 145 (202604) | PASS 1/1 | PASS 3/3 | FAIL: duplicate inline template C2995 |
| GCC 14.3.0 / libstdc++ 14 | PASS 1/1 | PASS 3/3 | FAIL: duplicate Value/template definitions |
| GCC 16.0.1 experimental 20260315 / libstdc++ 16 | PASS 1/1 | PASS 3/3 | FAIL: duplicate Value/template definitions |
| Clang 20.1.2 / libstdc++ 16 selected by driver | PASS 1/1 | PASS 3/3 | PASS 4/4 with diagnostic enabled |

Exports and dynamic-loader dependencies were inspected on both platforms;
`bump`/`sum` resolve through the DLL/.so. Function-address identity is also checked
at runtime in the mixed consumer. This does not prove ABI compatibility between
compiler versions, libraries, CRT modes, platforms or changed declarations.

`SPIKE_CHECK_IMPORT_FIRST=ON` adds the diagnostic reverse-order consumer to the
build/tests. Keep it OFF (default) for the three supported usage patterns. To
reproduce the limitation without replacing modules by headers:

```powershell
cmake -S . -B build/reverse -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSPIKE_CHECK_IMPORT_FIRST=ON
cmake --build build/reverse --verbose
```

The first exploratory arrangement put the module directly in `toy_shared` and
textual STL includes after import. MSVC reported inconsistent DLL linkage and
duplicate STL definitions; GCC reported STL declaration failures. Those failed
builds motivated the separate facade and explicit include ordering. No claim is
made that this arrangement solves general include-after-import composition.

Clang with libc++, macOS, other compiler/STL pairings, installed consumers,
`import std`, header units, Visual Studio generators and cross-toolchain ABI/BMI
compatibility are not verified. Full module packaging remains separate work.

Primary references: [CMake named modules/scanning](https://cmake.org/cmake/help/v4.4/manual/cmake-cxxmodules.7.html),
[CMake FILE_SET](https://cmake.org/cmake/help/v4.4/command/target_sources.html),
[Clang ABI-preserving styles](https://clang.llvm.org/docs/StandardCPlusPlusModules.html#abi-preserving-styles),
[MSVC DLL exports](https://learn.microsoft.com/en-us/cpp/cpp/dllexport-dllimport?view=msvc-170),
[GCC modules](https://gcc.gnu.org/onlinedocs/gcc-14.3.0/gcc/C_002b_002b-Modules.html).
