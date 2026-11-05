# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Checks a build tree of package-consumer-check in the subdirectory or fetchcontent mode: the API, added as a
# subproject, set up no CPack configuration and installs nothing.
# Usage: cmake -DBINARY_DIR=<build tree> -DCONFIG=<configuration> -P check-subproject.cmake

foreach (file CPackConfig.cmake dxFeedGraalCxxApiPackConfig.cmake)
    if (EXISTS "${BINARY_DIR}/${file}")
        message(FATAL_ERROR "The subproject set up CPack: ${BINARY_DIR}/${file}")
    endif ()
endforeach ()

set(prefix "${BINARY_DIR}/subproject-install-check")
file(REMOVE_RECURSE "${prefix}")
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BINARY_DIR}" --prefix "${prefix}" --config "${CONFIG}"
        RESULT_VARIABLE result)

if (NOT result EQUAL 0)
    message(FATAL_ERROR "cmake --install failed: ${result}")
endif ()

file(GLOB_RECURSE installed LIST_DIRECTORIES false RELATIVE "${prefix}" "${prefix}/*")

if (installed)
    list(JOIN installed "\n  " installedList)
    message(FATAL_ERROR "The subproject installs files:\n  ${installedList}")
endif ()

message(STATUS "OK: no CPack configuration, nothing installed")
