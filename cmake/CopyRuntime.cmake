# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Sets <outputVariable> to the directory of the file of an executable or library target, as $<TARGET_FILE_DIR:target>
# gives it, but without referencing the target: the Visual Studio generator turns such a reference in a custom target
# into a dependency on the target, which is a cycle when the target depends on the custom target.
function(dxfcxx_target_output_directory target outputVariable)
    get_target_property(type ${target} TYPE)

    if (type STREQUAL "EXECUTABLE" OR (WIN32 AND type STREQUAL "SHARED_LIBRARY"))
        set(kind RUNTIME)
    elseif (type STREQUAL "SHARED_LIBRARY" OR type STREQUAL "MODULE_LIBRARY")
        set(kind LIBRARY)
    else ()
        set(kind ARCHIVE)
    endif ()

    get_target_property(directory ${target} ${kind}_OUTPUT_DIRECTORY)

    if (NOT directory)
        get_target_property(directory ${target} BINARY_DIR)
    endif ()

    # Multi-configuration generators append the configuration, unless the directory has a generator expression.
    get_property(multiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if (multiConfig AND NOT directory MATCHES "\\$<")
        string(APPEND directory "/$<CONFIG>")
    endif ()

    set(${outputVariable} "${directory}" PARENT_SCOPE)
endfunction()

# Copies the libraries that several targets load at run time (the Graal Native SDK library, the shared library of this
# project, the sanitizer runtime) into their common output directory (the one of the first target) by one custom target
# <name>, which the given targets depend on. A POST_BUILD copy in each target copied the same files into the same
# directory at the same time in a parallel build and sometimes failed ("Error copying file (if different) ... No such
# file or directory"). The custom target runs on every build (before the targets): copy_if_different copies only the
# files that changed.
#
# dxfcxx_copy_runtime(<name> FILES <file>... TARGETS <target>... [DEPENDS <target>...])
#   FILES    the files to copy (generator expressions are allowed)
#   TARGETS  the targets that need the files next to them, all with the same output directory
#   DEPENDS  the targets that build the files (they are built before the copy)
function(dxfcxx_copy_runtime name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "FILES;TARGETS;DEPENDS")
    list(GET arg_TARGETS 0 firstTarget)
    dxfcxx_target_output_directory(${firstTarget} directory)

    # The copy runs before the targets are built, so their directory may not exist yet (the configuration subdirectory
    # of a multi-configuration generator), and copy_if_different of one file would create a file with its name.
    add_custom_target(${name}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${directory}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${arg_FILES} "${directory}"
            VERBATIM)

    if (arg_DEPENDS)
        add_dependencies(${name} ${arg_DEPENDS})
    endif ()

    foreach (target IN LISTS arg_TARGETS)
        add_dependencies(${target} ${name})
    endforeach ()
endfunction()
