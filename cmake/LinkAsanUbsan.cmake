# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

function(LinkAsan targetName)
    target_compile_options(${targetName} PRIVATE "-fsanitize=address")
    target_link_options(${targetName} PRIVATE "-fsanitize=address")
    #target_link_libraries(${targetName} PUBLIC asan ubsan)
endfunction()

function(LinkUbsan targetName)
    target_compile_options(${targetName} PRIVATE "-fsanitize=undefined")
    target_link_options(${targetName} PRIVATE "-fsanitize=undefined")
    #target_link_libraries(${targetName} PUBLIC asan ubsan)
endfunction()