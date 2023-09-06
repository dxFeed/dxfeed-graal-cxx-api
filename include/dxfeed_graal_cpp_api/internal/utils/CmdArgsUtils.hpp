// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <type_traits>

namespace dxfcpp {

class EventTypeEnum;

struct DXFCPP_EXPORT CmdArgsUtils final {
    /**
     * Parses an input string and returns a set of symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols.
     */
    static std::unordered_set<std::string> parseSymbols(const std::string &symbols);

    /**
     * Parses an input string and returns a set of event types.
     *
     * @param types The comma-separated list of event types.
     * @return The created set of parsed types.
     */
    static std::unordered_set<std::reference_wrapper<const EventTypeEnum>> parseTypes(const std::string &types);
};

DXFCPP_EXPORT std::int64_t parseDateTimeOrPeriod(const std::string& string);
} // namespace dxfcpp