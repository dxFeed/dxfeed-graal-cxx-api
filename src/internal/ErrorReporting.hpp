// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "InternalExport.hpp"

#include "../../include/dxfeed_graal_cpp_api/detail/StringParam.hpp"

#include <source_location>

DXFCPP_BEGIN_NAMESPACE

namespace detail {

/**
 * Reports a message of the library with the level ErrorInfo::Level::INFO: adds it to the journal of ErrorManager and
 * passes it to the handler like an error. Only the library reports INFO, there is no public function for it.
 *
 * @param component The part of the library that reports the message.
 * @param message The message.
 * @param location The place of the report in the source code.
 */
DXFCPP_INTERNAL_EXPORT void reportInfo(StringParam component, StringParam message,
                                       const std::source_location &location = std::source_location::current()) noexcept;

} // namespace detail

DXFCPP_END_NAMESPACE
