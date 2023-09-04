// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <string>

namespace dxfcpp::enum_utils {

/**
 * Returns a separated list of names of EventTypeEnum elements.
 *
 * @param separator The list separator
 * @return A separated list of names
 */
DXFCPP_EXPORT std::string getEventTypeEnumNamesList(std::string separator = std::string(",")) noexcept;

}

