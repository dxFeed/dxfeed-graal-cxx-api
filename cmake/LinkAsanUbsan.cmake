# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

function(LinkAsanUbsan targetName)
    target_compile_options(${targetName} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_options(${targetName} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_libraries(${targetName} PUBLIC asan ubsan)
endfunction()