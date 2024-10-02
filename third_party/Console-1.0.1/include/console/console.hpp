// Copyright (c) 2023 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#ifdef TTLDTOR_CONSOLE_EXPORT
#    error TTLDTOR_CONSOLE_EXPORT was previously defined
#endif

#if defined(TTLDTOR_CONSOLE_USE_DLLS) && defined(_MSC_VER)
#    if defined(LIB_TTLDTOR_CONSOLE_EXPORTS)
#        define TTLDTOR_CONSOLE_EXPORT __declspec(dllexport)
#        define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DECLARE
#        define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DEFINE __declspec(dllexport)
#    else
#        define TTLDTOR_CONSOLE_EXPORT __declspec(dllimport)
#        define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DECLARE
#        define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DEFINE __declspec(dllimport)
#    endif // defined(LIB_TTLDTOR_CONSOLE)
#elif defined(TTLDTOR_CONSOLE_USE_DLLS) && defined(LIB_TTLDTOR_CONSOLE_EXPORTS)
#    define TTLDTOR_CONSOLE_EXPORT __attribute__((visibility("default")))
#    define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DECLARE __attribute__((visibility("default")))
#    define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DEFINE
#else
#    define TTLDTOR_CONSOLE_EXPORT
#    define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DECLARE
#    define TTLDTOR_CONSOLE_EXPORT_TEMPLATE_DEFINE
#endif

#include <cstdint>
#include <utility>

namespace org {
namespace ttldtor {
namespace console {

struct TTLDTOR_CONSOLE_EXPORT Console {
    /**
     * Returns the width and height of the current console.
     * @return The width and height of the current console.
     */
    static std::pair<std::size_t, std::size_t> getSize() noexcept;
};

} // namespace console
} // namespace ttldtor
} // namespace org
