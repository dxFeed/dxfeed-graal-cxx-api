// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace dxfcpp {

class EventTypeEnum;

struct DXFCPP_EXPORT CmdArgsUtils final {
    /**
     * Parses an input string and returns a set of symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols.
     */
    static std::unordered_set<std::string> parseSymbols(const std::string &symbols) noexcept;

    /**
     * Parses an input string and returns a set of event types.
     *
     * @param types The comma-separated list of event types.
     * @return The created set of parsed types.
     */
    static std::unordered_set<std::reference_wrapper<const EventTypeEnum>>
    parseTypes(const std::string &types) noexcept;

    /**
     * Parses the input collection of strings and returns a collection of key-value properties.
     * The input strings should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(const std::string &properties) noexcept;
};

/**
 * Parses Date+Time string and converts to timestamp
 *
 * @param string Date+Time string
 * @return UTC timestamp
 */
DXFCPP_EXPORT std::int64_t parseDateTime(const std::string &string) noexcept;

struct Arg {};

struct ValueArg {};

struct AddressArg : ValueArg {
    [[nodiscard]] std::string getName() const noexcept {
        return "address";
    }

    [[nodiscard]] std::string getShortDescription() const noexcept {
        return
            R"(
        The address(es) to connect to retrieve data (see "Help address").
        For Token-Based Authorization, use the following format: "<address>:<port>[login=entitle:<token>]".
)";
    }

    [[nodiscard]] std::string parse(const std::string& s) const noexcept{
        return s;
    }
};

} // namespace dxfcpp