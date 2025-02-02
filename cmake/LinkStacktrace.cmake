# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

function(LinkStacktrace targetName)
    target_compile_definitions(${targetName} PRIVATE $<$<PLATFORM_ID:Darwin>:BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED>)

    if (TARGET Boost::stacktrace_windbg)
        target_link_libraries(${targetName} PUBLIC Boost::stacktrace_windbg ole32 dbgeng)
    elseif (TARGET Boost::stacktrace_backtrace)
        target_link_libraries(${targetName} PUBLIC Boost::stacktrace_backtrace dl backtrace)

        if (TARGET Boost::stacktrace_addr2line)
            target_compile_definitions(${targetName} PRIVATE BOOST_STACKTRACE_USE_ADDR2LINE)
        endif ()
    elseif (TARGET Boost::stacktrace_addr2line)
        target_link_libraries(${targetName} PUBLIC Boost::stacktrace_addr2line dl)
    elseif (TARGET Boost::stacktrace_basic)
        target_link_libraries(${targetName} PUBLIC Boost::stacktrace_basic dl)
    else ()
        target_link_libraries(${targetName} PUBLIC Boost::stacktrace_noop)
    endif ()
endfunction()