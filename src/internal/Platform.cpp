// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <chrono>
#include <sstream>
#include <thread>

#if defined(__APPLE__)
#    include <TargetConditionals.h>
#endif

namespace dxfcpp {

std::string getOSName() noexcept {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__CYGWIN__)
    return "Windows";
#elif defined(__ANDROID__)
    return "Android";
#elif defined(__linux__)
    return "Linux";
#elif defined(__sun__)
    return "Solaris";
#elif defined(__APPLE__)
#    if defined(TARGET_IPHONE_SIMULATOR)
    return "iOS|tvOS|watchOS-Simulator";
#    elif defined(TARGET_OS_MACCATALYST)
    return "MacOS-Catalyst";
#    elif defined(TARGET_OS_IPHONE)
    return "iOS|tvOS|watchOS";
#    elif defined(TARGET_OS_MAC)
    return "MacOS";
#    else
#        return "UnknownApple";
#    endif
#elif defined(__FreeBSD__)
    return "FreeBSD";
#elif defined(__DragonFly__)
    return "DragonFly";
#elif defined(__NetBSD__)
    return "NetBSD";
#elif defined(__OpenBSD__)
    return "OpenBSD";
#elif defined(BSD)
    return "BSD-like";
#elif defined(__hpux)
    return "HP-UX";
#elif defined(_AIX) || defined(__TOS_AIX__)
    return "AIX";
#elif defined(__HAIKU__)
    return "Haiku";
#elif defined(__QNX__)
    return "QNX";
#elif defined(unix) || defined(__unix__) || defined(__unix)
    return "UNIX-like";
#elif defined(_POSIX_VERSION)
    return "POSIX-like";
#else
    return "Unknown";
#endif
}

std::string getOSArchitecture() noexcept {
#if defined(__alpha__) || defined(__alpha)
    return "alpha";
#elif defined(__arm64) || defined(__aarch64__)
    return "aarch64";
#elif defined(__arm__) || defined(__arm) || defined(_M_ARM)
    return "arm";
#elif defined(__amd64__) || defined(__x86_64__) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64)
    return "x86_64";
#elif defined(i386) || defined(__i386__) || defined(_M_IX86)
    return "x86_32";
#elif defined(__ia64__) || defined(__ia64) || defined(_M_IA64) || defined(__itanium__)
    return "itanium";
#elif defined(__mips__) || defined(mips) || defined(__mips) || defined(__MIPS__)
    return "mips";
#elif defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || defined(_ARCH_PPC64)
    return "ppc64";
#elif defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) || defined(__PPC__)
    return "powerpc";
#elif defined(__sparc__) || defined(__sparc)
    return "spark";
#elif defined(__370__) || defined(__s390__) || defined(__zarch__) || defined(__SYSC_ZARCH__)
    return "systemz";
#elif defined(__hppa__) || defined(__HPPA__) || defined(__hppa)
    return "hppa";
#elif defined(__convex__)
    return "convex";
#elif defined(__e2k__)
    return "e2k";
#elif defined(__riscv)
    return "riscv";
#elif defined(_TMS320C2XX) || defined(_TMS320C5X) || defined(_TMS320C6X)
    return "tms320";
#else
    return "unknown";
#endif
}

std::string Platform::getPlatformInfo() noexcept {
    const auto cores = getLogicalCoresCount();

    return fmt::format("{} {}({} core{})", getOSName(), getOSArchitecture(), cores, (cores > 1 ? "s" : ""));
}

} // namespace dxfcpp