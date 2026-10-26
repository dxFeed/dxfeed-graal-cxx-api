# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Tests of scripts/release.cmake (the same cases as test_release.py): cmake -P scripts/tests/test_release.cmake
# The whole release in a git repository and the agreement with release.py are tested by test_release.py.

cmake_minimum_required(VERSION 3.21)

set(RELEASE_NO_MAIN TRUE)
include("${CMAKE_CURRENT_LIST_DIR}/../release.cmake")

set(failures 0)
set(ITEMS "* New item.\n    * Detail; with a semicolon.\n")
set(OLD "## v8.1.0\n\n* Old item.\n")

function(check name actual expected)
    if (NOT actual STREQUAL expected)
        message(NOTICE "FAILED: ${name}\n--- expected:\n${expected}\n--- actual:\n${actual}\n---")
        math(EXPR failures "${failures} + 1")
        set(failures "${failures}" PARENT_SCOPE)
    endif ()
endfunction()

function(check_update name text version expected)
    release_update_notes("${text}" "${version}" result action error)
    check("${name} (error)" "${error}" "")
    check("${name}" "${result}" "${expected}")
    set(failures "${failures}" PARENT_SCOPE)
endfunction()

function(check_update_error name text version expected)
    release_update_notes("${text}" "${version}" result action error)

    if (NOT error MATCHES "${expected}")
        check("${name}: the error" "${error}" "(matching) ${expected}")
    endif ()

    set(failures "${failures}" PARENT_SCOPE)
endfunction()

# Versions
release_parse_version("v10.0.1-beta3" v)
check("parse" "${v_OK}|${v_BASE}|${v_MAJOR}|${v_KIND}|${v_NUMBER}|${v_ORDER}" "TRUE|10.0.1|10|beta|3|1")

foreach (text IN ITEMS "8.2.0" "v8.2" "v8.2.0-rc" "v8.2.0-gamma1" "v8.2.0-RC1")
    release_parse_version("${text}" bad)
    check("not a version: ${text}" "${bad_OK}" "FALSE")
endforeach ()

set(previous "")

foreach (text IN ITEMS alpha1 alpha2 beta1 pre1 rc1 rc10)
    if (NOT previous STREQUAL "")
        release_parse_version("v8.2.0-${previous}" a)
        release_parse_version("v8.2.0-${text}" b)
        release_pre_release_less(a b less)
        check("${previous} < ${text}" "${less}" "TRUE")
    endif ()

    set(previous "${text}")
endforeach ()

# The heading of ReleaseNotes.md
check_update("rc1 over unreleased items" "${ITEMS}\n${OLD}" v8.2.0-rc1 "## v8.2.0-rc1\n\n${ITEMS}\n${OLD}")
check_update("rc2 after rc1" "## v8.2.0-rc1\n\n${ITEMS}\n${OLD}" v8.2.0-rc2 "## v8.2.0-rc2\n\n${ITEMS}\n${OLD}")
check_update("rc1 after beta3" "## v8.2.0-beta3\n\n${ITEMS}\n${OLD}" v8.2.0-rc1 "## v8.2.0-rc1\n\n${ITEMS}\n${OLD}")
check_update("release after rc3" "## v8.2.0-rc3\n\n${ITEMS}\n${OLD}" v8.2.0 "## v8.2.0\n\n${ITEMS}\n${OLD}")
check_update("release over unreleased items" "${ITEMS}\n${OLD}" v8.2.0 "## v8.2.0\n\n${ITEMS}\n${OLD}")
set(added "* Added after rc1.\n\n## v8.2.0-rc1\n\n${ITEMS}\n${OLD}")
check_update("rc2 joins the items added after rc1" "${added}" v8.2.0-rc2
             "## v8.2.0-rc2\n\n* Added after rc1.\n\n${ITEMS}\n${OLD}")
check_update("release joins the items added after rc1" "${added}" v8.2.0
             "## v8.2.0\n\n* Added after rc1.\n\n${ITEMS}\n${OLD}")

foreach (text IN ITEMS "${ITEMS}\n${OLD}" "## v8.2.0-rc1\n\n${ITEMS}\n${OLD}" "${OLD}")
    check_update("a draft changes nothing" "${text}" v8.2.0-draft1 "${text}")
endforeach ()

check_update_error("an earlier pre-release" "## v8.2.0-rc2\n\n${ITEMS}\n${OLD}" v8.2.0-rc1
                   "already has v8.2.0-rc2: v8.2.0-rc1 cannot follow it")
check_update_error("the same pre-release" "## v8.2.0-rc2\n\n${ITEMS}\n${OLD}" v8.2.0-rc2
                   "already has v8.2.0-rc2: v8.2.0-rc2 cannot follow it")
check_update_error("alpha after rc" "## v8.2.0-rc1\n\n${ITEMS}\n${OLD}" v8.2.0-alpha1
                   "already has v8.2.0-rc1: v8.2.0-alpha1 cannot follow it")
check_update_error("a released version" "## v8.2.0\n\n${ITEMS}\n${OLD}" v8.2.0
                   "already has v8.2.0: v8.2.0 cannot follow it")
check_update_error("a pre-release after its release" "## v8.2.0\n\n${ITEMS}\n${OLD}" v8.2.0-rc1
                   "already has v8.2.0: v8.2.0-rc1 cannot follow it")
check_update_error("no unreleased items" "${OLD}" v8.2.0
                   "has no unreleased items at the top")
check_update_error("an older version" "${ITEMS}\n${OLD}" v8.0.5
                   "already has v8.1.0: v8.0.5 cannot follow it")
check_update_error("a heading that is not a version" "${ITEMS}\n## Unreleased\n" v8.2.0
                   "the heading '## Unreleased'.* is not a version")

