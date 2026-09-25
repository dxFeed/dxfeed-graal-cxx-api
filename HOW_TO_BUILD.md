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
ctest -C Release --extra-verbose
```

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
