# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# A project that uses the C++ API in one of the ways it is delivered (DXFCXX_CONSUMER_MODE):
#   package      - an installed (or unpacked) package: find_package(dxFeedGraalCxxApi CONFIG), CMAKE_PREFIX_PATH points
#                  to it;
#   subdirectory - the sources as a directory of the project: add_subdirectory(DXFCXX_SOURCE_DIR);
#   fetchcontent - the sources by FetchContent (SOURCE_DIR DXFCXX_SOURCE_DIR, no network).
# It builds two programs with each library target it gets (dxfcxx::static, dxfcxx::dxfcxx): the publication and receipt
# of a quote through a LOCAL_HUB endpoint (../static-mt-check/main.cpp) and the types of the API across the library
# boundary (types.cpp), and registers them as tests. The project's own code is compiled with hidden visibility and with
# the default options only: the library must not need options from its users.
# Usage:
#   cmake -S .github/ci/package-consumer-check -B <dir> -DDXFCXX_CONSUMER_MODE=package -DCMAKE_PREFIX_PATH=<prefix>
#     (a -static-mt package: add -DDXFCXX_CONSUMER_EXPECT_SHARED=OFF
#     "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>")
#   cmake -S .github/ci/package-consumer-check -B <dir> -DDXFCXX_CONSUMER_MODE=subdirectory -DDXFCXX_SOURCE_DIR=<repo>
#   cmake -S .github/ci/package-consumer-check -B <dir> -DDXFCXX_CONSUMER_MODE=fetchcontent -DDXFCXX_SOURCE_DIR=<repo>
#   cmake --build <dir> && ctest --test-dir <dir> --output-on-failure

cmake_minimum_required(VERSION 3.21)

project(dxfcxx_package_consumer_check LANGUAGES CXX)

set(DXFCXX_CONSUMER_MODE "package" CACHE STRING "How the API is used: package, subdirectory or fetchcontent")
set(DXFCXX_SOURCE_DIR "" CACHE PATH "The sources of the API (the subdirectory and fetchcontent modes)")
option(DXFCXX_CONSUMER_EXPECT_SHARED
        "The API has the shared library target dxfcxx::dxfcxx (OFF for the -static-mt packages: their shared library is not exported)"
        ON)

# No C++ standard is set here: the library targets require the one their headers need.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# The programs find the libraries next to them (Windows) or by RUNPATH (Linux, macOS).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

if (DXFCXX_CONSUMER_MODE STREQUAL "package")
    find_package(dxFeedGraalCxxApi CONFIG REQUIRED)
    message(STATUS "dxFeedGraalCxxApi ${dxFeedGraalCxxApi_VERSION}: ${dxFeedGraalCxxApi_DIR}")

    # SameMajorVersion: the package is found for its own major version, not for an older one (a newer one is never
    # found in any compatibility mode).
    set(major ${dxFeedGraalCxxApi_VERSION_MAJOR})
    find_package(dxFeedGraalCxxApi ${major}.0 CONFIG REQUIRED)

    if (major GREATER 0)
        math(EXPR olderMajor "${major} - 1")
        find_package(dxFeedGraalCxxApi ${olderMajor} CONFIG QUIET)

        if (dxFeedGraalCxxApi_FOUND)
            message(FATAL_ERROR "find_package(dxFeedGraalCxxApi ${olderMajor}) finds ${dxFeedGraalCxxApi_VERSION}")
        endif ()
    endif ()
elseif (DXFCXX_CONSUMER_MODE STREQUAL "subdirectory" OR DXFCXX_CONSUMER_MODE STREQUAL "fetchcontent")
    if (NOT EXISTS "${DXFCXX_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "No sources of the API in '${DXFCXX_SOURCE_DIR}': set DXFCXX_SOURCE_DIR")
    endif ()

    # What a subproject must not change in this project (checked after it is added).
    set(BOOST_RUNTIME_LINK "shared" CACHE STRING "The choice of this project")

    if (DXFCXX_CONSUMER_MODE STREQUAL "subdirectory")
        add_subdirectory("${DXFCXX_SOURCE_DIR}" dxfcxx)
    else ()
        include(FetchContent)
        FetchContent_Declare(dxFeedGraalCxxApi SOURCE_DIR "${DXFCXX_SOURCE_DIR}")
        FetchContent_MakeAvailable(dxFeedGraalCxxApi)
    endif ()

    if (NOT BOOST_RUNTIME_LINK STREQUAL "shared")
        message(FATAL_ERROR "The API changed BOOST_RUNTIME_LINK of the project to '${BOOST_RUNTIME_LINK}'")
    endif ()

    set(DXFCXX_CHECK_SUBPROJECT ON)
else ()
    message(FATAL_ERROR "DXFCXX_CONSUMER_MODE: package, subdirectory or fetchcontent, not '${DXFCXX_CONSUMER_MODE}'")
endif ()

foreach (target dxfcxx::static dxfcxx::graal)
    if (NOT TARGET ${target})
        message(FATAL_ERROR "No target ${target}")
    endif ()
endforeach ()

set(DXFCXX_CHECK_KINDS static)

if (DXFCXX_CONSUMER_EXPECT_SHARED)
    if (NOT TARGET dxfcxx::dxfcxx)
        message(FATAL_ERROR "No target dxfcxx::dxfcxx")
    endif ()

    list(APPEND DXFCXX_CHECK_KINDS shared)
elseif (TARGET dxfcxx::dxfcxx)
    message(FATAL_ERROR "Target dxfcxx::dxfcxx in a package without the shared library")
endif ()

enable_testing()

foreach (kind IN LISTS DXFCXX_CHECK_KINDS)
    if (kind STREQUAL "static")
        set(library dxfcxx::static)
    else ()
        set(library dxfcxx::dxfcxx)
    endif ()

    add_executable(local_hub_${kind} "${CMAKE_CURRENT_SOURCE_DIR}/../static-mt-check/main.cpp")
    add_executable(types_${kind} types.cpp)

    foreach (program local_hub_${kind} types_${kind})
        target_link_libraries(${program} PRIVATE ${library})
        add_test(NAME ${program} COMMAND ${program})
        set_tests_properties(${program} PROPERTIES TIMEOUT 120)

        # Windows has no RUNPATH: the DLLs the program loads (the SDK library, the shared library of the API) go next
        # to it. Linux and macOS find the libraries by RUNPATH.
        if (WIN32)
            add_custom_command(TARGET ${program} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${program}>
                    "$<TARGET_FILE_DIR:${program}>"
                    COMMAND_EXPAND_LISTS VERBATIM)
        endif ()
    endforeach ()
endforeach ()

# The build options of the library do not reach this project (the generators that write compile_commands.json).
if (CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    add_test(NAME no_options_of_the_library
            COMMAND ${CMAKE_COMMAND} -DDXFCXX_CONSUMER_BINARY_DIR=${CMAKE_BINARY_DIR}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check-options.cmake")
endif ()

if (DXFCXX_CHECK_SUBPROJECT)
    # The installation and the packages of this project stay its own: the API adds no install rules and no CPack
    # configuration.
    add_test(NAME subproject_leaves_install_and_cpack
            COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_BINARY_DIR} -DCONFIG=$<CONFIG>
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check-subproject.cmake")
endif ()
