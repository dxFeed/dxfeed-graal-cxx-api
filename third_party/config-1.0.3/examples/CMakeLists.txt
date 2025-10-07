# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

add_executable(simple
        simple.cpp
)

set_target_properties(simple PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
)

set(ORG_TTLDTOR_CONFIG_MSVC_COMPILE_OPTIONS /W4 /utf-8)
set(ORG_TTLDTOR_CONFIG_GCC_LIKE_COMPILE_OPTIONS -Wall -Wextra -pedantic)

target_link_libraries(simple PRIVATE config::config)
target_compile_options(simple
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:${ORG_TTLDTOR_CONFIG_MSVC_COMPILE_OPTIONS}>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang,Intel>:${ORG_TTLDTOR_CONFIG_GCC_LIKE_COMPILE_OPTIONS}>
)
