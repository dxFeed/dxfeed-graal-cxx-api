# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Adds Portals (the terminal output of MarketDepthModelSample) when the toolchain can build it: CMake 3.24 or newer and
# a C++23 compiler and standard library with std::expected, std::format, std::move_only_function and std::stop_token
# (Apple libc++ of Xcode 16 has no std::stop_token and std::move_only_function). Otherwise prints one status line and adds
# nothing, so the library and the other samples keep their CMake 3.21 and C++20 requirements.
# Uses third_party/portals-<PORTALS_VERSION> when it is present, otherwise the pinned commit from GitHub. Its targets are
# EXCLUDE_FROM_ALL: they are built only for the sample and are not installed.
# Sets <result_var> to ON when the portals::core target is available.
function(dxfcxx_add_portals result_var)
    set(${result_var} OFF PARENT_SCOPE)
    set(reason "")

    if (CMAKE_VERSION VERSION_LESS 3.24)
        set(reason "CMake ${CMAKE_VERSION} (3.24 is needed)")
    elseif (NOT "cxx_std_23" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        set(reason "no C++23 support in ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    else ()
        include(CheckCXXSourceCompiles)

        set(CMAKE_CXX_STANDARD 23)
        set(CMAKE_REQUIRED_QUIET ON)
        check_cxx_source_compiles([[
            #include <expected>
            #include <format>
            #include <functional>
            #include <stop_token>
            #include <string>

            int main() {
                std::expected<int, std::string> value = 1;
                std::stop_source source;
                std::stop_callback callback(source.get_token(), [] {});
                std::move_only_function<int()> function = [] { return 1; };

                return std::format("{}", *value) == "1" && function() == 1 ? 0 : 1;
            }
        ]] DXFCXX_HAVE_CXX23_FOR_PORTALS)

        if (NOT DXFCXX_HAVE_CXX23_FOR_PORTALS)
            set(reason "no std::expected, std::format, std::move_only_function or std::stop_token in the C++23 standard library of ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
        endif ()
    endif ()

    if (reason)
        message(STATUS "MarketDepthModelSample is skipped: Portals ${PORTALS_VERSION} needs CMake 3.24 and C++23, ${reason}")

        return()
    endif ()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/portals-${PORTALS_VERSION}/CMakeLists.txt")
        add_subdirectory(third_party/portals-${PORTALS_VERSION} EXCLUDE_FROM_ALL)
    else ()
        CPMAddPackage(NAME portals
                GITHUB_REPOSITORY ttldtor/portals
                GIT_TAG 7072f51766150531be5fcf864f6e2d9e92fffb7b # v0.2.0
                EXCLUDE_FROM_ALL YES)
    endif ()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
        SetupStaticRuntimeMSVC(portals_core ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
        SetupStaticRuntimeMSVC(portals_utf8proc ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
    endif ()

    dxfcxx_system_includes(portals::core)
    set(${result_var} ON PARENT_SCOPE)
endfunction()
