// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/internal/Conf.hpp"

#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * Returns `String::NUL` as a string object, for the functions that return a reference to a string. The object is never
 * destroyed, so it can be used in the constructors and the destructors of global variables.
 *
 * @return The `"<null>"` string.
 */
const std::string &nulString() noexcept;

DXFCPP_END_NAMESPACE
