MESSAGE(STATUS "DXFCXX_GRAAL_TARGET_PLATFORM: ${DXFCXX_GRAAL_TARGET_PLATFORM}")
MESSAGE(STATUS "DXFCXX_GRAAL_TARGET_OS: ${DXFCXX_GRAAL_TARGET_OS}")
MESSAGE(STATUS "DXFCXX_GRAAL_TARGET_CPU: ${DXFCXX_GRAAL_TARGET_CPU}")
MESSAGE(STATUS "DXFCXX_TARGET_PLATFORM: ${DXFCXX_TARGET_PLATFORM}")
MESSAGE(STATUS "DXFCXX_TARGET_OS: ${DXFCXX_TARGET_OS}")
MESSAGE(STATUS "DXFCXX_TARGET_CPU: ${DXFCXX_TARGET_CPU}")

if ((DXFCXX_GRAAL_TARGET_PLATFORM STREQUAL "unknown-unknown") AND (DXFCXX_TARGET_PLATFORM STREQUAL "unknown-unknown"))
    if ((DEFINED $ENV{DXFCXX_CUSTOM_OS}) AND (DXFCXX_TARGET_OS STREQUAL "unknown"))
        if ($ENV{DXFCXX_CUSTOM_OS} MATCHES "win")
            set(DXFCXX_TARGET_OS "windows")
            set(DXFCXX_GRAAL_TARGET_OS "windows")
        elseif (($ENV{DXFCXX_CUSTOM_OS} MATCHES "mac") OR ($ENV{DXFCXX_CUSTOM_OS} MATCHES "osx"))
            set(DXFCXX_TARGET_OS "osx")
            set(DXFCXX_GRAAL_TARGET_OS "osx")
        elseif (($ENV{DXFCXX_CUSTOM_OS} MATCHES "linux"))
            set(DXFCXX_TARGET_OS "linux")
            set(DXFCXX_GRAAL_TARGET_OS "linux")
        else ()
            set(DXFCXX_TARGET_OS $ENV{DXFCXX_CUSTOM_OS})
            set(DXFCXX_GRAAL_TARGET_OS $ENV{DXFCXX_CUSTOM_OS})
        endif ()
    endif ()

    if ((DEFINED $ENV{DXFCXX_CUSTOM_CPU}) AND (${DXFCXX_TARGET_CPU} STREQUAL "unknown"))
        if (($ENV{DXFCXX_CUSTOM_CPU} MATCHES "amd64") OR ($ENV{DXFCXX_CUSTOM_CPU} MATCHES "x86_64"))
            set(DXFCXX_TARGET_CPU "x86_64")
            set(DXFCXX_GRAAL_TARGET_CPU "amd64")
        elseif (($ENV{DXFCXX_CUSTOM_CPU} MATCHES "arm64") OR ($ENV{DXFCXX_CUSTOM_CPU} MATCHES "aarch64"))
            set(DXFCXX_TARGET_CPU "aarch64")
            set(DXFCXX_GRAAL_TARGET_CPU "aarch64")
        else ()
            set(DXFCXX_TARGET_CPU $ENV{DXFCXX_CUSTOM_CPU})
            set(DXFCXX_GRAAL_TARGET_CPU $ENV{DXFCXX_CUSTOM_CPU})
        endif ()
    endif ()
