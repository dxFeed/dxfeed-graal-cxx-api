# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

function(LinkAsan targetName)
    if (WIN32)
        if (MINGW)
            target_compile_options(${targetName} PRIVATE "-fsanitize=address")
            target_link_options(${targetName} PRIVATE "-fsanitize=address")
        else ()
            target_compile_options(${targetName} PRIVATE "/fsanitize=address")
            target_link_options(${targetName} PRIVATE "/fsanitize=address")

            target_compile_definitions(${targetName}
                    PUBLIC
                    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:
                    _DISABLE_VECTOR_ANNOTATION
                    _DISABLE_STRING_ANNOTATION
                    >
            )
        endif ()
    else ()
        target_compile_options(${targetName} PRIVATE "-fsanitize=address")
        target_link_options(${targetName} PRIVATE "-fsanitize=address")
    endif ()
endfunction()

function(LinkUbsan targetName)
    if (WIN32)
        if (MINGW)
            target_compile_options(${targetName} PRIVATE "-fsanitize=address")
            target_link_options(${targetName} PRIVATE "-fsanitize=address")
        endif ()
    else ()
        target_compile_options(${targetName} PRIVATE "-fsanitize=undefined")
        target_link_options(${targetName} PRIVATE "-fsanitize=undefined")
    endif ()
endfunction()