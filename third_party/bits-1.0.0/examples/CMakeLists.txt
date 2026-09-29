# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

add_executable(colormanip
        colormanip.cpp
)

set_target_properties(colormanip PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
)

target_link_libraries(colormanip PRIVATE bits::bits)