# The notes of a version
release_notes_of("## v8.2.0\n\n${ITEMS}\n${OLD}" v8.2.0 notes error)
check("notes of a release" "${notes}" "${ITEMS}")
release_notes_of("## v8.2.0\n\n${ITEMS}\n${OLD}" v8.1.0 notes error)
check("notes of the previous release" "${notes}" "* Old item.\n")
release_notes_of("${ITEMS}\n${OLD}" v8.2.0-draft1 notes error)
check("notes of a draft" "${notes}" "${ITEMS}")
release_notes_of("${ITEMS}\n${OLD}" v8.2.0 notes error)
check("no section: an error" "${error}" "ReleaseNotes.md has no heading '## v8.2.0'")

# The fragments release-notes/*.md (as release_read_fragments gives them, in any order)
set(fragments "MDAPI-9.md;graal.md;MDAPI-3.md;MDAPI-100.md;graal-sdk.md;MDAPI-20.md;MDAPI-12.md")
set(fragments_0 "* **\\[MDAPI-9]\\[C++]** Older.\n")
set(fragments_1 "* Without a ticket.\n")
set(fragments_2 "* **\\[MDAPI-3]\\[C++]** Changed; the nested items of a\n  breaking change:\n    * **\\[BREAKING]** Nested.\n")
set(fragments_3 "* **\\[MDAPI-100]\\[C++]** Newest.\n")
set(fragments_4 "* Migrated to Graal SDK v9.9.9.\n")
set(fragments_5 "\n\r\n* **\\[MDAPI-20]\\[C++]** Blank lines around,\r\n  CRLF.  \r\n\r\n")
set(fragments_6 "* **\\[MDAPI-12]\\[C++]** **\\[BREAKING]** Breaking.\n")
set(FRAGMENT_ITEMS "* **\\[MDAPI-12]\\[C++]** **\\[BREAKING]** Breaking.\n* **\\[MDAPI-100]\\[C++]** Newest.\n")
string(APPEND FRAGMENT_ITEMS "* **\\[MDAPI-20]\\[C++]** Blank lines around,\n  CRLF.\n* **\\[MDAPI-9]\\[C++]** Older.\n")
string(APPEND FRAGMENT_ITEMS "${fragments_2}* Migrated to Graal SDK v9.9.9.\n* Without a ticket.\n")

release_sort_fragments(fragments order)
check("the order of the fragments" "${order}" "6;3;5;0;2;4;1")
release_add_fragments("${ITEMS}\n${OLD}" fragments result error)
check("fragments after the unreleased items" "${result}" "${ITEMS}${FRAGMENT_ITEMS}\n${OLD}")

set(one "a.md")
set(one_0 "* A.\n")
release_add_fragments("${ITEMS}\n\n\n${OLD}" one result error)
check("blank lines before the heading" "${result}" "${ITEMS}* A.\n\n${OLD}")
release_add_fragments("## v8.2.0-rc1\n\n${ITEMS}\n${OLD}" one result error)
check("fragments above a pre-release" "${result}" "* A.\n\n## v8.2.0-rc1\n\n${ITEMS}\n${OLD}")
set(one_0 "* A.")
release_add_fragments("" one result error)
check("fragments without a heading" "${result}" "* A.\n")

set(none "")

foreach (text IN ITEMS "${ITEMS}\n${OLD}" "${ITEMS}\n\n\n${OLD}" "${OLD}" "")
    release_add_fragments("${text}" none result error)
    check("no fragments change nothing" "${result}" "${text}")
endforeach ()

set(empty "MDAPI-1.md")
set(empty_0 " \r\n\n")
release_add_fragments("${ITEMS}\n${OLD}" empty result error)
check("an empty fragment" "${error}" "release-notes/MDAPI-1.md is empty")

set(several "MDAPI-1.md")

foreach (case IN ITEMS "2|* A.\n* B.\n" "0|Text.\n" "1|Text.\n* A.\n")
    string(FIND "${case}" "|" bar)
    string(SUBSTRING "${case}" 0 ${bar} count)
    math(EXPR bar "${bar} + 1")
    string(SUBSTRING "${case}" ${bar} -1 several_0)
    release_add_fragments("${ITEMS}\n${OLD}" several result error)
    check("a fragment of ${count} items of the top level" "${error}"
          "release-notes/MDAPI-1.md has ${count} items of the top level instead of one (put the others in its nested items)")
endforeach ()

# The places of the version
set(cmake_text "set(DXFCXX_VERSION \"v8.1.0\" CACHE STRING \"The package version\")\n")
set(doxyfile_text "PROJECT_NAME = x\nPROJECT_NUMBER         = v8.1.0\n")
set(conf_text "#    define DXFCPP_BEGIN_NAMESPACE \\\n        namespace dxfcpp { \\\n        inline namespace v8 {\n")
release_set_versions(v9.0.0-rc1 messages error)
check("set versions (error)" "${error}" "")
check("CMakeLists.txt" "${cmake_text}" "set(DXFCXX_VERSION \"v9.0.0-rc1\" CACHE STRING \"The package version\")\n")
check("Doxyfile" "${doxyfile_text}" "PROJECT_NAME = x\nPROJECT_NUMBER         = v9.0.0-rc1\n")
check("Conf.hpp" "${conf_text}"
      "#    define DXFCPP_BEGIN_NAMESPACE \\\n        namespace dxfcpp { \\\n        inline namespace v9 {\n")
set(doxyfile_text "PROJECT_NAME = x\n")
release_set_versions(v8.2.0 messages error)
check("a missing place" "${error}" "docs/Doxyfile: expected one place of the version, found 0")

if (failures GREATER 0)
    message(FATAL_ERROR "test_release.cmake: ${failures} failed")
endif ()

message(NOTICE "test_release.cmake: OK")
