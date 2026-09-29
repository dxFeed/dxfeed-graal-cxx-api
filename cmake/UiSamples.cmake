# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Decides whether the UI samples (Dear ImGui, GLFW, OpenGL) are built, by DXFCXX_BUILD_UI_SAMPLES:
#  - AUTO: when their system dependencies are found (always on Windows and macOS); otherwise one status line with the
#    packages to install;
#  - ON: always; the configuration fails when the dependencies are missing;
#  - OFF: never.
# Sets <result_var> to ON or OFF.
function(dxfcxx_check_ui_samples result_var)
    string(TOUPPER "${DXFCXX_BUILD_UI_SAMPLES}" mode)

    if (mode MATCHES "^(ON|YES|TRUE|Y|1)$")
        set(mode ON)
    elseif (mode MATCHES "^(OFF|NO|FALSE|N|0)$")
        set(${result_var} OFF PARENT_SCOPE)

        return()
    elseif (NOT mode STREQUAL "AUTO")
        message(FATAL_ERROR "DXFCXX_BUILD_UI_SAMPLES must be AUTO, ON or OFF, not '${DXFCXX_BUILD_UI_SAMPLES}'")
    endif ()

    # GLFW uses the native window system on Windows and macOS; on other systems it is built with the X11 backend.
    if (WIN32 OR APPLE)
        set(${result_var} ON PARENT_SCOPE)

        return()
    endif ()

    # The X11 headers that GLFW requires, and the Debian/Ubuntu packages that provide them.
    set(x11_packages
            X11 libx11-dev
            Xkb libx11-dev
            Xrandr libxrandr-dev
            Xinerama libxinerama-dev
            Xcursor libxcursor-dev
            Xi libxi-dev
            Xshape libxext-dev)
    set(missing "")
    set(packages "")

    find_package(X11 QUIET)

    while (x11_packages)
        list(POP_FRONT x11_packages component package)

        if (NOT X11_${component}_INCLUDE_PATH)
            list(APPEND missing ${component})
            list(APPEND packages ${package})
        endif ()
    endwhile ()

    find_package(OpenGL QUIET)

    if (NOT TARGET OpenGL::GL)
        list(APPEND missing OpenGL)
        list(APPEND packages libgl1-mesa-dev)
    endif ()

    if (NOT missing)
        set(${result_var} ON PARENT_SCOPE)

        return()
    endif ()

    list(REMOVE_DUPLICATES packages)
    list(JOIN missing ", " missing_text)
    list(JOIN packages " " packages_text)
    set(text "need ${missing_text} (Debian/Ubuntu: ${packages_text})")

    if (mode STREQUAL "ON")
        message(FATAL_ERROR "DXFCXX_BUILD_UI_SAMPLES is ON, but the UI samples ${text}")
    endif ()

    message(STATUS "Skipping the UI samples, they ${text}; set DXFCXX_BUILD_UI_SAMPLES=OFF to silence this")
    set(${result_var} OFF PARENT_SCOPE)
endfunction()
