# Process
Cross-platform library of utilities for obtaining metrics of the current process

[![Build](https://github.com/ttldtor/Process/actions/workflows/build.yml/badge.svg?branch=default)](https://github.com/ttldtor/Process/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/ttldtor/Process)](https://github.com/ttldtor/Process/releases/latest)
![](https://img.shields.io/badge/C++%20standard-C++14-blueviolet)
[![License](https://img.shields.io/badge/license-BSL--1.0-orange)](https://github.com/ttldtor/Process/blob/default/LICENSE)
[![Downloads](https://img.shields.io/github/downloads/ttldtor/Process/total)](https://github.com/ttldtor/Process/releases/latest)

### Status:
* [x] Windows
* [x] Linux
* [x] MacOS
* [x] Android. [Example](https://github.com/ttldtor/ProcessExample)
* [x] Termux
* [x] FreeBSD
* [ ] OpenBSD
* [ ] AIX
* [ ] ...

### Example:

_**CMakeLists.txt:**_
```cmake
cmake_minimum_required(VERSION 3.20)

project(Test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(Process GIT_REPOSITORY "https://github.com/ttldtor/Process.git" GIT_TAG default)
FetchContent_MakeAvailable(Process)

add_executable(${PROJECT_NAME}
        main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE process::process)

```

**_main.cpp:_**
```cpp
#include <process/process.hpp>

#include <chrono>
#include <iostream>
#include <vector>

using namespace std::literals;
using namespace org::ttldtor::process;

int main() {
    auto now = [] {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    };
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;

    auto t1 = now();

    while (now() - t1 <= 5) {
        static std::size_t i = 0;
        (void)(i++);
    }

    std::cout << "Total CPU time (+5s): " << Process::getTotalProcessorTime().count() << "ms" << std::endl;
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::vector<std::uint8_t> vb(10 * 1024 * 1024);
    std::cout << "Physical memory usage (+10MB): " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    vb.resize(1);
    vb.shrink_to_fit();
    std::cout << "Physical memory usage (-10MB): " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
}
```

_**Program output:**_
```text
Physical memory usage: 4168KB
Total CPU time: 0ms
Total CPU time (+5s): 5609ms
Physical memory usage: 4212KB
Physical memory usage (+10MB): 14460KB
Physical memory usage (-10MB): 4216KB
```
