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

or use the `scripts` dir. Make sure that you use `command prompt for VS` on Win.

## Run Tests

```shell
cd build
ctest -C Release --extra-verbose
```
