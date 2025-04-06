# Copyright (c) 2024 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.21)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(BuildDocumentation)

if (DXFCXX_BUILD_DOC)

    # check if Doxygen is installed
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_OUT Doxyfile)

        message("Doxygen build started")

        add_custom_target(${PROJECT_NAME} ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)

endif (DXFCXX_BUILD_DOC)