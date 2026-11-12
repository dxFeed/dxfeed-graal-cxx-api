# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.21)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(dxFeedGraalCxxApi)

include(cmake/ParseVersion.cmake)

set(DXFCXX_VERSION "v8.2.0" CACHE STRING "The dxFeed Graal CXX API package version")

dxfcxx_ParseVersion(${DXFCXX_VERSION} DXFCXX_MAJOR_VERSION DXFCXX_MINOR_VERSION DXFCXX_PATCH_VERSION DXFCXX_SUFFIX_VERSION)

set(DXFEED_GRAAL_NATIVE_SDK_VERSION "3.16.0" CACHE STRING "")
set(FMTLIB_VERSION "12.1.0")
set(BOOST_VERSION "1.84.0")
set(UTFCPP_VERSION "3.2.3")
set(DATE_VERSION "3.0.1")
set(PROCESS_VERSION "3.0.1")
set(CONSOLE_VERSION "1.0.1")
set(PORTALS_VERSION "0.2.0")
set(BITS_VERSION "1.0.0")
set(RANGE_VERSION "0.12")
set(DOCTEST_VERSION "2.4.11")
set(GLFW_VERSION "3.4")
set(IMGUI_VERSION "1.92.9b")
set(CPM_CMAKE_VERSION "v0.40.2")
set(CONFIG_VERSION "1.0.3")

# SHA-256 of the downloaded archives (URL_HASH): a changed or substituted archive fails the configuration.
set(FMTLIB_SHA256 "695fd197fa5aff8fc67b5f2bbc110490a875cdf7a41686ac8512fb480fa8ada7")
set(BOOST_SHA256 "2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e")
set(GLFW_SHA256 "a133ddc3d3c66143eba9035621db8e0bcf34dba1ee9514a9e23e96afd39fd57a")
set(IMGUI_SHA256 "e1c46d676c2bcb7ced847ba27f50553e33a19db97b3cadaec7f8be64449139f8")
# The Graal Native SDK archives of the releases page by platform, for DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION only.
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION "3.16.0")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-linux "b7eef6512bcc059f4c3629f288c7a8abfded0c7efa880aa63060ba5431484350")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-linux-debug "4e339d4e2c800d68bdbdec3783dc19bfe53907321d1c352793bc4acd346b2dc6")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-linux "c9d3e2841309ba8de8cb8412dae03d263ccc0a299933bb4758a32abe81c4a077")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-linux-debug "018088f3d307233e7d368ea672f93339bae2e7d8ad8edca747bca0f046f5f850")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows "a1866cb8176ddc2428f882be50e1b877ba94f8a9f6646f5f8439e936ea4913a5")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-debug "c0ee20b5f3151259c33c298c410a414be5958a80e5d24130eea2bef754263e57")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-static-crt "71d73973e14742daf11336965b5478e74a93689e1ff503bee943b9076d321157")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-debug-static-crt "c5e5a5332af8d24f9706ccf5ea3ad5e55e37fe922948f615009511d795329bf8")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-static-lib "4414249520112d1a7eb1b53053d691d411b86484447a8e2c818d08fbb2845386")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_amd64-windows-debug-static-lib "6841fb36352c716d5652a9bc0ec64a5764ab2816748904dc53807976f223775e")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_x86_64-osx "2a37f734742bd02139e9fec634b2356369c5b002233a33580c4f5c01c7529bfd")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-osx "516046ec33d3fdddf0cfd86968fe3b76b8d3030f3677bd04de43fee9e96bbc8d")
set(DXFEED_GRAAL_NATIVE_SDK_SHA256_aarch64-ios "8c730443570b2b35cd2d5ab0489e3e632a27c112971d49fd05d1b387cee55dd7")

