# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

add_executable(simple
        simple.cpp
)

set_target_properties(simple PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
)

target_link_libraries(simple PRIVATE config::config)
