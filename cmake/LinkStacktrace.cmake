# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Sets <backendVariable> to the target of the Boost.Stacktrace library (a static library) and <librariesVariable> to
# the system libraries it needs.
function(dxfcxx_stacktrace_backend backendVariable librariesVariable)
    if (TARGET Boost::stacktrace_windbg)
        set(backend Boost::stacktrace_windbg)
        set(libraries ole32 dbgeng)
    elseif (TARGET Boost::stacktrace_backtrace)
        set(backend Boost::stacktrace_backtrace)
        set(libraries dl backtrace)
    elseif (TARGET Boost::stacktrace_addr2line)
        set(backend Boost::stacktrace_addr2line)
        set(libraries dl)
    elseif (TARGET Boost::stacktrace_basic)
        set(backend Boost::stacktrace_basic)
        set(libraries dl)
    else ()
        set(backend Boost::stacktrace_noop)
        set(libraries "")
    endif ()

    set(${backendVariable} ${backend} PARENT_SCOPE)
    set(${librariesVariable} ${libraries} PARENT_SCOPE)
endfunction()

# Links Boost.Stacktrace to the target. The CMake package of the static library links its own imported target of the
# library, dxfcxx::boost_stacktrace (cmake/dxFeedGraalCxxApiConfig.cmake.in), instead of the Boost target.
function(LinkStacktrace targetName)
    target_compile_definitions(${targetName} PRIVATE $<$<PLATFORM_ID:Darwin>:BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED>)

    dxfcxx_stacktrace_backend(backend libraries)
    target_link_libraries(${targetName} PRIVATE
            $<BUILD_INTERFACE:${backend}> $<INSTALL_INTERFACE:dxfcxx::boost_stacktrace> ${libraries})

    if (backend STREQUAL "Boost::stacktrace_backtrace" AND TARGET Boost::stacktrace_addr2line)
        target_compile_definitions(${targetName} PRIVATE BOOST_STACKTRACE_USE_ADDR2LINE)
    endif ()
endfunction()
