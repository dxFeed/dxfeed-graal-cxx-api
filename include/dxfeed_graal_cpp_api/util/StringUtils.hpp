// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <string>

/**
 * \addtogroup dxfcpp_util
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Converts a double value to a string in the format of Java `Double.toString`: the shortest decimal representation
 * that identifies the value, `1.0` for 1, `1.0E-4` for 0.0001, `NaN`, `Infinity`, `-Infinity`, `-0.0`.
 *
 * @param d The value.
 * @return The string.
 */
DXFCPP_EXPORT std::string toString(double d); // NOLINT(*-redundant-declaration)

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
