// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>

/**
 * \addtogroup dxfcpp_util
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

// windows.h defines TRUE and FALSE as macros: they are hidden here and restored after the enum.
#pragma push_macro("TRUE")
#pragma push_macro("FALSE")
#undef TRUE
#undef FALSE

/**
 * Lightweight implementation of "nullable bool": a flag that is either set, not set, or not relevant.
 */
enum class Tristate : std::uint8_t {
    /// The flag is not set.
    FALSE = 0,

    /// The flag is set.
    TRUE = 1,

    /// The flag is not relevant: it is not taken into account.
    NONE = 2,
};

#pragma pop_macro("FALSE")
#pragma pop_macro("TRUE")

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
