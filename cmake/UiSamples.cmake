# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Decides whether the UI samples (Dear ImGui, GLFW, OpenGL) are built, by DXFCXX_BUILD_UI_SAMPLES:
#  - AUTO: when their system dependencies are found (always on Windows and macOS); otherwise one status line with the
#    packages to install;
#  - ON: always; the configuration fails when the dependencies are missing;
#  - OFF: never.
# On Linux and other systems with X11 or Wayland, GLFW is built with every window system backend whose development
# files are found (at least one is needed) and selects one at run time (XDG_SESSION_TYPE, then the first that
# connects). DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND=ON requires the Wayland backend.
# Sets <result_var> to ON or OFF, and DXFCXX_UI_SAMPLES_GLFW_X11 and DXFCXX_UI_SAMPLES_GLFW_WAYLAND to the backends.
function(dxfcxx_check_ui_samples result_var)
    string(TOUPPER "${DXFCXX_BUILD_UI_SAMPLES}" mode)

    set(DXFCXX_UI_SAMPLES_GLFW_X11 OFF PARENT_SCOPE)
    set(DXFCXX_UI_SAMPLES_GLFW_WAYLAND OFF PARENT_SCOPE)

    if (mode MATCHES "^(ON|YES|TRUE|Y|1)$")
        set(mode ON)
    elseif (mode MATCHES "^(OFF|NO|FALSE|N|0)$")
        set(${result_var} OFF PARENT_SCOPE)

        return()
    elseif (NOT mode STREQUAL "AUTO")
        message(FATAL_ERROR "DXFCXX_BUILD_UI_SAMPLES must be AUTO, ON or OFF, not '${DXFCXX_BUILD_UI_SAMPLES}'")
    endif ()

    # GLFW uses the native window system on Windows and macOS.
    if (WIN32 OR APPLE)
        set(${result_var} ON PARENT_SCOPE)

        return()
    endif ()

    # X11: the headers that GLFW requires, and the Debian/Ubuntu packages that provide them.
    set(x11_headers
            X11 libx11-dev
            Xkb libx11-dev
            Xrandr libxrandr-dev
            Xinerama libxinerama-dev
            Xcursor libxcursor-dev
            Xi libxi-dev
            Xshape libxext-dev)
    set(x11_missing "")
    set(x11_packages "")

    find_package(X11 QUIET)

    while (x11_headers)
        list(POP_FRONT x11_headers component package)

        if (NOT X11_${component}_INCLUDE_PATH)
            list(APPEND x11_missing ${component})
        endif ()

        list(APPEND x11_packages ${package})
    endwhile ()

    # Wayland: wayland-scanner and the pkg-config modules that GLFW requires (it has its own protocol files).
    set(wayland_missing "")
    set(wayland_packages libwayland-dev libxkbcommon-dev pkg-config)

    find_program(DXFCXX_WAYLAND_SCANNER wayland-scanner)
    find_package(PkgConfig QUIET)

    if (NOT DXFCXX_WAYLAND_SCANNER)
        list(APPEND wayland_missing wayland-scanner)
    endif ()

    if (PKG_CONFIG_FOUND)
        pkg_check_modules(DXFCXX_UI_WAYLAND QUIET
                wayland-client>=0.2.7 wayland-cursor>=0.2.7 wayland-egl>=0.2.7 xkbcommon>=0.5.0)

        if (NOT DXFCXX_UI_WAYLAND_FOUND)
            list(APPEND wayland_missing wayland-client/cursor/egl xkbcommon)
        endif ()
    else ()
        list(APPEND wayland_missing pkg-config)
    endif ()

    find_package(OpenGL QUIET)

    set(x11 OFF)
    set(wayland OFF)

    if (NOT x11_missing)
        set(x11 ON)
    endif ()

    if (NOT wayland_missing)
        set(wayland ON)
    endif ()

    if (DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND AND NOT wayland)
        list(JOIN wayland_missing ", " wayland_missing_text)
        list(JOIN wayland_packages " " wayland_packages_text)
        message(FATAL_ERROR "DXFCXX_SAMPLE_GLFW_BUILD_WAYLAND is ON, but ${wayland_missing_text} not found "
                "(Debian/Ubuntu: ${wayland_packages_text})")
    endif ()

    if ((x11 OR wayland) AND TARGET OpenGL::GL)
        set(DXFCXX_UI_SAMPLES_GLFW_X11 ${x11} PARENT_SCOPE)
        set(DXFCXX_UI_SAMPLES_GLFW_WAYLAND ${wayland} PARENT_SCOPE)
        set(${result_var} ON PARENT_SCOPE)

        return()
    endif ()

    # What is missing, with the Debian/Ubuntu packages.
    set(needs "")
    set(packages "")

    if (NOT TARGET OpenGL::GL)
        list(APPEND needs "OpenGL")
        list(APPEND packages "OpenGL: libgl1-mesa-dev")
    endif ()

    if (NOT x11 AND NOT wayland)
        list(REMOVE_DUPLICATES x11_packages)
        list(JOIN x11_packages " " x11_packages_text)
        list(JOIN wayland_packages " " wayland_packages_text)
        list(APPEND needs "X11 or Wayland")
        list(APPEND packages "X11: ${x11_packages_text}" "Wayland: ${wayland_packages_text}")
    endif ()

    list(JOIN needs " and " needs_text)
    list(JOIN packages "; " packages_text)
    set(text "need the ${needs_text} development files (Debian/Ubuntu: ${packages_text})")

    if (mode STREQUAL "ON")
        message(FATAL_ERROR "DXFCXX_BUILD_UI_SAMPLES is ON, but the UI samples ${text}")
    endif ()

    message(STATUS "Skipping the UI samples, they ${text}; set DXFCXX_BUILD_UI_SAMPLES=OFF to silence this")
    set(${result_var} OFF PARENT_SCOPE)
endfunction()