include(cmake/third_party/CPM.cmake-${CPM_CMAKE_VERSION}/CPM.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

if (NOT DEFINED DXFCXX_IS_ROOT_PROJECT)
    if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(DXFCXX_IS_ROOT_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    else ()
        set(DXFCXX_IS_ROOT_PROJECT OFF)
    endif ()
endif ()

option(DXFCXX_BUILD_UNIT_TESTS "Build tests" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(DXFCXX_BUILD_FUZZERS "Build the libFuzzer targets in fuzz/ (Clang only; instruments the static library for coverage)" OFF)
option(DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS "Dynamically link unit tests" OFF)
option(DXFCXX_ENABLE_NETWORK_TESTS "Build the unit tests that connect to demo.dxfeed.com (CTest label: network)" OFF)
option(DXFCXX_BUILD_SAMPLES "Build samples" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_DYNAMICALLY_LINK_SAMPLES "Dynamically link samples" OFF)
set(DXFCXX_BUILD_UI_SAMPLES "AUTO" CACHE STRING "Build the UI samples (Dear ImGui, GLFW, OpenGL): AUTO when their dependencies are found, ON (fail without them) or OFF")
set_property(CACHE DXFCXX_BUILD_UI_SAMPLES PROPERTY STRINGS AUTO ON OFF)
option(DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND "Require the GLFW Wayland backend of the UI samples (it is built whenever found)" OFF)
option(DXFCXX_BUILD_TOOLS "Build tools" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_DYNAMICALLY_LINK_TOOLS "Dynamically link tools" OFF)
option(DXFCXX_BUILD_DOC "Build documentation" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL "Prepare install" ON)
option(DXFCXX_INSTALL_LIB "Prepare install the libraries" ON)
option(DXFCXX_INSTALL_SAMPLES "Prepare install the samples" ${DXFCXX_IS_ROOT_PROJECT})
option(DXFCXX_INSTALL_TOOLS "Prepare install the tools" ${DXFCXX_IS_ROOT_PROJECT})

# The static-mt Full Source Bundle has only the Graal Native SDK with the static C runtime, so it builds with the static
# runtime by default. The platform is not detected yet, hence the literal Windows directory names (see the SDK selection).
set(DXFCXX_LINK_STATIC_RUNTIME_DEFAULT OFF)

if (WIN32
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}-amd64-windows-static-crt/CMakeLists.txt"
        AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}-amd64-windows/CMakeLists.txt")
    set(DXFCXX_LINK_STATIC_RUNTIME_DEFAULT ON)
endif ()

option(DXFCXX_LINK_STATIC_RUNTIME "Compile and link with -MT/-MTd or equivalent flag(s) to use a multi-threaded statically-linked runtime library. Visual Studio only." ${DXFCXX_LINK_STATIC_RUNTIME_DEFAULT})
option(DXFCXX_NODEFAULTLIB "Ignore libcmt/libcmtd/msvcrt/msvcrtd. Use if DXFCXX_LINK_STATIC_RUNTIME == ON." ${DXFCXX_LINK_STATIC_RUNTIME})

option(DXFCXX_ENABLE_METRICS "Enable metrics collection" OFF)

option(DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_GITHUB "" ON)
set(DXFEED_GRAAL_NATIVE_SDK_BASE_URL "https://github.com/dxFeed/dxfeed-graal-native-sdk/releases/download/" CACHE STRING "")
option(DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK "" OFF)

option(DXFCXX_USE_DXFG_STUB "Use dxFeed Graal SDK stub" OFF)
set(DXFG_STUB_REPO "https://github.com/ttldtor/DxFeedGraalNativeSdkStub" CACHE STRING "")

set(DXFG_LOCAL_ARCHIVE_PATH "" CACHE STRING "")

option(DXFCXX_ENABLE_ASAN "Enable AddressSanitizer for the library and all its consumers" OFF)
option(DXFCXX_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (can be combined with ASan; not supported by MSVC)" OFF)
option(DXFCXX_ENABLE_TSAN "Enable ThreadSanitizer (cannot be combined with ASan; not supported by MSVC)" OFF)
option(DXFCXX_USE_PRECOMPILED_HEADERS "Use precompiled headers" ON)
option(DXFCXX_BUILD_HEADER_CHECK "Compile every public header on its own to check that it is self-contained" OFF)

option(DXFCXX_FEATURE_STACKTRACE "Allow capturing stacktraces using boost::stacktrace" OFF)

# https://developercommunity.visualstudio.com/t/Invalid-code-generation-in-release-1940/10678572?sort=newest&viewtype=all
# https://github.com/microsoft/STL/wiki/VS-2022-Changelog#vs-2022-1710
# https://github.com/microsoft/STL/pull/3824
# https://github.com/actions/runner-images/issues/10004
option(DXFCXX_MSVC_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR "Enable _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR workaround for old MSVC runtimes" OFF)


option(DXFCXX_TRACING "Enable tracing" OFF)
option(DXFCXX_LIST_TRACING "Enable list tracing" OFF)
option(DXFCXX_ISOLATES_TRACING "Enable isolates tracing" OFF)

include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

set(DXFCXX_GRAAL_TARGET_PLATFORM "unknown-unknown" CACHE STRING "")
set(DXFCXX_TARGET_PLATFORM "unknown-unknown" CACHE STRING "")
set(DXFCXX_TARGET_OS "unknown" CACHE STRING "")
set(DXFCXX_GRAAL_TARGET_OS "unknown" CACHE STRING "")
set(DXFCXX_TARGET_CPU "unknown" CACHE STRING "")
set(DXFCXX_GRAAL_TARGET_CPU "unknown" CACHE STRING "")

include(cmake/ParseAndDetectPlatforms.cmake)
include(cmake/LinkStacktrace.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/CopyRuntime.cmake)
include(cmake/SetupStaticRuntimeMSVC.cmake)

# The Graal Native SDK archive that the options select, named as on the releases page: the platform, -debug, and on
# Windows with the static runtime the SDK DLL with the static C runtime (-static-crt) and the static library of the SDK
# (-static-lib) that is only installed (both archives define the target DxFeedGraalNativeSdk).
set(DXFEED_GRAAL_NATIVE_SDK_ARCHIVE ${DXFCXX_GRAAL_TARGET_PLATFORM})

if (DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK)
    set(DXFEED_GRAAL_NATIVE_SDK_ARCHIVE ${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}-debug)
endif ()

if (WIN32 AND DXFCXX_LINK_STATIC_RUNTIME)
    set(DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE ${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}-static-lib)
    set(DXFEED_GRAAL_NATIVE_SDK_ARCHIVE ${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}-static-crt)
endif ()

# The Full Source Bundles have the SDK archives unpacked into third_party/graal-native-sdk-<version>-<archive>. If there
# is one for the platform, the archive that the options select must be there too.
set(DXFCXX_BUNDLED_SDK_NAME_PREFIX "graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}-")
set(DXFCXX_BUNDLED_SDK_PREFIX "third_party/${DXFCXX_BUNDLED_SDK_NAME_PREFIX}")
file(GLOB DXFCXX_BUNDLED_SDK_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/third_party" LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/${DXFCXX_BUNDLED_SDK_PREFIX}${DXFCXX_GRAAL_TARGET_PLATFORM}*/CMakeLists.txt")

if (DXFCXX_BUNDLED_SDK_FILES)
    set(DXFCXX_BUNDLED_SDK_MISSING "")

    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DXFCXX_BUNDLED_SDK_PREFIX}${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}/CMakeLists.txt")
        list(APPEND DXFCXX_BUNDLED_SDK_MISSING ${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE})
    endif ()

    if (DEFINED DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE AND NOT EXISTS
            "${CMAKE_CURRENT_SOURCE_DIR}/${DXFCXX_BUNDLED_SDK_PREFIX}${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE}/DxFeedGraalNativeSdk_static.lib")
        list(APPEND DXFCXX_BUNDLED_SDK_MISSING ${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE})
    endif ()

    if (DXFCXX_BUNDLED_SDK_MISSING)
        set(DXFCXX_BUNDLED_SDK_ARCHIVES "")

        # graal-native-sdk-<version>-<archive>/CMakeLists.txt -> <archive>
        foreach (DXFCXX_BUNDLED_SDK_FILE IN LISTS DXFCXX_BUNDLED_SDK_FILES)
            string(REPLACE "${DXFCXX_BUNDLED_SDK_NAME_PREFIX}" "" DXFCXX_BUNDLED_SDK_FILE "${DXFCXX_BUNDLED_SDK_FILE}")
            string(REPLACE "/CMakeLists.txt" "" DXFCXX_BUNDLED_SDK_FILE "${DXFCXX_BUNDLED_SDK_FILE}")
            list(APPEND DXFCXX_BUNDLED_SDK_ARCHIVES ${DXFCXX_BUNDLED_SDK_FILE})
        endforeach ()

        list(SORT DXFCXX_BUNDLED_SDK_ARCHIVES)
        list(JOIN DXFCXX_BUNDLED_SDK_ARCHIVES ", " DXFCXX_BUNDLED_SDK_ARCHIVES_TEXT)
        list(JOIN DXFCXX_BUNDLED_SDK_MISSING " and " DXFCXX_BUNDLED_SDK_MISSING_TEXT)
        set(DXFCXX_BUNDLED_SDK_ERROR "third_party has the Graal Native SDK ${DXFEED_GRAAL_NATIVE_SDK_VERSION} ${DXFCXX_BUNDLED_SDK_ARCHIVES_TEXT}, but not ${DXFCXX_BUNDLED_SDK_MISSING_TEXT} that DXFCXX_LINK_STATIC_RUNTIME=${DXFCXX_LINK_STATIC_RUNTIME} and DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK=${DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK} select.")

        if (WIN32 AND DXFCXX_LINK_STATIC_RUNTIME)
            string(APPEND DXFCXX_BUNDLED_SDK_ERROR " The SDK with the static C runtime is in the *-Full-Source-Bundle-static-mt.zip bundle. To build from *-Full-Source-Bundle.zip, set DXFCXX_LINK_STATIC_RUNTIME=OFF.")
        elseif (WIN32)
            string(APPEND DXFCXX_BUNDLED_SDK_ERROR " The SDK with the dynamic C runtime is in the *-Full-Source-Bundle.zip bundle. To build from *-Full-Source-Bundle-static-mt.zip, set DXFCXX_LINK_STATIC_RUNTIME=ON (its default).")
        endif ()

        message(FATAL_ERROR "${DXFCXX_BUNDLED_SDK_ERROR}")
    endif ()

    message(STATUS "Graal Native SDK: ${DXFCXX_BUNDLED_SDK_PREFIX}${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}")
    add_subdirectory(${DXFCXX_BUNDLED_SDK_PREFIX}${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE})

    if (DEFINED DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE)
        message(STATUS "Graal Native SDK static library: ${DXFCXX_BUNDLED_SDK_PREFIX}${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE}")
        set(DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${DXFCXX_BUNDLED_SDK_PREFIX}${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE}")
    endif ()
else ()
    if (NOT DXFG_LOCAL_ARCHIVE_PATH STREQUAL "")
        FetchContent_Declare(DxFeedGraalNativeSdk
                URL "${DXFG_LOCAL_ARCHIVE_PATH}"
        )
    else ()
        if (DXFCXX_USE_DXFG_STUB)
            FetchContent_Declare(
                    DxFeedGraalNativeSdk
                    GIT_REPOSITORY "${DXFG_STUB_REPO}.git" GIT_TAG "v${DXFEED_GRAAL_NATIVE_SDK_VERSION}"
                    USES_TERMINAL_DOWNLOAD TRUE
                    GIT_PROGRESS TRUE
                    DOWNLOAD_NO_EXTRACT FALSE
            )
        else ()
            if (DEFINED ENV{DXFEED_GRAAL_NATIVE_SDK_URL})
                FetchContent_Declare(
                        DxFeedGraalNativeSdk
                        URL $ENV{DXFEED_GRAAL_NATIVE_SDK_URL}
                        USES_TERMINAL_DOWNLOAD TRUE
                        GIT_PROGRESS TRUE
                        DOWNLOAD_NO_EXTRACT FALSE
                )
            else ()
                if (DXFCXX_USE_DXFEED_GRAAL_NATIVE_SDK_GITHUB)
                    set(DXFEED_GRAAL_NATIVE_SDK_URL "${DXFEED_GRAAL_NATIVE_SDK_BASE_URL}v${DXFEED_GRAAL_NATIVE_SDK_VERSION}/graal-native-sdk-${DXFEED_GRAAL_NATIVE_SDK_VERSION}")
                endif ()

                # The platforms that have Graal Native SDK archives on the releases page.
                set(DXFCXX_GRAAL_SUPPORTED_PLATFORMS amd64-linux aarch64-linux amd64-windows x86_64-osx aarch64-osx aarch64-ios)
                set(DXFCXX_GRAAL_SUPPORTED_DEBUG_PLATFORMS amd64-linux aarch64-linux amd64-windows)

                if (DXFCXX_USE_DEBUG_DXFEED_GRAAL_NATIVE_SDK)
                    set(DXFCXX_GRAAL_SUPPORTED_PLATFORMS ${DXFCXX_GRAAL_SUPPORTED_DEBUG_PLATFORMS})
                endif ()

                if (NOT DXFCXX_GRAAL_TARGET_PLATFORM IN_LIST DXFCXX_GRAAL_SUPPORTED_PLATFORMS)
                    list(JOIN DXFCXX_GRAAL_SUPPORTED_PLATFORMS ", " DXFCXX_GRAAL_SUPPORTED_PLATFORMS_TEXT)
                    message(FATAL_ERROR "Unsupported platform ${DXFCXX_GRAAL_TARGET_PLATFORM}: the Graal Native SDK archives are built for ${DXFCXX_GRAAL_SUPPORTED_PLATFORMS_TEXT}. To use another build, set DXFG_LOCAL_ARCHIVE_PATH or the DXFEED_GRAAL_NATIVE_SDK_URL environment variable.")
                endif ()

                if (WIN32 AND DXFCXX_LINK_STATIC_RUNTIME AND DXFEED_GRAAL_NATIVE_SDK_VERSION VERSION_LESS "3.9.0")
                    message(FATAL_ERROR "DXFCXX_LINK_STATIC_RUNTIME needs the Graal Native SDK 3.9.0 or newer (the static-crt and static-lib archives), not ${DXFEED_GRAAL_NATIVE_SDK_VERSION}")
                endif ()

                set(DXFEED_GRAAL_NATIVE_SDK_ARCHIVE_URL_PREFIX ${DXFEED_GRAAL_NATIVE_SDK_URL})
                set(DXFEED_GRAAL_NATIVE_SDK_URL ${DXFEED_GRAAL_NATIVE_SDK_URL}-${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}.zip)
                set(DXFEED_GRAAL_NATIVE_SDK_URL_HASH "")
                set(DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_URL_HASH "")

                if (DXFEED_GRAAL_NATIVE_SDK_VERSION STREQUAL DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION)
                    set(DXFEED_GRAAL_NATIVE_SDK_URL_HASH URL_HASH SHA256=${DXFEED_GRAAL_NATIVE_SDK_SHA256_${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE}})

                    if (DEFINED DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE)
                        set(DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_URL_HASH URL_HASH SHA256=${DXFEED_GRAAL_NATIVE_SDK_SHA256_${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE}})
                    endif ()
                else ()
                    message(WARNING "No SHA-256 for the Graal Native SDK ${DXFEED_GRAAL_NATIVE_SDK_VERSION} (known: ${DXFEED_GRAAL_NATIVE_SDK_SHA256_VERSION}): the downloaded archive is not verified")
                endif ()

                if (DEFINED DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE)
                    # SOURCE_SUBDIR without CMakeLists.txt: download and extract only.
                    FetchContent_Declare(
                            DxFeedGraalNativeSdkStaticLib
                            URL ${DXFEED_GRAAL_NATIVE_SDK_ARCHIVE_URL_PREFIX}-${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_ARCHIVE}.zip
                            ${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_URL_HASH}
                            SOURCE_SUBDIR no-cmake-project
                            USES_TERMINAL_DOWNLOAD TRUE
                            DOWNLOAD_NO_EXTRACT FALSE
                    )
                    FetchContent_MakeAvailable(DxFeedGraalNativeSdkStaticLib)
                    set(DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_DIR "${dxfeedgraalnativesdkstaticlib_SOURCE_DIR}")
                endif ()

                FetchContent_Declare(
                        DxFeedGraalNativeSdk
                        URL ${DXFEED_GRAAL_NATIVE_SDK_URL}
                        ${DXFEED_GRAAL_NATIVE_SDK_URL_HASH}
                        USES_TERMINAL_DOWNLOAD TRUE
                        GIT_PROGRESS TRUE
                        DOWNLOAD_NO_EXTRACT FALSE
                )
            endif ()
        endif ()
    endif ()

    FetchContent_MakeAvailable(DxFeedGraalNativeSdk)
    # DxFeedGraalNativeSdk_SOURCE_DIR
endif ()

if (WIN32 AND DXFCXX_LINK_STATIC_RUNTIME AND NOT DEFINED DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_DIR)
    message(WARNING "DXFCXX_LINK_STATIC_RUNTIME: the Graal Native SDK is taken neither from the releases page nor from the static-mt Full Source Bundle, so its C runtime and its static library are up to the given SDK")
endif ()


if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Process-${PROCESS_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/Process-${PROCESS_VERSION})
else ()
    CPMAddPackage("gh:ttldtor/Process#v${PROCESS_VERSION}")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    SetupStaticRuntimeMSVC(process ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Console-${CONSOLE_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/Console-${CONSOLE_VERSION})
else ()
    CPMAddPackage("gh:ttldtor/Console#v${CONSOLE_VERSION}")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    SetupStaticRuntimeMSVC(console ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

# bits (header-only): the bit operations of the library headers. EXCLUDE_FROM_ALL disables its own install rules
# (its headers are installed with the library below): FetchContent passes it to add_subdirectory() since CMake 3.28,
# older versions ignore it there.
if (NOT DEFINED FETCHCONTENT_SOURCE_DIR_BITS AND
        EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/bits-${BITS_VERSION}/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_BITS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/bits-${BITS_VERSION}")
endif ()

FetchContent_Declare(
        bits
        GIT_REPOSITORY https://github.com/ttldtor/bits.git
        GIT_TAG 9b59e45d2cc75303ffc4cc6d5b42363f8ab3508b # v1.0.0
        EXCLUDE_FROM_ALL
)
if (CMAKE_VERSION VERSION_LESS 3.28)
    FetchContent_GetProperties(bits)
    if (NOT bits_POPULATED)
        FetchContent_Populate(bits)
        add_subdirectory(${bits_SOURCE_DIR} ${bits_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif ()
else ()
    FetchContent_MakeAvailable(bits)
endif ()

add_subdirectory(third_party/utfcpp-${UTFCPP_VERSION})

set(FMT_INSTALL OFF)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt-${FMTLIB_VERSION}/CMakeLists.txt")
    add_subdirectory(third_party/fmt-${FMTLIB_VERSION})
else ()
    FetchContent_Declare(
            fmt
            URL https://github.com/fmtlib/fmt/releases/download/${FMTLIB_VERSION}/fmt-${FMTLIB_VERSION}.zip # downloading a zip release speeds up the download
            URL_HASH SHA256=${FMTLIB_SHA256}
            USES_TERMINAL_DOWNLOAD TRUE
            GIT_PROGRESS TRUE
            DOWNLOAD_NO_EXTRACT FALSE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(fmt)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    SetupStaticRuntimeMSVC(fmt ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
endif ()

#set(BUILD_TZ_LIB ON)
#set(USE_SYSTEM_TZ_DB ON)
add_subdirectory(third_party/date-${DATE_VERSION})

if (DXFCXX_FEATURE_STACKTRACE)
    set(BOOST_INCLUDE_LIBRARIES stacktrace)

    if (DXFCXX_LINK_STATIC_RUNTIME AND
            (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
                    (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")))
        set(BOOST_RUNTIME_LINK static CACHE STRING "Boost runtime library selection for the MS ABI" FORCE)
    endif ()

    CPMAddPackage(
            NAME Boost
            VERSION ${BOOST_VERSION}
            URL https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}.tar.xz
            URL_HASH SHA256=${BOOST_SHA256}
            OPTIONS "BOOST_ENABLE_CMAKE ON"
            USES_TERMINAL_DOWNLOAD TRUE
            GIT_PROGRESS TRUE
            OVERRIDE_FIND_PACKAGE
    )
endif ()

# config (header-only) is used by the library sources only: EXCLUDE_FROM_ALL keeps its install rules (headers and
# a CMake package config) out of the packages.
add_subdirectory(third_party/config-${CONFIG_VERSION} EXCLUDE_FROM_ALL)

include(cmake/Warnings.cmake)
dxfcxx_system_includes(fmt::fmt-header-only fmt::fmt utf8cpp date::date config::config bits::bits process::process
        console::console)

if (DXFCXX_FEATURE_STACKTRACE AND DEFINED Boost_SOURCE_DIR)
    dxfcxx_system_includes_in_directory("${Boost_SOURCE_DIR}")
endif ()

set(dxFeedGraalCxxApi_Internal_Sources
        src/internal/CEntryPointErrors.cpp
        src/internal/Isolate.cpp
        src/internal/JavaObjectHandle.cpp
        src/internal/EventClassList.cpp
        src/internal/Common.cpp
        src/internal/Metrics.cpp
        src/internal/Platform.cpp
        src/internal/StopWatch.cpp
        src/internal/TimeFormat.cpp
        src/internal/Timer.cpp
)

set(dxFeedGraalCxxApi_InternalUtils_Sources
        src/internal/utils/StringUtils.cpp
        src/internal/utils/EnumUtils.cpp
        src/internal/utils/CmdArgsUtils.cpp
)

set(dxFeedGraalCxxApi_InternalUtilsDebug_Sources
        src/internal/utils/debug/Debug.cpp
)

set(dxFeedGraalCxxApi_Exceptions_Sources
        src/exceptions/RuntimeException.cpp
        src/exceptions/InvalidArgumentException.cpp
        src/exceptions/GraalException.cpp
        src/exceptions/JavaException.cpp
)

set(dxFeedGraalCxxApi_Executors_Sources
        src/executors/InPlaceExecutor.cpp
)

set(dxFeedGraalCxxApi_Glossary_Sources
        src/glossary/AdditionalUnderlyings.cpp
        src/glossary/CFI.cpp
        src/glossary/PriceIncrements.cpp
)

set(dxFeedGraalCxxApi_Isolated_Sources
        src/isolated/api/IsolatedDXFeed.cpp
        src/isolated/api/IsolatedDXEndpoint.cpp
        src/isolated/api/IsolatedDXFeedSubscription.cpp
        src/isolated/api/IsolatedDXPublisher.cpp
        src/isolated/api/IsolatedDXPublisherObservableSubscription.cpp
        src/isolated/api/osub/IsolatedObservableSubscriptionChangeListener.cpp
        src/isolated/candlewebservice/IsolatedHistoryEndpoint.cpp
        src/isolated/auth/IsolatedAuthToken.cpp
        src/isolated/event/IsolatedEventType.cpp
        src/isolated/executors/IsolatedInPlaceExecutor.cpp
        src/isolated/glossary/IsolatedAdditionalUnderlyings.cpp
        src/isolated/glossary/IsolatedCFI.cpp
        src/isolated/glossary/IsolatedPriceIncrements.cpp
        src/isolated/internal/IsolatedString.cpp
        src/isolated/internal/IsolatedObject.cpp
        src/isolated/internal/IsolatedTimeFormat.cpp
        src/isolated/internal/IsolatedTools.cpp
        src/isolated/promise/IsolatedPromise.cpp
        src/isolated/ipf/IsolatedInstrumentProfile.cpp
        src/isolated/ipf/IsolatedInstrumentProfileReader.cpp
        src/isolated/ipf/live/IsolatedInstrumentProfileCollector.cpp
        src/isolated/ipf/live/IsolatedInstrumentProfileConnection.cpp
        src/isolated/logging/IsolatedLogging.cpp
        src/isolated/metrics/IsolatedMetrics.cpp
        src/isolated/model/IsolatedTxModelListener.cpp
        src/isolated/model/IsolatedIndexedTxModel.cpp
        src/isolated/model/IsolatedTimeSeriesTxModel.cpp
        src/isolated/ondemand/IsolatedOnDemandService.cpp
        src/isolated/util/IsolatedTimePeriod.cpp
        src/isolated/schedule/IsolatedDay.cpp
        src/isolated/schedule/IsolatedSession.cpp
        src/isolated/schedule/IsolatedSchedule.cpp
)

set(dxFeedGraalCxxApi_Api_Sources
        src/api/DXEndpoint.cpp
        src/api/DXFeed.cpp
        src/api/DXFeedSubscription.cpp
        src/api/DXPublisherObservableSubscription.cpp
        src/api/DXPublisher.cpp
)

set(dxFeedGraalCxxApi_ApiOsub_Sources
        src/api/osub/WildcardSymbol.cpp
        src/api/osub/TimeSeriesSubscriptionSymbol.cpp
        src/api/osub/IndexedEventSubscriptionSymbol.cpp
        src/api/osub/ObservableSubscriptionChangeListener.cpp
)

set(dxFeedGraalCxxApi_Auth_Sources
        src/auth/AuthToken.cpp
)

set(dxFeedGraalCxxApi_CandleWebService_Sources
        src/candlewebservice/HistoryEndpoint.cpp
)

set(dxFeedGraalCxxApi_Ipf_Sources
        src/ipf/InstrumentProfile.cpp
        src/ipf/InstrumentProfileType.cpp
        src/ipf/InstrumentProfileField.cpp
        src/ipf/InstrumentProfileReader.cpp
        src/ipf/live/InstrumentProfileCollector.cpp
        src/ipf/live/InstrumentProfileConnection.cpp
        src/ipf/live/IterableInstrumentProfile.cpp
)

set(dxFeedGraalCxxApi_Logging_Sources
        src/logging/Logging.cpp
)

set(dxFeedGraalCxxApi_Model_Sources
        src/model/TxModelListener.cpp
        src/model/IndexedTxModel.cpp
        src/model/TimeSeriesTxModel.cpp
        src/model/MarketDepthModel.cpp
)

set(dxFeedGraalCxxApi_OnDemand_Sources
        src/ondemand/OnDemandService.cpp
)

set(dxFeedGraalCxxApi_Promise_Sources
        src/promise/Promise.cpp
        src/promise/Promises.cpp
)

set(dxFeedGraalCxxApi_Symbols_Sources
        src/symbols/StringSymbol.cpp
        src/symbols/SymbolWrapper.cpp
)

set(dxFeedGraalCxxApi_System_Sources
        src/system/System.cpp
)

set(dxFeedGraalCxxApi_Event_Sources
        src/event/EventTypeEnum.cpp
        src/event/IndexedEventSource.cpp
        src/event/EventFlag.cpp
        src/event/IndexedEvent.cpp
        src/event/EventMapper.cpp
        src/event/EventSourceWrapper.cpp
        src/event/TimeSeriesEvent.cpp
)

set(dxFeedGraalCxxApi_EventCandle_Sources
        src/event/candle/Candle.cpp
        src/event/candle/CandleAlignment.cpp
        src/event/candle/CandleExchange.cpp
        src/event/candle/CandlePeriod.cpp
        src/event/candle/CandlePrice.cpp
        src/event/candle/CandlePriceLevel.cpp
        src/event/candle/CandleType.cpp
        src/event/candle/CandleSession.cpp
        src/event/candle/CandleSymbol.cpp
)

set(dxFeedGraalCxxApi_EventMarket_Sources
        src/event/market/AnalyticOrder.cpp
        src/event/market/OtcMarketsPriceType.cpp
        src/event/market/OtcMarketsOrder.cpp
        src/event/market/Direction.cpp
        src/event/market/IcebergType.cpp
        src/event/market/MarketEvent.cpp
        src/event/market/MarketEventSymbols.cpp
        src/event/market/OptionSale.cpp
        src/event/market/Order.cpp
        src/event/market/OrderAction.cpp
        src/event/market/OrderBase.cpp
        src/event/market/OrderSource.cpp
        src/event/market/PriceType.cpp
        src/event/market/Profile.cpp
        src/event/market/Quote.cpp
        src/event/market/Scope.cpp
        src/event/market/ShortSaleRestriction.cpp
        src/event/market/Side.cpp
        src/event/market/SpreadOrder.cpp
        src/event/market/Summary.cpp
        src/event/market/TimeAndSale.cpp
        src/event/market/TimeAndSaleType.cpp
        src/event/market/Trade.cpp
        src/event/market/TradeBase.cpp
        src/event/market/TradeETH.cpp
        src/event/market/TradingStatus.cpp
)

set(dxFeedGraalCxxApi_EventOption_Sources
        src/event/option/Greeks.cpp
        src/event/option/Underlying.cpp
        src/event/option/TheoPrice.cpp
        src/event/option/Series.cpp
)

set(dxFeedGraalCxxApi_EventMisc_Sources
        src/event/misc/Message.cpp
        src/event/misc/TextMessage.cpp
)

set(dxFeedGraalCxxApi_Schedule_Sources
        src/schedule/SessionFilter.cpp
        src/schedule/SessionType.cpp
        src/schedule/DayFilter.cpp
        src/schedule/Day.cpp
        src/schedule/Session.cpp
        src/schedule/Schedule.cpp
)

set(dxFeedGraalCxxApi_Util_Sources
        src/util/TimePeriod.cpp
)

set(dxFeedGraalCxxApi_Sources
        ${dxFeedGraalCxxApi_Internal_Sources}
        ${dxFeedGraalCxxApi_InternalUtils_Sources}
        ${dxFeedGraalCxxApi_InternalUtilsDebug_Sources}
        ${dxFeedGraalCxxApi_Isolated_Sources}
        ${dxFeedGraalCxxApi_Exceptions_Sources}
        ${dxFeedGraalCxxApi_Executors_Sources}
        ${dxFeedGraalCxxApi_Glossary_Sources}
        ${dxFeedGraalCxxApi_Api_Sources}
        ${dxFeedGraalCxxApi_ApiOsub_Sources}
        ${dxFeedGraalCxxApi_Auth_Sources}
        ${dxFeedGraalCxxApi_CandleWebService_Sources}
        ${dxFeedGraalCxxApi_Ipf_Sources}
        ${dxFeedGraalCxxApi_Logging_Sources}
        ${dxFeedGraalCxxApi_Model_Sources}
        ${dxFeedGraalCxxApi_OnDemand_Sources}
        ${dxFeedGraalCxxApi_Promise_Sources}
        ${dxFeedGraalCxxApi_Symbols_Sources}
        ${dxFeedGraalCxxApi_System_Sources}
        ${dxFeedGraalCxxApi_Event_Sources}
        ${dxFeedGraalCxxApi_EventCandle_Sources}
        ${dxFeedGraalCxxApi_EventMarket_Sources}
        ${dxFeedGraalCxxApi_EventOption_Sources}
        ${dxFeedGraalCxxApi_EventMisc_Sources}
        ${dxFeedGraalCxxApi_Schedule_Sources}
        ${dxFeedGraalCxxApi_Util_Sources}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR};@loader_path/../../../${CMAKE_INSTALL_LIBDIR};@loader_path;@executable_path;@executable_path/../Frameworks")
elseif (UNIX)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../../../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../lib64:$ORIGIN/../lib:$ORIGIN")
endif ()

add_library(${PROJECT_NAME}_static STATIC
        ${dxFeedGraalCxxApi_Sources}
        src/api.cpp
)

if (WIN32)
    if (MINGW)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            list(APPEND CMAKE_RC_FLAGS "--use-temp-file")
        endif ()
    endif ()

    add_library(${PROJECT_NAME} SHARED
            ${dxFeedGraalCxxApi_Sources}
            resources/dxFeedGraalCxxApi.rc
            src/api.cpp
    )
else ()
    add_library(${PROJECT_NAME} SHARED
            ${dxFeedGraalCxxApi_Sources}
            src/api.cpp
    )
endif ()

add_library(dxfcxx ALIAS ${PROJECT_NAME})
add_library(dxfcxx::static ALIAS ${PROJECT_NAME}_static)
add_library(dxfcxx::graal ALIAS DxFeedGraalNativeSdk)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    # set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    SetupStaticRuntimeMSVC(${PROJECT_NAME}_static ${DXFCXX_LINK_STATIC_RUNTIME} ${DXFCXX_NODEFAULTLIB})
    # The DLL links the static CRT itself, so no /NODEFAULTLIB.
    SetupStaticRuntimeMSVC(${PROJECT_NAME} ${DXFCXX_LINK_STATIC_RUNTIME} OFF)
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        DXFCPP_USE_DLLS LIBDXFCPP_EXPORTS FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)

target_include_directories(${PROJECT_NAME}_static PUBLIC include)
target_include_directories(${PROJECT_NAME}_static SYSTEM PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
target_compile_definitions(${PROJECT_NAME}_static PRIVATE
        FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)

# The Graal Native SDK marks the dxfg_ functions that have dxfg_v4_ twins deprecated (DXFG_DEPRECATED, SDK 3.12+).
# The library still calls them, so it compiles without these warnings: GCC and Clang (clang-cl too) get an empty
# DXFG_DEPRECATED, cl cannot define a function-like macro on the command line and disables its warning C4996 instead.
# The definition goes through the compile options: CMake drops function-like macros from the compile definitions.
foreach (DXFCXX_LIBRARY_TARGET IN ITEMS ${PROJECT_NAME} ${PROJECT_NAME}_static)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${DXFCXX_LIBRARY_TARGET} PRIVATE /wd4996)
    else ()
        target_compile_options(${DXFCXX_LIBRARY_TARGET} PRIVATE "-DDXFG_DEPRECATED(message)=")
    endif ()
endforeach ()

# fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
set(DXFCXX_MSVC_COMPILE_OPTIONS /W4 /bigobj /utf-8)
set(DXFCXX_MSVC_DEBUG_LINK_OPTIONS /PDBALTPATH:$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>)
set(DXFCXX_GCC_LIKE_COMPILE_OPTIONS -fPIC -Wall -Wextra -pedantic)
set(DXFCXX_GCC_LIKE_LINK_OPTIONS "-fPIC")

#A workaround to fix 'relocation truncated to fit: IMAGE_REL_AMD64_SECREL' error (MinGW + Debug)
set(DXFCXX_MINGW_DEBUG_LINK_OPTIONS "-Wl,--disable-dynamicbase,--disable-high-entropy-va,--default-image-base-low")

add_library(dxfcxx_mingw_debug_link_options_interface INTERFACE)

if (MINGW)
    target_link_options(dxfcxx_mingw_debug_link_options_interface
            INTERFACE
            $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:${DXFCXX_MINGW_DEBUG_LINK_OPTIONS}>
    )
endif ()

if (DXFCXX_MSVC_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
    # https://developercommunity.visualstudio.com/t/Invalid-code-generation-in-release-1940/10678572?sort=newest&viewtype=all
    # https://github.com/microsoft/STL/wiki/VS-2022-Changelog#vs-2022-1710
    # https://github.com/microsoft/STL/pull/3824
    # https://github.com/actions/runner-images/issues/10004
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_definitions(${PROJECT_NAME} PUBLIC _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
        target_compile_definitions(${PROJECT_NAME}_static PUBLIC _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
    endif ()
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(DXFCXX_GCC_LIKE_COMPILE_OPTIONS ${DXFCXX_GCC_LIKE_COMPILE_OPTIONS} "-Wa,--noexecstack")
    set(DXFCXX_GCC_LIKE_LINK_OPTIONS ${DXFCXX_GCC_LIKE_LINK_OPTIONS} "-Wl,-z,noexecstack")
endif ()

target_compile_options(${PROJECT_NAME}
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
)

target_link_options(${PROJECT_NAME}
        PUBLIC
        $<$<AND:$<COMPILE_LANG_AND_ID:CXX,MSVC>,$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>:${DXFCXX_MSVC_DEBUG_LINK_OPTIONS}>
        $<$<AND:$<COMPILE_LANG_AND_ID:C,MSVC>,$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>:${DXFCXX_MSVC_DEBUG_LINK_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
)

target_compile_options(${PROJECT_NAME}_static
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,MSVC>:${DXFCXX_MSVC_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_COMPILE_OPTIONS}>
)

target_link_options(${PROJECT_NAME}_static
        PUBLIC
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
        $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,Intel>:${DXFCXX_GCC_LIKE_LINK_OPTIONS}>
)

if (DXFCXX_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(${PROJECT_NAME} PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
    target_precompile_headers(${PROJECT_NAME}_static PRIVATE include/dxfeed_graal_cpp_api/internal/PrecompiledHeaders.hpp)
endif ()

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        bits::bits
        PRIVATE
        dxfcxx_warnings
        DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date config::config dxfcxx_mingw_debug_link_options_interface
)

target_link_libraries(${PROJECT_NAME}_static
        PUBLIC
        bits::bits
        PRIVATE
        dxfcxx_warnings
        DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date config::config dxfcxx_mingw_debug_link_options_interface
)

if (DXFCXX_FEATURE_STACKTRACE)
    LinkStacktrace(${PROJECT_NAME})
    LinkStacktrace(${PROJECT_NAME}_static)

    target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCXX_FEATURE_STACKTRACE)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCXX_FEATURE_STACKTRACE)
endif ()

# Threads: before glibc 2.34, pthread is a separate library (libpthread). PUBLIC: the library and the programs that
# use std::thread with it link it.
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
target_link_libraries(${PROJECT_NAME}_static PUBLIC Threads::Threads)

# Both library targets are instrumented; PUBLIC propagates the options and the sanitizer runtime to every consumer.
target_link_libraries(${PROJECT_NAME} PUBLIC dxfcxx_sanitizers)
target_link_libraries(${PROJECT_NAME}_static PUBLIC dxfcxx_sanitizers)

if (DXFCXX_BUILD_FUZZERS)
    # libFuzzer ships with LLVM Clang only (not with AppleClang, GCC or MSVC).
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "DXFCXX_BUILD_FUZZERS requires LLVM Clang (libFuzzer), the compiler is ${CMAKE_CXX_COMPILER_ID}")
    endif ()

    # Coverage instrumentation of the library code for libFuzzer; the fuzz targets link libFuzzer itself.
    target_compile_options(${PROJECT_NAME}_static PRIVATE -fsanitize=fuzzer-no-link)
endif ()

if (DXFCXX_SANITIZERS_ENABLED)
    # Deliberately defective functions for the sanitizer canary tests (tests/sanitizers); sanitized builds only.
    target_sources(${PROJECT_NAME} PRIVATE src/internal/SanitizerCanary.cpp)
    target_sources(${PROJECT_NAME}_static PRIVATE src/internal/SanitizerCanary.cpp)
endif ()

if (DXFCXX_BUILD_HEADER_CHECK)
    # One translation unit per public header, containing only `#include <header>` and compiled without precompiled
    # headers: a header that relies on includes it does not make fails to compile here.
    file(GLOB_RECURSE DXFCXX_PUBLIC_HEADERS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
    set(DXFCXX_HEADER_CHECK_SOURCES "")

    foreach (header ${DXFCXX_PUBLIC_HEADERS})
        string(MAKE_C_IDENTIFIER "${header}" headerId)

        if (header MATCHES "\\.h$")
            set(source "${CMAKE_CURRENT_BINARY_DIR}/header_check/${headerId}.c") # C headers are checked as C
        else ()
            set(source "${CMAKE_CURRENT_BINARY_DIR}/header_check/${headerId}.cpp")
        endif ()

        file(CONFIGURE OUTPUT "${source}" CONTENT "#include <${header}>\n")
        list(APPEND DXFCXX_HEADER_CHECK_SOURCES "${source}")
    endforeach ()

    add_library(dxfcxx_header_check OBJECT ${DXFCXX_HEADER_CHECK_SOURCES})
    # The same include directories, definitions, options and dependencies as the library itself.
    target_link_libraries(dxfcxx_header_check PRIVATE ${PROJECT_NAME}_static dxfcxx_warnings
            DxFeedGraalNativeSdk utf8cpp fmt::fmt-header-only date::date config::config)
    target_include_directories(dxfcxx_header_check SYSTEM PRIVATE third_party/range-v3-${RANGE_VERSION}/include)
    target_compile_definitions(dxfcxx_header_check PRIVATE FMT_HEADER_ONLY=1 _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1)
endif ()

dxfcxx_copy_runtime(dxfcxx_graal_runtime FILES $<TARGET_FILE:DxFeedGraalNativeSdk>
        TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        DXFCXX_VERSION="${DXFCXX_VERSION}"
        DXFCXX_MAJOR_VERSION=${DXFCXX_MAJOR_VERSION}
        DXFCXX_MINOR_VERSION=${DXFCXX_MINOR_VERSION}
        DXFCXX_PATCH_VERSION=${DXFCXX_PATCH_VERSION}
)

target_compile_definitions(${PROJECT_NAME}_static PUBLIC
        DXFCXX_VERSION="${DXFCXX_VERSION}"
        DXFCXX_MAJOR_VERSION=${DXFCXX_MAJOR_VERSION}
        DXFCXX_MINOR_VERSION=${DXFCXX_MINOR_VERSION}
        DXFCXX_PATCH_VERSION=${DXFCXX_PATCH_VERSION}
)

if (DXFCXX_ENABLE_METRICS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCXX_ENABLE_METRICS=1)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCXX_ENABLE_METRICS=1)
endif ()

if (DXFCXX_TRACING)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCPP_DEBUG=1)
    target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCPP_DEBUG=1)

    if (DXFCXX_LIST_TRACING)
        target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCPP_TRACE_LISTS=1)
        target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCPP_TRACE_LISTS=1)
    endif ()

    if (DXFCXX_ISOLATES_TRACING)
        target_compile_definitions(${PROJECT_NAME} PUBLIC DXFCPP_TRACE_ISOLATES=1)
        target_compile_definitions(${PROJECT_NAME}_static PUBLIC DXFCPP_TRACE_ISOLATES=1)
    endif ()
endif ()

if (DXFCXX_BUILD_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif ()

if (DXFCXX_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif ()

if (DXFCXX_BUILD_SAMPLES)
    add_subdirectory(samples/cpp/API/AuthSample)
    add_subdirectory(samples/cpp/API/ConnectSample)
    add_subdirectory(samples/cpp/API/PrintQuoteEventsSample)
    add_subdirectory(samples/cpp/API/PublishProfilesSample)
    add_subdirectory(samples/cpp/API/QuoteAndTradeSample)
    add_subdirectory(samples/cpp/API/ReconnectSample)
    add_subdirectory(samples/cpp/API/RequestProfileSample)
    add_subdirectory(samples/cpp/Candle/FetchDailyCandlesSample)
    add_subdirectory(samples/cpp/Candle/CandleDataSample)
    add_subdirectory(samples/cpp/Console/LastEventConsoleSample)
    add_subdirectory(samples/cpp/File/ConvertTapeFileSample)
    add_subdirectory(samples/cpp/File/FileParserSample)
    add_subdirectory(samples/cpp/File/WriteTapeFileSample)
    add_subdirectory(samples/cpp/IPF/IpfSample)
    add_subdirectory(samples/cpp/IPF/IpfLiveSample)
    add_subdirectory(samples/cpp/IPF/OptionChainSample)
    add_subdirectory(samples/cpp/Model/IncOrderSnapshotSample)
    add_subdirectory(samples/cpp/Model/MultipleMarketDepthSample)
    add_subdirectory(samples/cpp/Model/PriceLevelBookSample)
    add_subdirectory(samples/cpp/OnDemand/OnDemandSample)
    add_subdirectory(samples/cpp/Schedule/ScheduleSample)

    include(cmake/Portals.cmake)
    dxfcxx_add_portals(DXFCXX_PORTALS_ENABLED)

    if (DXFCXX_PORTALS_ENABLED)
        add_subdirectory(samples/cpp/Model/MarketDepthModelSample)
    endif ()

    include(cmake/UiSamples.cmake)
    dxfcxx_check_ui_samples(DXFCXX_UI_SAMPLES_ENABLED)

    if (DXFCXX_UI_SAMPLES_ENABLED)
        add_subdirectory(samples/cpp/UI/DXFeedTimeAndSalesSample)
    endif ()
endif ()

add_subdirectory(docs)

if (DXFCXX_BUILD_TOOLS)
    add_subdirectory(tools/Tools)
endif ()

if (DXFCXX_INSTALL)
    include(GNUInstallDirs)
    set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/${PROJECT_NAME}PackConfig.cmake")
    set(CPACK_SOURCE_GENERATOR ZIP)
    set(CPACK_PACKAGE_VENDOR "Devexperts LLC")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${DXFCXX_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${DXFCXX_VERSION}-${DXFCXX_TARGET_PLATFORM}${DXFCXX_PACKAGE_SUFFIX}")
    include(CPack)

    if (DXFCXX_INSTALL_LIB)
        #install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
        # With the static runtime on Windows the DLL has its own C runtime and heap, and the C++ API passes the objects
        # of the standard library across the DLL boundary, so the DLL is built but not installed: only its C API could
        # be used safely, and those who need it build the DLL themselves.
        if (NOT (WIN32 AND DXFCXX_LINK_STATIC_RUNTIME))
            install(TARGETS ${PROJECT_NAME})
        endif ()
        install(TARGETS ${PROJECT_NAME}_static)
        install(DIRECTORY "include" DESTINATION "." PATTERN "build" EXCLUDE)
        # The public headers include <bits/bits.hpp>.
        install(DIRECTORY "${bits_SOURCE_DIR}/include/" DESTINATION "include")

        # The -static-mt archives: _static.lib links either with the SDK import library and DLL, or with the static
        # library of the SDK and the Windows libraries of link-flags.txt.
        if (WIN32 AND DXFCXX_LINK_STATIC_RUNTIME)
            install(FILES $<TARGET_LINKER_FILE:DxFeedGraalNativeSdk> DESTINATION lib)

            if (DEFINED DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_DIR)
                install(FILES
                        "${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_DIR}/DxFeedGraalNativeSdk_static.lib"
                        "${DXFEED_GRAAL_NATIVE_SDK_STATIC_LIB_DIR}/link-flags.txt"
                        DESTINATION lib)
            endif ()
        endif ()
    endif ()

    install(FILES "DEPENDENCIES.md" "LICENSE" "README.md" "ReleaseNotes.md" "THIRD_PARTY_LICENSES.md" DESTINATION ".")
endif ()
