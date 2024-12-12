# Console
Cross-platform library of utilities for working with the console.

[![Build](https://github.com/ttldtor/Console/actions/workflows/build.yml/badge.svg?branch=default)](https://github.com/ttldtor/Console/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/ttldtor/Console)](https://github.com/ttldtor/Console/releases/latest)
![](https://img.shields.io/badge/C++%20standard-C++11-blueviolet)
[![License](https://img.shields.io/badge/license-BSL--1.0-orange)](https://github.com/ttldtor/Console/blob/default/LICENSE)
[![Downloads](https://img.shields.io/github/downloads/ttldtor/Console/total)](https://github.com/ttldtor/Console/releases/latest)

### Status:
* [x] Windows
* [x] Linux
* [x] MacOS
* [ ] Termux
* [ ] FreeBSD
* [ ] OpenBSD
* [ ] AIX
* [ ] ...

### Example:

_**CMakeLists.txt:**_
```cmake
cmake_minimum_required(VERSION 3.20)

project(Test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(Console GIT_REPOSITORY "https://github.com/ttldtor/Console.git" GIT_TAG default)
FetchContent_MakeAvailable(Console)

add_executable(${PROJECT_NAME}
        main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE console::console)

```

**_main.cpp:_**
```cpp
#include <iostream>

#include <console/console.hpp>

using namespace org::ttldtor::console;

int main() {
    auto consoleSize = Console::getSize();

    std::cout << "Console size: " << consoleSize.first << " x " << consoleSize.second << std::endl;

    return 0;
}
```

_**Program output:**_
```text
Console size: 120 x 27
```