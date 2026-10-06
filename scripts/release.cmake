# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Prepares a release of the dxFeed Graal C++ API with CMake only: the same as scripts/release.py (see
# HOW_TO_RELEASE.md).
#
#   cmake -P scripts/release.cmake vX.Y.Z[-alphaN|-betaN|-preN|-rcN]  sets the version (CMakeLists.txt, docs/Doxyfile,
#                                     the inline namespace of Conf.hpp) and the heading of ReleaseNotes.md, commits
#                                     "vX.Y.Z..." and tags it
#   cmake -P scripts/release.cmake vX.Y.Z-draftN               a trial run of release.yml: only the tag
#   cmake -P scripts/release.cmake <version> --push            the same, then pushes the branch and the tag
#   cmake -P scripts/release.cmake <version> --dry-run         prints what would change and changes nothing
#   cmake -P scripts/release.cmake <version> --notes [--ref <ref>]  prints the notes of the version
#   cmake -P scripts/release.cmake vX.Y.Z-draftN --cleanup     deletes the draft release and its tag (GitHub CLI)
#
# The text of ReleaseNotes.md is processed as one string (never as a list: it may contain semicolons).

cmake_minimum_required(VERSION 3.21)

if (NOT DEFINED RELEASE_ROOT)
    get_filename_component(RELEASE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif ()

set(RELEASE_NOTES "ReleaseNotes.md")
set(RELEASE_CMAKE "CMakeLists.txt")
set(RELEASE_DOXYFILE "docs/Doxyfile")
set(RELEASE_CONF "include/dxfeed_graal_cpp_api/internal/Conf.hpp")
set(RELEASE_PRE_RELEASE_ORDER alpha beta pre rc)

# Parses a version: <out>_OK, <out>_BASE (X.Y.Z), <out>_MAJOR, <out>_KIND ("" for a release, a pre-release kind or
# "draft"), <out>_NUMBER, <out>_ORDER (the index of a pre-release kind).
function(release_parse_version text out)
    if ("${text}" MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(-(alpha|beta|pre|rc|draft)([0-9]+))?$")
        set(${out}_OK TRUE PARENT_SCOPE)
        set(${out}_BASE "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}" PARENT_SCOPE)
        set(${out}_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${out}_KIND "${CMAKE_MATCH_5}" PARENT_SCOPE)
        set(number "${CMAKE_MATCH_6}")
        list(FIND RELEASE_PRE_RELEASE_ORDER "${CMAKE_MATCH_5}" order)
        set(${out}_ORDER "${order}" PARENT_SCOPE)

        if (number STREQUAL "")
            set(number 0)
        endif ()

        set(${out}_NUMBER "${number}" PARENT_SCOPE)
    else ()
        set(${out}_OK FALSE PARENT_SCOPE)
    endif ()
endfunction()

# <out> is TRUE if the pre-release a is earlier than the pre-release b (both parsed).
function(release_pre_release_less a b out)
    if (${a}_ORDER LESS ${b}_ORDER OR (${a}_ORDER EQUAL ${b}_ORDER AND ${a}_NUMBER LESS ${b}_NUMBER))
        set(${out} TRUE PARENT_SCOPE)
    else ()
        set(${out} FALSE PARENT_SCOPE)
    endif ()
endfunction()

# The first "## <word>" heading of the text: <out>_FOUND, <out>_START (the position of the line), <out>_END (the
# position after its line break), <out>_NAME.
function(release_first_heading text out)
    string(REGEX MATCH "(^|\n)## [^ \t\r\n]+[ \t\r]*(\n|$)" match "${text}")

    if (match STREQUAL "")
        set(${out}_FOUND FALSE PARENT_SCOPE)
        return()
    endif ()

    string(FIND "${text}" "${match}" start)
    string(LENGTH "${match}" length)
    math(EXPR end "${start} + ${length}")

    if (match MATCHES "^\n")
        math(EXPR start "${start} + 1")
    endif ()

    string(STRIP "${match}" line)
    string(SUBSTRING "${line}" 3 -1 name)
    set(${out}_FOUND TRUE PARENT_SCOPE)
    set(${out}_START "${start}" PARENT_SCOPE)
    set(${out}_END "${end}" PARENT_SCOPE)
    set(${out}_NAME "${name}" PARENT_SCOPE)
endfunction()

# ReleaseNotes.md with the heading of the version (see HOW_TO_RELEASE.md; a draft changes nothing): <out_text>,
# <out_action>, or <out_error>. The same as update_notes of release.py.
function(release_update_notes text version out_text out_action out_error)
    set(${out_error} "" PARENT_SCOPE)
    release_parse_version("${version}" v)

    if (v_KIND STREQUAL "draft")
        set(${out_text} "${text}" PARENT_SCOPE)
        set(${out_action} "unchanged (a draft)" PARENT_SCOPE)
        return()
    endif ()

    release_first_heading("${text}" heading)
    set(replaces FALSE)
    set(unreleased "${text}")

    if (heading_FOUND)
        string(SUBSTRING "${text}" 0 ${heading_START} unreleased)
        release_parse_version("${heading_NAME}" f)

        if (NOT f_OK)
            set(${out_error} "${RELEASE_NOTES}: the heading '## ${heading_NAME}' is not a version" PARENT_SCOPE)
            return()
        endif ()

        if (f_BASE VERSION_EQUAL v_BASE AND NOT f_ORDER EQUAL -1)
            if (v_KIND STREQUAL "")
                set(replaces TRUE)
            else ()
                release_pre_release_less(f v replaces)
            endif ()
        endif ()

        if (f_BASE VERSION_GREATER v_BASE OR (f_BASE VERSION_EQUAL v_BASE AND NOT replaces))
            set(${out_error} "${RELEASE_NOTES} already has ${heading_NAME}: ${version} cannot follow it" PARENT_SCOPE)
            return()
        endif ()
    endif ()

    if (NOT replaces AND NOT unreleased MATCHES "[^ \t\r\n]")
        set(message "${RELEASE_NOTES} has no unreleased items at the top")

        if (heading_FOUND)
            string(APPEND message " (the first heading is ${heading_NAME})")
        endif ()

        set(${out_error} "${message}" PARENT_SCOPE)
        return()
    endif ()

    if (replaces)
        string(SUBSTRING "${text}" ${heading_END} -1 after)

        if (after MATCHES "^[ \t\r]*\n")
            string(REGEX REPLACE "^[ \t\r]*\n" "" after "${after}") # the empty line after the removed heading
        endif ()

        set(text "${unreleased}${after}")
        set(action "## ${heading_NAME} replaced by ## ${version}")
    else ()
        set(action "## ${version} inserted above the unreleased items")
    endif ()

    string(REGEX REPLACE "^([ \t\r]*\n)+" "" text "${text}")
    set(${out_text} "## ${version}\n\n${text}" PARENT_SCOPE)
    set(${out_action} "${action}" PARENT_SCOPE)
endfunction()

# The notes of the version for its GitHub release (a draft: the unreleased items at the top): <out> or <out_error>.
function(release_notes_of text version out out_error)
    set(${out_error} "" PARENT_SCOPE)
    release_parse_version("${version}" v)

    if (v_KIND STREQUAL "draft")
        release_first_heading("${text}" heading)
        set(section "${text}")

        if (heading_FOUND)
            string(SUBSTRING "${text}" 0 ${heading_START} section)
        endif ()
    else ()
        string(REPLACE "." "\\." escaped "${version}")
        string(REGEX MATCH "(^|\n)## ${escaped}[ \t\r]*(\n|$)" match "${text}")

        if (match STREQUAL "")
            set(${out_error} "${RELEASE_NOTES} has no heading '## ${version}'" PARENT_SCOPE)
            return()
        endif ()

        string(FIND "${text}" "${match}" start)
        string(LENGTH "${match}" length)
        math(EXPR start "${start} + ${length}")
        string(SUBSTRING "${text}" ${start} -1 section)
        release_first_heading("${section}" next)

        if (next_FOUND)
            string(SUBSTRING "${section}" 0 ${next_START} section)
        endif ()
    endif ()

    string(STRIP "${section}" section)
    set(${out} "${section}\n" PARENT_SCOPE)
endfunction()

# Sets one place of the version: <text_var> is updated, <out_message> or <out_error>.
function(release_set_place text_var path name pattern replacement new out_message out_error)
    set(${out_error} "" PARENT_SCOPE)
    set(${out_message} "" PARENT_SCOPE)
    set(text "${${text_var}}")
    string(REGEX MATCHALL "${pattern}" matches "${text}")
    list(LENGTH matches count)

    if (NOT count EQUAL 1)
        set(${out_error} "${path}: expected one place of the version, found ${count}" PARENT_SCOPE)
        return()
    endif ()

    string(REGEX MATCH "${pattern}" match "${text}")
    set(old "${CMAKE_MATCH_2}")
    string(REGEX REPLACE "${pattern}" "${replacement}" text "${text}")
    set(${text_var} "${text}" PARENT_SCOPE)

    if (old STREQUAL new)
        set(${out_message} "${path}: ${name} ${old} (unchanged)" PARENT_SCOPE)
    else ()
        set(${out_message} "${path}: ${name} ${old} -> ${new}" PARENT_SCOPE)
    endif ()
endfunction()

# Sets the version in the texts of CMakeLists.txt, docs/Doxyfile and Conf.hpp (the variables cmake_text,
# doxyfile_text, conf_text of the caller): <out_messages> (a list) or <out_error>.
function(release_set_versions version out_messages out_error)
    release_parse_version("${version}" v)
    set(${out_error} "" PARENT_SCOPE)
    set(messages "")

    foreach (place IN ITEMS cmake doxyfile conf)
        if (place STREQUAL "cmake")
            set(args "${RELEASE_CMAKE}" DXFCXX_VERSION "(set\\(DXFCXX_VERSION \")(v[^\"]*)(\")" "\\1${version}\\3"
                "${version}")
        elseif (place STREQUAL "doxyfile")
            set(args "${RELEASE_DOXYFILE}" PROJECT_NUMBER "(\nPROJECT_NUMBER[ \t]*=[ \t]*)([^ \t\r\n]+)()"
                "\\1${version}" "${version}")
        else ()
            set(args "${RELEASE_CONF}" "inline namespace" "(inline namespace )(v[0-9]+)( {)" "\\1v${v_MAJOR}\\3"
                "v${v_MAJOR}")
        endif ()

        set(text "${${place}_text}")
        set(error "")
        release_set_place(text ${args} message error)

        if (NOT error STREQUAL "")
            set(${out_error} "${error}" PARENT_SCOPE)
            return()
        endif ()

        set(${place}_text "${text}" PARENT_SCOPE)
        list(APPEND messages "${message}")
    endforeach ()

    set(${out_messages} "${messages}" PARENT_SCOPE)
endfunction()

function(release_print text)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${text}")
endfunction()

function(release_fail text)
    message(FATAL_ERROR "release.cmake: ${text}")
endfunction()

# Runs git in the root: <out> (stripped stdout); a failure stops the script unless ALLOW_FAILURE is given.
function(release_git out)
    cmake_parse_arguments(PARSE_ARGV 1 arg "ALLOW_FAILURE" "" "")
    execute_process(COMMAND git ${arg_UNPARSED_ARGUMENTS} WORKING_DIRECTORY "${RELEASE_ROOT}" RESULT_VARIABLE result
                    OUTPUT_VARIABLE output ERROR_VARIABLE error OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (NOT result EQUAL 0 AND NOT arg_ALLOW_FAILURE)
        release_fail("git ${arg_UNPARSED_ARGUMENTS}: ${error}")
    endif ()

    set(${out} "${output}" PARENT_SCOPE)
endfunction()

# Reads a file of the root: <var> (file(READ) drops the CR of CRLF on all the platforms), <var>_CRLF (detected in the
# bytes: "0d0a" at a byte boundary).
function(release_read path var)
    file(READ "${RELEASE_ROOT}/${path}" text)
    file(READ "${RELEASE_ROOT}/${path}" hex LIMIT 4096 HEX)
    string(REPLACE "\r\n" "\n" text "${text}")
    set(${var} "${text}" PARENT_SCOPE)
    set(${var}_CRLF FALSE PARENT_SCOPE)

    # No regular expression here: the recursive regex engine of CMake overflows the stack on a long hex string.
    set(offset 0)
    string(FIND "${hex}" "0d0a" position)

    while (NOT position EQUAL -1)
        math(EXPR odd "(${offset} + ${position}) % 2")

        if (odd EQUAL 0) # at a byte boundary
            set(${var}_CRLF TRUE PARENT_SCOPE)
            return()
        endif ()

        math(EXPR skip "${position} + 1")
        math(EXPR offset "${offset} + ${skip}")
        string(SUBSTRING "${hex}" ${skip} -1 hex)
        string(FIND "${hex}" "0d0a" position)
    endwhile ()
endfunction()

# Writes the text with the given line ends: file(WRITE) adds CR on Windows only, file(CONFIGURE) has NEWLINE_STYLE.
# @ONLY: no ${} substitution, and every "@" is escaped as @AT@ (substituted back).
function(release_write_file file text crlf)
    if (text MATCHES "#cmakedefine")
        release_fail("${file}: #cmakedefine cannot be written by file(CONFIGURE)")
    endif ()

    set(AT "@")
    string(REPLACE "@" "@AT@" text "${text}")

    if (crlf)
        set(style CRLF)
    else ()
        set(style UNIX)
    endif ()

    file(CONFIGURE OUTPUT "${file}" CONTENT "${text}" @ONLY NEWLINE_STYLE ${style})
endfunction()

function(release_write path text crlf)
    release_write_file("${RELEASE_ROOT}/${path}" "${text}" ${crlf})
endfunction()

function(release_push)
    foreach (ref IN LISTS ARGN)
        release_print("git push origin ${ref}")
        release_git(ignored push origin "${ref}")
    endforeach ()
endfunction()

function(release_prepare version dry_run push)
    release_parse_version("${version}" v)
    release_git(local tag --list "${version}")
    release_git(remote ls-remote --tags origin "refs/tags/${version}" ALLOW_FAILURE)

    if (NOT local STREQUAL "" OR NOT remote STREQUAL "")
        release_fail("the tag ${version} already exists")
    endif ()

    if (v_KIND STREQUAL "draft")
        release_git(head rev-parse --short HEAD)
        release_print("${version}: a trial run of release.yml, no commit, only the tag on ${head}")

        if (dry_run)
            release_print("(dry run: no tag ${version})")
            return()
        endif ()

        release_git(ignored tag "${version}")
        release_print("Tagged ${version}")

        if (push)
            release_push("${version}")
        else ()
            release_print("Push it: git push origin ${version}")
        endif ()

        release_print("Remove it afterwards: release.cmake ${version} --cleanup")
        return()
    endif ()

    if (NOT dry_run)
        release_git(status status --porcelain)

        if (NOT status STREQUAL "")
            release_fail("the working tree has changes: commit or stash them first")
        endif ()
    endif ()

    release_read("${RELEASE_NOTES}" notes_text)
    release_read("${RELEASE_CMAKE}" cmake_text)
    release_read("${RELEASE_DOXYFILE}" doxyfile_text)
    release_read("${RELEASE_CONF}" conf_text)
    release_set_versions("${version}" messages error)

    if (NOT error STREQUAL "")
        release_fail("${error}")
    endif ()

    release_update_notes("${notes_text}" "${version}" new_notes action error)

    if (NOT error STREQUAL "")
        release_fail("${error}")
    endif ()

    release_print("${RELEASE_NOTES}: ${action}")

    foreach (message IN LISTS messages)
        release_print("${message}")
    endforeach ()

    if (dry_run)
        release_print("(dry run: no files changed, no commit ${version}, no tag ${version})")
        return()
    endif ()

    release_write("${RELEASE_NOTES}" "${new_notes}" ${notes_text_CRLF})
    release_write("${RELEASE_CMAKE}" "${cmake_text}" ${cmake_text_CRLF})
    release_write("${RELEASE_DOXYFILE}" "${doxyfile_text}" ${doxyfile_text_CRLF})
    release_write("${RELEASE_CONF}" "${conf_text}" ${conf_text_CRLF})
    release_git(ignored add "${RELEASE_NOTES}" "${RELEASE_CMAKE}" "${RELEASE_DOXYFILE}" "${RELEASE_CONF}")
    release_git(ignored commit -q -m "${version}")
    release_git(ignored tag "${version}")
    release_git(branch rev-parse --abbrev-ref HEAD)
    release_print("Committed and tagged ${version}")

    if (push)
        release_push("${branch}" "${version}")
    else ()
        release_print("Push them: git push origin ${branch} && git push origin ${version}")
    endif ()
endfunction()

function(release_main)
    set(version "")
    set(mode "")
    set(ref "")
    math(EXPR last "${CMAKE_ARGC} - 1")
    set(expect_ref FALSE)
    set(usage "usage: cmake -P scripts/release.cmake <version>")
    string(APPEND usage " [--push | --dry-run | --notes [--ref <ref>] | --cleanup]")

    if (last LESS 3)
        release_fail("${usage}")
    endif ()

    foreach (index RANGE 3 ${last})
        set(arg "${CMAKE_ARGV${index}}")

        if (expect_ref)
            set(ref "${arg}")
            set(expect_ref FALSE)
        elseif (arg STREQUAL "--")
            continue()
        elseif (arg STREQUAL "--ref")
            set(expect_ref TRUE)
        elseif (arg MATCHES "^--(push|dry-run|notes|cleanup)$")
            if (NOT mode STREQUAL "")
                release_fail("${mode} and ${arg} cannot be used together")
            endif ()

            set(mode "${arg}")
        elseif (version STREQUAL "")
            set(version "${arg}")
        else ()
            release_fail("unexpected argument ${arg}")
        endif ()
    endforeach ()

    if (version STREQUAL "")
        release_fail("${usage}")
    endif ()

    release_parse_version("${version}" v)

    if (NOT v_OK)
        release_fail("'${version}' is not a version: vX.Y.Z or vX.Y.Z-<alpha|beta|pre|rc|draft><N>")
    endif ()

    if (mode STREQUAL "--notes")
        if (ref STREQUAL "")
            release_read("${RELEASE_NOTES}" text)
        else ()
            release_git(text show "${ref}:${RELEASE_NOTES}")
            string(REPLACE "\r\n" "\n" text "${text}")
        endif ()

        release_notes_of("${text}" "${version}" notes error)

        if (NOT error STREQUAL "")
            release_fail("${error}")
        endif ()

        # Exactly the notes on stdout (message() writes to stderr): through a file and cmake -E cat.
        string(RANDOM LENGTH 12 suffix)
        set(file "${RELEASE_ROOT}/.release-notes-${suffix}.tmp")
        release_write_file("${file}" "${notes}" FALSE)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E cat "${file}")
        file(REMOVE "${file}")
    elseif (mode STREQUAL "--cleanup")
        if (NOT v_KIND STREQUAL "draft")
            release_fail("--cleanup removes only drafts, not ${version}")
        endif ()

        execute_process(COMMAND gh release delete "${version}" --cleanup-tag --yes WORKING_DIRECTORY "${RELEASE_ROOT}")
        release_git(ignored tag -d "${version}" ALLOW_FAILURE)
        release_print("Removed the draft release and the tag ${version} (if they existed)")
    else ()
        set(dry_run FALSE)
        set(push FALSE)

        if (mode STREQUAL "--dry-run")
            set(dry_run TRUE)
        elseif (mode STREQUAL "--push")
            set(push TRUE)
        endif ()

        release_prepare("${version}" ${dry_run} ${push})
    endif ()
endfunction()

if (NOT RELEASE_NO_MAIN)
    release_main()
endif ()
