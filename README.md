# dxFeed Native API

![Build status](https://github.com/dxFeed/dxfeed-native-api/actions/workflows/build.yml/badge.svg?branch=main)

A project repository that is a dxFeed API written in C++ and provides a C\Obj-C\C++ interface for using the dxFeed Java
API features via dxfeed-graal-native-api as a backend

# Dependencies

## Compile-time

- dxFeed Native Graal API v0.1.0
- [utfcpp](https://github.com/nemtrif/utfcpp) v3.2.1
- [fmt](https://github.com/fmtlib/fmt) v9.1.0
- [Catch2](https://github.com/catchorg/Catch2) v3.1.0 (Tests)

## Run-time

- dxFeed Native Graal API v0.1.0
- [fmt](https://github.com/fmtlib/fmt) v9.1.0 (C++)
- [Catch2](https://github.com/catchorg/Catch2) v3.1.0 (Tests)

# TBD

- [ ] Endpoint entity
- [ ] DXFeed "anchor" entity
- [ ] DXPublisher "anchor" entity
- [ ] Subscription entity
- [ ] Native events mapper
- [x] System properties processing
- [ ] Obj-C artifacts (also for the Swift's FFI)
- [ ] C\C++ samples
- [ ] CI\CD
    - [x] Build
    - [ ] Tests
        - [x] System properties processing
    - [ ] Packaging
- [ ] Docs
    - [x] System properties processing

# How To Build

## Prerequisites

- python
- Conan
- CMake
- clang\gcc\Visual Studio

## Build

```shell
mkdir build
cd build
conan install ..
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release 

```

## Run Tests

```shell
cd build
ctest -C Release --extra-verbose
```

## Known Conan Package Manager Issues

**Ubuntu**: it is possible that Conan incorrectly sets the C ABI for the C++ standard library (5 instead of 11),
in which case it needs to be run with the following options:

```shell
conan install -s compiler.libcxx=libstdc++11 ..
```

**Windows**: it is possible that the Conan repository does not contain packages for the current version of the Visual
Studio compiler (which is detected automatically and set to the default profile). You can set the previous version,
for example 16:

```shell
conan install -s compiler.version=16 ..
```

**Windows or Mac or Linux**: it is possible that the Conan repository does not contain packages (catch2, fmt) for the
current
version of a compiler. You can build them:

```shell
conan install --build=catch2 --build=fmt ..
```

or

```shell
conan install --build missing ..
```

# License

MPL-2.0
