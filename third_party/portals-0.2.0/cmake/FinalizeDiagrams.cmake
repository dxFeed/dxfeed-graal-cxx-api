# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0

# Doxygen 1.18 emits SVG objects without intrinsic dimensions. Use responsive
# images at the generated diagram's own width instead of the browser's default
# object viewport. The SVG remains a portable, independently viewable asset.
file(GLOB diagram_pages "${HTML_DIR}/*.html")
foreach(page IN LISTS diagram_pages)
  file(READ "${page}" original)
  set(content "${original}")
  string(REGEX MATCHALL
    "<object type=\"image/svg\\+xml\" data=\"inline_mermaid_[A-Za-z0-9_-]+\\.svg\"></object>"
    diagrams "${content}")
  foreach(diagram IN LISTS diagrams)
    string(REGEX REPLACE ".*data=\"([^\"]+)\".*" "\\1" image "${diagram}")
    file(READ "${HTML_DIR}/${image}" svg)
    if(NOT svg MATCHES "viewBox=\"[-0-9.]+ [-0-9.]+ ([0-9.]+) ([0-9.]+)\"")
      message(FATAL_ERROR "Missing diagram dimensions in ${image}")
    endif()
    set(width "${CMAKE_MATCH_1}")
    if(NOT svg MATCHES "<title[^>]*>([^<]+)</title>")
      message(FATAL_ERROR "Missing Mermaid accTitle in ${image}")
    endif()
    set(caption "${CMAKE_MATCH_1}")
    string(REPLACE "\"" "&quot;" caption "${caption}")
    string(REPLACE "${diagram}"
      "<img src=\"${image}\" alt=\"${caption}\" style=\"width: ${width}px;\"/>"
      content "${content}")
  endforeach()
  if(NOT content STREQUAL original)
    file(WRITE "${page}" "${content}")
  endif()
endforeach()
