# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

include(FetchContent)
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12)
FetchContent_MakeAvailable(doctest)

add_executable(config_tests
        main.cpp
        config.cpp
)

set_target_properties(config_tests PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
)

target_link_libraries(config_tests PRIVATE config::config)
target_include_directories(config_tests PRIVATE ${doctest_SOURCE_DIR})

add_test(NAME config_tests COMMAND config_tests)
