// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/internal/Conf.hpp"

#include <string>

DXFCPP_BEGIN_NAMESPACE

struct StringLike;

namespace candle {

/**
 * Removes the attribute with the default value from a candle symbol during normalization. If the removal would change
 * the base symbol (the base itself ends with `{...}`, as in `A{=t}{=t}`, so the next parse would take it for the
 * attributes), the attribute is kept with the canonical default value instead, so that normalization is idempotent.
 *
 * @param symbol The candle symbol.
 * @param key The attribute key.
 * @param defaultValue The string of the default value of the attribute.
 * @return The normalized symbol.
 */
std::string removeDefaultAttribute(const StringLike &symbol, const StringLike &key, const StringLike &defaultValue);

} // namespace candle

DXFCPP_END_NAMESPACE
