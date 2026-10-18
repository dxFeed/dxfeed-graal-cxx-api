# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
# Finalize portable example links and recognized Doxygen 1.18 index defects.
if(NOT DEFINED HTML_DIR OR NOT IS_DIRECTORY "${HTML_DIR}")
  message(FATAL_ERROR "HTML_DIR must name the generated Doxygen HTML directory")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/FinalizeDiagrams.cmake")

# GitHub links are relative to docs/*.md; generated HTML has a flat layout.
# Ship the exact referenced sources inside the HTML tree so an offline copy
# works without the repository or a hard-coded GitHub branch URL.
set(example_sources
  tests/support/gallery_before.hpp
  examples/gallery/form.hpp
  examples/dashboard/dashboard.hpp
  tests/package_consumer/form_declarative.cpp
  tests/package_consumer/composed_before.cpp
  tests/package_consumer/composed_after.cpp
  tests/package_consumer/composed_named.cpp
  tests/package_consumer/composed_support.hpp
  tests/package_consumer/form_before.cpp
  tests/package_consumer/form_after.cpp
  tests/package_consumer/form_support.hpp
  tests/package_consumer/form_replay.hpp
  tests/package_consumer/virtual_before.cpp
  tests/package_consumer/virtual_after.cpp
  tests/package_consumer/virtual_managed.cpp
  tests/package_consumer/virtual_scene.hpp
  tests/package_consumer/virtual_support.hpp
  tests/package_consumer/virtual_lifecycle.cpp)
foreach(source IN LISTS example_sources)
  get_filename_component(directory "${source}" DIRECTORY)
  file(MAKE_DIRECTORY "${HTML_DIR}/source/${directory}")
  configure_file("${CMAKE_CURRENT_LIST_DIR}/../${source}" "${HTML_DIR}/source/${source}" COPYONLY)
endforeach()

foreach(page examples managed-controls declarative-ui virtual-preparation)
  set(page_file "${HTML_DIR}/${page}.html")
  file(READ "${page_file}" content)
  foreach(source IN LISTS example_sources)
    string(REPLACE "href=\"../${source}\"" "href=\"source/${source}\"" content "${content}")
  endforeach()
  file(WRITE "${page_file}" "${content}")
endforeach()

file(GLOB index_files "${HTML_DIR}/functions*.html")
foreach(index_file IN LISTS index_files)
  file(READ "${index_file}" original)
  set(content "${original}")

  # Doxygen encodes the destructor anchor but leaves its incoming links unencoded.
  string(REGEX MATCHALL "href=\"(functions[^\"]*\\.html)?#index_~\"" destructor_links "${content}")
  foreach(link IN LISTS destructor_links)
    string(REGEX REPLACE "^href=\"([^#]*)#index_~\"$" "\\1" target "${link}")
    if(target STREQUAL "")
      set(target_file "${index_file}")
    else()
      set(target_file "${HTML_DIR}/${target}")
    endif()
    if(EXISTS "${target_file}")
      file(READ "${target_file}" target_content)
      if(target_content MATCHES "(id|name)=\"index__7E\"")
        string(REPLACE "${link}" "href=\"${target}#index__7E\"" content "${content}")
      else()
        message(FATAL_ERROR "Unrecognized destructor anchor in ${target_file}; left unchanged")
      endif()
    else()
      message(FATAL_ERROR "Missing destructor index target ${target_file}; left unchanged")
    endif()
  endforeach()

  # Short member lists omit letter headings. Attach the missing local anchor to
  # the first actual member row, preserving the complete index and its links.
  string(REGEX MATCHALL "href=\"#index_[a-z]\"" letter_links "${content}")
  foreach(link IN LISTS letter_links)
    string(REGEX REPLACE "^href=\"#index_([a-z])\"$" "\\1" letter "${link}")
    if(content MATCHES "(id|name)=\"index_${letter}\"")
      continue()
    endif()

    if(NOT content MATCHES "<div class=\"qindex\">[^\n]*</div>[\r\n]+<ul>")
      message(FATAL_ERROR "Unrecognized flat member index in ${index_file}; left unchanged")
    endif()
    string(TOUPPER "${letter}" upper_letter)
    string(REGEX MATCH "<li>[${letter}${upper_letter}][A-Za-z0-9_]*&#160;:" row "${content}")
    if(row STREQUAL "")
      message(FATAL_ERROR "Missing member row for index_${letter} in ${index_file}; left unchanged")
    endif()
    # Replace only the first matching row prefix, not every symbol in that group.
    string(FIND "${content}" "${row}" position)
    string(SUBSTRING "${content}" 0 ${position} before)
    string(SUBSTRING "${content}" ${position} -1 after)
    string(REGEX REPLACE "^<li>" "<li id=\"index_${letter}\">" after "${after}")
    set(content "${before}${after}")
  endforeach()

  # Do not rewrite already-finalized files: repeated execution preserves bytes.
  if(NOT content STREQUAL original)
    file(WRITE "${index_file}" "${content}")
  endif()
endforeach()
