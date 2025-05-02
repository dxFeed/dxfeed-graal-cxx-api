# Copyright (c) 2025 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

function(SetupStaticRuntimeMSVC targetName linkStaticRuntime noDefaultLib)
    if (${linkStaticRuntime})
        set_target_properties(${targetName} PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )

        if (${noDefaultLib})
            target_link_options(${targetName} PRIVATE
                    /NODEFAULTLIB:libcmt
                    /NODEFAULTLIB:libcmtd
                    /NODEFAULTLIB:msvcrt
                    /NODEFAULTLIB:msvcrtd
                    /VERBOSE
            )
        endif ()
    endif ()
endfunction()