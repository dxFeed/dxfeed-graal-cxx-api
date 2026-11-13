// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "InternalExport.hpp"

#include <string_view>

DXFCPP_BEGIN_NAMESPACE

namespace detail {

/**
 * Checks the address of DXEndpoint::connect() before it goes to the SDK. QD has no connector for an address that
 * starts with `http://`, `https://`, `ws://` or `wss://` (after the leading characters up to the space, which QD trims;
 * in any case), such an address never worked. A QD configuration URL `http(s)://.../<name>.config#<name>` and all the
 * other forms pass. Only the start of the whole address is checked: a URL inside a list keeps the error of QD.
 *
 * @param address The address.
 * @throws InvalidArgumentException that names the expected form of the address, without the address itself (it may
 * have a login or a password).
 */
DXFCPP_INTERNAL_EXPORT void checkEndpointAddress(std::string_view address);

} // namespace detail

DXFCPP_END_NAMESPACE
