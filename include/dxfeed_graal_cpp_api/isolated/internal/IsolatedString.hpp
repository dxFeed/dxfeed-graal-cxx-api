// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedString {
/**
 * Calls the Graal SDK function `dxfg_String_release` in isolation.
 * @param string The pointer to C-string
 * @return `true` if OK.
 *
 * @throws std::invalid_argument if string is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool release(const char *string);
}

namespace IsolatedStringList {
/**
 * Calls the Graal SDK function `dxfg_CList_String_release` in isolation.
 * @param stringList The pointer to dxFeed Graal SDK' string list.
 * @return `true` if OK.
 *
 * @throws std::invalid_argument if stringList is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool release(/* dxfg_string_list* */ void *stringList);
}

}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()