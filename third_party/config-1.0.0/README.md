# config
Library for configuring applications.

![](https://img.shields.io/badge/C++%20standard-C++20-blueviolet)
[![Release](https://img.shields.io/github/v/release/ttldtor/config)](https://github.com/ttldtor/config/releases/latest)
[![License](https://img.shields.io/badge/license-BSL--1.0-orange)](https://github.com/ttldtor/config/blob/default/LICENSE.txt)

## Examples of usage

### CMake

```cmake
cmake_minimum_required(VERSION 3.16)
project(test_config LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
        config
        GIT_REPOSITORY https://github.com/ttldtor/config.git
        GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(config)

add_executable(${PROJECT_NAME} src/main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE config)
```

### Code

```c++
#include <iostream>
#include <config/config.hpp>

using namespace org::ttldtor::config;

int main() {
  Config config{};

  config.addSource(IniSource::fromString(R"("
user=demo
password=demo
  ")"));

  for (auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  // set MY_PREFIX_user=test
  config.addSource(EnvSource("MY_PREFIX_"));

  for (auto& [key, value] : config) {
    std::cout << key << " = " << value << '\n';
  }

  return 0;
}
```
