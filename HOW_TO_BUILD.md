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

## Run Tests

```shell
cd build
ctest -C Release --extra-verbose
```
