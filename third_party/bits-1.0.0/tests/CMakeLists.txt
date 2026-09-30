# Copyright (c) 2025 ttldtor.
# SPDX-License-Identifier: BSL-1.0

include(FetchContent)
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12)
FetchContent_MakeAvailable(doctest)

FetchContent_Declare(
        nanobench
        GIT_REPOSITORY https://github.com/martinus/nanobench.git
        GIT_TAG v4.1.0
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nanobench)

add_executable(bits_tests
        main.cpp
        bits.cpp
)

set_target_properties(bits_tests PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
)

target_link_libraries(bits_tests PRIVATE bits::bits)
target_include_directories(bits_tests PRIVATE ${doctest_SOURCE_DIR})

add_test(NAME bits_tests COMMAND bits_tests)

add_executable(bits_bench
        main.cpp
        bench.cpp
)

set_target_properties(bits_bench PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
)

target_link_libraries(bits_bench PRIVATE bits::bits nanobench)
target_include_directories(bits_bench PRIVATE ${doctest_SOURCE_DIR})

if (CMAKE_CONFIGURATION_TYPES)
    set_property(TARGET bits_bench PROPERTY EXCLUDE_FROM_DEFAULT_BUILD_Debug TRUE)
    add_test(NAME bits_bench COMMAND bits_bench)
    set_tests_properties(bits_bench PROPERTIES DISABLED "$<CONFIG:Debug>")
else ()
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_test(NAME bits_bench COMMAND bits_bench)
    else ()
        set_property(TARGET bits_bench PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif ()
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(nanobench PRIVATE -march=native -Werror -Xclang -fcxx-exceptions)
        target_compile_options(bits_bench PRIVATE -march=native -Werror -Xclang -fcxx-exceptions)
    else ()
        if (APPLE AND CMAKE_OSX_ARCHITECTURES)
            message(STATUS "${CMAKE_OSX_ARCHITECTURES}")
            list(FIND CMAKE_OSX_ARCHITECTURES "x86_64" _x86_index)
            if (_x86_index GREATER -1)
                target_compile_options(bits_bench PRIVATE -O3 -Werror -pedantic)
            else ()
                target_compile_options(bits_bench PRIVATE -O3 -march=native -Werror -pedantic)
            endif ()
        else ()
            target_compile_options(bits_bench PRIVATE -O3 -march=native -Werror -pedantic)
        endif ()
    endif ()
endif ()


if (MSVC)
    include(CheckCompilerFlag)
    check_compiler_flag(CXX /arch:AVX2 HAVE_AVX2)

    if (HAVE_AVX2)
        target_compile_options(bits_bench PRIVATE
                $<$<AND:$<CXX_COMPILER_ID:CXX,MSVC>,$<CONFIG:Release>>:/O2 /arch:AVX2>
        )
    endif ()
endif ()
