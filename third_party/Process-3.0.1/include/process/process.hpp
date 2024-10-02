// Copyright (c) 2023 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#ifdef TTLDTOR_PROCESS_EXPORT
#    error TTLDTOR_PROCESS_EXPORT was previously defined
#endif

#if defined(TTLDTOR_PROCESS_USE_DLLS) && defined(_MSC_VER)
#    if defined(LIB_TTLDTOR_PROCESS_EXPORTS)
#        define TTLDTOR_PROCESS_EXPORT __declspec(dllexport)
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE __declspec(dllexport)
#    else
#        define TTLDTOR_PROCESS_EXPORT __declspec(dllimport)
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE
#        define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE __declspec(dllimport)
#    endif // defined(LIB_TTLDTOR_PROCESS)
#elif defined(TTLDTOR_PROCESS_USE_DLLS) && defined(LIB_TTLDTOR_PROCESS_EXPORTS)
#    define TTLDTOR_PROCESS_EXPORT __attribute__((visibility("default")))
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE __attribute__((visibility("default")))
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE
#else
#    define TTLDTOR_PROCESS_EXPORT
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DECLARE
#    define TTLDTOR_PROCESS_EXPORT_TEMPLATE_DEFINE
#endif

#include <chrono>
#include <cstdint>

namespace org {
namespace ttldtor {
namespace process {

struct TTLDTOR_PROCESS_EXPORT Process {

    /**
     * Returns the kernel (privileged, system) CPU time for the current process in ms.
     *
     * @return The kernel (privileged, system) CPU time for the current process in ms.
     */
    static std::chrono::milliseconds getKernelProcessorTime() noexcept;

    /**
     * Returns the kernel (privileged, system) CPU time for the current process in ms.
     *
     * @return The kernel (privileged, system) CPU time for the current process in ms.
     */
    static std::chrono::milliseconds getPrivilegedProcessorTime() noexcept {
        return getKernelProcessorTime();
    }

    /**
     * Returns the kernel (privileged, system) CPU time for the current process in ms.
     *
     * @return The kernel (privileged, system) CPU time for the current process in ms.
     */
    static std::chrono::milliseconds getSystemProcessorTime() noexcept {
        return getKernelProcessorTime();
    }

    /**
     * Returns the user CPU time for the current process in ms.
     *
     * @return The user CPU time for the current process in ms.
     */
    static std::chrono::milliseconds getUserProcessorTime() noexcept;

    /**
     * Returns the total (kernel + user) CPU time for the current process in ms.
     *
     * @return The total (kernel + user) CPU time for the current process in ms.
     */
    static std::chrono::milliseconds getTotalProcessorTime() noexcept;

    /**
     * Returns the size of physical memory equipped by the current process.
     *
     * @return The size of physical memory equipped by the current process.
     */
    static std::uint64_t getWorkingSetSize() noexcept;

    /**
     * Returns the size of physical memory equipped by the current process.
     *
     * @return The size of physical memory equipped by the current process.
     */
    static std::uint64_t getPhysicalMemorySize() noexcept {
        return getWorkingSetSize();
    }

    /**
     * Returns the size of private (virtual) memory equipped by the current process.
     *
     * @return The size of private (virtual) memory equipped by the current process.
     */
    static std::uint64_t getPrivateMemorySize() noexcept;

    /**
     * Returns the size of private (virtual) memory equipped by the current process.
     *
     * @return The size of private (virtual) memory equipped by the current process.
     */
    static std::uint64_t getVirtualMemorySize() noexcept {
        return getPrivateMemorySize();
    }
};

} // namespace process
} // namespace ttldtor
} // namespace org