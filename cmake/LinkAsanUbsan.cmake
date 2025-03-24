# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

function(LinkAsanUbsan targetName)
    target_compile_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
    target_link_libraries(${PROJECT_NAME} PUBLIC asan ubsan)
endfunction()