else ()
    if (DXFCXX_GRAAL_TARGET_PLATFORM STREQUAL "unknown-unknown")
        string(REPLACE "-" ";" DXFCXX_TARGET_CPU_AND_OS ${DXFCXX_TARGET_PLATFORM})
        list(LENGTH DXFCXX_TARGET_CPU_AND_OS DXFCXX_TARGET_CPU_AND_OS_LENGTH)

        if (DXFCXX_TARGET_CPU_AND_OS_LENGTH EQUAL 1)
            list(GET DXFCXX_TARGET_CPU_AND_OS 0 DXFCXX_TARGET_CPU_OR_OS)
            if ((DXFCXX_TARGET_CPU_OR_OS MATCHES "64") OR (DXFCXX_TARGET_CPU_OR_OS MATCHES "x86") OR (DXFCXX_TARGET_CPU_OR_OS MATCHES "arm"))
                if (DXFCXX_TARGET_CPU STREQUAL "unknown")
                    set(DXFCXX_TARGET_CPU ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()

                if (DXFCXX_GRAAL_TARGET_CPU STREQUAL "unknown")
                    set(DXFCXX_GRAAL_TARGET_CPU ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()
            else ()
                if (DXFCXX_TARGET_OS STREQUAL "unknown")
                    set(DXFCXX_TARGET_OS ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()

                if (DXFCXX_GRAAL_TARGET_OS STREQUAL "unknown")
                    set(DXFCXX_GRAAL_TARGET_OS ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()
            endif ()
        else ()
            if (DXFCXX_TARGET_CPU STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 0 DXFCXX_TARGET_CPU)
            endif ()

            if (DXFCXX_TARGET_OS STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 1 DXFCXX_TARGET_OS)
            endif ()

            if (DXFCXX_GRAAL_TARGET_CPU STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 0 DXFCXX_GRAAL_TARGET_CPU)
            endif ()

            if (DXFCXX_GRAAL_TARGET_OS STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 1 DXFCXX_GRAAL_TARGET_OS)
            endif ()
        endif ()
    elseif (DXFCXX_TARGET_PLATFORM STREQUAL "unknown-unknown")
        string(REPLACE "-" ";" DXFCXX_TARGET_CPU_AND_OS ${DXFCXX_GRAAL_TARGET_PLATFORM})
        list(LENGTH DXFCXX_TARGET_CPU_AND_OS DXFCXX_TARGET_CPU_AND_OS_LENGTH)

        if (DXFCXX_TARGET_CPU_AND_OS_LENGTH EQUAL 1)
            list(GET DXFCXX_TARGET_CPU_AND_OS 0 DXFCXX_TARGET_CPU_OR_OS)
            if ((DXFCXX_TARGET_CPU_OR_OS MATCHES "64") OR (DXFCXX_TARGET_CPU_OR_OS MATCHES "x86") OR (DXFCXX_TARGET_CPU_OR_OS MATCHES "arm"))
                if (DXFCXX_TARGET_CPU STREQUAL "unknown")
                    set(DXFCXX_TARGET_CPU ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()

                if (DXFCXX_GRAAL_TARGET_CPU STREQUAL "unknown")
                    set(DXFCXX_GRAAL_TARGET_CPU ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()
            else ()
                if (DXFCXX_TARGET_OS STREQUAL "unknown")
                    set(DXFCXX_TARGET_OS ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()

                if (DXFCXX_GRAAL_TARGET_OS STREQUAL "unknown")
                    set(DXFCXX_GRAAL_TARGET_OS ${DXFCXX_TARGET_CPU_OR_OS})
                endif ()
            endif ()
        else ()
            if (DXFCXX_TARGET_CPU STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 0 DXFCXX_TARGET_CPU)
            endif ()

            if (DXFCXX_TARGET_OS STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 1 DXFCXX_TARGET_OS)
            endif ()

            if (DXFCXX_GRAAL_TARGET_CPU STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 0 DXFCXX_GRAAL_TARGET_CPU)
            endif ()

            if (DXFCXX_GRAAL_TARGET_OS STREQUAL "unknown")
                list(GET DXFCXX_TARGET_CPU_AND_OS 1 DXFCXX_GRAAL_TARGET_OS)
            endif ()
        endif ()
    endif ()
endif ()

set(DXFCXX_CURRENT_OS "unknown")

if (WIN32)
    set(DXFCXX_CURRENT_OS "windows")
elseif (APPLE)
    set(DXFCXX_CURRENT_OS "osx")
elseif (UNIX)
    set(DXFCXX_CURRENT_OS "linux")
endif ()

if (DXFCXX_TARGET_OS STREQUAL "unknown")
    if (NOT DXFCXX_GRAAL_TARGET_OS STREQUAL "unknown")
        set(DXFCXX_TARGET_OS ${DXFCXX_GRAAL_TARGET_OS})
    else ()
        set(DXFCXX_TARGET_OS ${DXFCXX_CURRENT_OS})
    endif ()
endif ()

if (DXFCXX_GRAAL_TARGET_OS STREQUAL "unknown")
    if (NOT DXFCXX_TARGET_OS STREQUAL "unknown")
        set(DXFCXX_GRAAL_TARGET_OS ${DXFCXX_TARGET_OS})
    else ()
        set(DXFCXX_GRAAL_TARGET_OS ${DXFCXX_CURRENT_OS})
    endif ()
endif ()

set(DXFCXX_CURRENT_CPU "unknown")

if ((CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64") OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64") OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "ARM64") OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AARCH64"))
    set(DXFCXX_CURRENT_CPU "aarch64")
elseif ((CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64") OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "amd64") OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64"))
    set(DXFCXX_CURRENT_CPU "x86_64")
else ()
    set(DXFCXX_CURRENT_CPU ${CMAKE_HOST_SYSTEM_PROCESSOR})
endif ()

if (DXFCXX_TARGET_CPU STREQUAL "unknown")
    if (NOT DXFCXX_GRAAL_TARGET_CPU STREQUAL "unknown")
        set(DXFCXX_TARGET_CPU ${DXFCXX_GRAAL_TARGET_CPU})
    else ()
        set(DXFCXX_TARGET_CPU ${DXFCXX_CURRENT_CPU})
    endif ()
endif ()

if (DXFCXX_GRAAL_TARGET_CPU STREQUAL "unknown")
    if (NOT DXFCXX_TARGET_CPU STREQUAL "unknown")
        set(DXFCXX_GRAAL_TARGET_CPU ${DXFCXX_TARGET_CPU})
    else ()
        set(DXFCXX_GRAAL_TARGET_CPU ${DXFCXX_CURRENT_CPU})
    endif ()
endif ()

#normalization

if (DXFCXX_TARGET_OS STREQUAL "windows" OR DXFCXX_TARGET_OS STREQUAL "linux")
    if (DXFCXX_TARGET_CPU STREQUAL "amd64")
        set(DXFCXX_TARGET_CPU "x86_64")
    endif ()
endif ()

if (DXFCXX_GRAAL_TARGET_OS STREQUAL "windows" OR DXFCXX_GRAAL_TARGET_OS STREQUAL "linux")
    if (DXFCXX_GRAAL_TARGET_CPU STREQUAL "x86_64")
        set(DXFCXX_GRAAL_TARGET_CPU "amd64")
    endif ()
endif ()

if (DXFCXX_GRAAL_TARGET_PLATFORM MATCHES "unknown")
    set(DXFCXX_GRAAL_TARGET_PLATFORM "${DXFCXX_GRAAL_TARGET_CPU}-${DXFCXX_GRAAL_TARGET_OS}")
endif ()

if ($CACHE{DXFCXX_TARGET_PLATFORM} MATCHES "unknown")
    set(DXFCXX_TARGET_PLATFORM "${DXFCXX_TARGET_CPU}-${DXFCXX_TARGET_OS}")
endif ()

MESSAGE(STATUS "DXFCXX_GRAAL_TARGET_PLATFORM: ${DXFCXX_GRAAL_TARGET_PLATFORM}")
MESSAGE(STATUS "DXFCXX_GRAAL_TARGET_OS: ${DXFCXX_GRAAL_TARGET_OS}")
MESSAGE(STATUS "DXFCXX_GRAAL_TARGET_CPU: ${DXFCXX_GRAAL_TARGET_CPU}")
MESSAGE(STATUS "DXFCXX_TARGET_PLATFORM: ${DXFCXX_TARGET_PLATFORM}")
MESSAGE(STATUS "DXFCXX_TARGET_OS: ${DXFCXX_TARGET_OS}")
MESSAGE(STATUS "DXFCXX_TARGET_CPU: ${DXFCXX_TARGET_CPU}")