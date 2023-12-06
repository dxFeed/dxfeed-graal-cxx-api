# Copyright (c) 2023 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.21)

if (POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif ()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

project(ConvertTapeFile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CXX_EXTENSIONS OFF)
set(C_EXTENSIONS OFF)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR};@loader_path;@executable_path;@executable_path/../Frameworks")
elseif (UNIX)
    set(CMAKE_SKIP_BUILD_RPATH ON)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}:$ORIGIN/../lib64:$ORIGIN/../lib:$ORIGIN")
endif ()

add_executable(${PROJECT_NAME} src/main.cpp)

#if (DXFCXX_ENABLE_VS_ASAN)
#    target_compile_options(${PROJECT_NAME} PRIVATE "/fsanitize=address")
#    target_link_options(${PROJECT_NAME} PRIVATE "/fsanitize=address")
#
#    target_compile_definitions(${PROJECT_NAME}
#            PUBLIC
#            $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:
#            _DISABLE_VECTOR_ANNOTATION
#            _DISABLE_STRING_ANNOTATION
#            >
#    )
#endif ()

#if (DXFCXX_ENABLE_ASAN_UBSAN)
#    target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx)
#    target_compile_definitions(${PROJECT_NAME} PRIVATE DXFCPP_USE_DLLS)
#    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            $<TARGET_FILE:dxfcxx::graal>
#            ${CMAKE_CURRENT_SOURCE_DIR}/ConvertTapeFile.in
#            $<TARGET_FILE:dxfcxx>
#            $<TARGET_FILE_DIR:${PROJECT_NAME}>)
#    target_compile_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
#    target_link_options(${PROJECT_NAME} PRIVATE "-fsanitize=address" "-fsanitize=undefined")
#else ()
#    target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx::static)
#    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
#            $<TARGET_FILE:dxfcxx::graal>
#            ${CMAKE_CURRENT_SOURCE_DIR}/ConvertTapeFile.in
#            $<TARGET_FILE_DIR:${PROJECT_NAME}>)
#endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE dxfcxx::static)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dxfcxx::graal>
        ${CMAKE_CURRENT_SOURCE_DIR}/ConvertTapeFile.in
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

if (DXFCXX_INSTALL AND DXFCXX_INSTALL_SAMPLES)
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME})
    install(FILES "ConvertTapeFile.in" DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME})

    if (WIN32)
        install(FILES $<TARGET_FILE:dxfcxx::graal> DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME})
    endif ()
endif ()