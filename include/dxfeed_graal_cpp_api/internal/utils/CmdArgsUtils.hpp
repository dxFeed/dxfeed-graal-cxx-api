// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include "../../event/EventTypeEnum.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace dxfcpp {

struct DXFCPP_EXPORT CmdArgsUtils final {
    /**
     * Parses an input string and returns a set of symbols.
     *
     * If the result type is SymbolWrapper, then the symbol "all" or "*" can be passed. This will add WildcardSymbol
     * (all symbols).
     *
     * @tparam R The result type (std::string, SymbolWrapper, CandleSymbol)
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    template <typename R = std::string> static std::unordered_set<R> parseSymbols(const std::string &symbols) noexcept;

    /**
     * Parses an input string and returns a set of symbols.
     *
     * If the result type is SymbolWrapper, then the symbol "all" or "*" can be passed. This will add WildcardSymbol
     * (all symbols).
     *
     * @tparam R The result type (std::string, SymbolWrapper, CandleSymbol)
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    template <typename R = std::string> static std::unordered_set<R> parseSymbols(const char *symbols) noexcept {
        return parseSymbols<R>(std::string(symbols));
    }

    /**
     * Parses an input string and returns a set of symbols.
     *
     * If the result type is SymbolWrapper, then the symbol "all" or "*" can be passed. This will add WildcardSymbol
     * (all symbols).
     *
     * @tparam R The result type (std::string, SymbolWrapper, CandleSymbol)
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    template <typename R = std::string> static std::unordered_set<R> parseSymbols(std::string_view symbols) noexcept {
        return parseSymbols<R>(symbols.data());
    }

    /**
     * Parses an input string and returns a set of symbols.
     *
     * If the result type is SymbolWrapper, then the symbol "all" or "*" can be passed. This will add WildcardSymbol
     * (all symbols).
     *
     * @tparam R The result type (std::string, SymbolWrapper, CandleSymbol)
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    template <typename R = std::string>
    static std::unordered_set<R> parseSymbols(std::optional<std::string> symbols) noexcept {
        if (symbols.has_value()) {
            return parseSymbols<R>(symbols.value());
        }

        return {};
    }

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created set of parsed types.
     */
    static std::unordered_set<std::reference_wrapper<const EventTypeEnum>>
    parseTypes(const std::string &types) noexcept;

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created set of parsed types.
     */
    static std::unordered_set<std::reference_wrapper<const EventTypeEnum>> parseTypes(const char *types) noexcept {
        return parseTypes(std::string(types));
    }

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created set of parsed types.
     */
    static std::unordered_set<std::reference_wrapper<const EventTypeEnum>> parseTypes(std::string_view types) noexcept {
        return parseTypes(types.data());
    }

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created set of parsed types.
     */
    static std::unordered_set<std::reference_wrapper<const EventTypeEnum>>
    parseTypes(std::optional<std::string> types) noexcept;

    /**
     * Parses the input collection of strings and returns a collection of key-value properties.
     * The input strings should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(const std::string &properties) noexcept;

    /**
     * Parses the input collection of strings and returns a collection of key-value properties.
     * The input strings should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(const char *properties) noexcept {
        return parseProperties(std::string(properties));
    }

    /**
     * Parses the input collection of strings and returns a collection of key-value properties.
     * The input strings should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(std::string_view properties) noexcept {
        return parseProperties(properties.data());
    }

    /**
     * Parses the input collection of strings and returns a collection of key-value properties.
     * The input strings should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string>
    parseProperties(std::optional<std::string> properties) noexcept {
        if (properties.has_value()) {
            return parseProperties(properties.value());
        }

        return {};
    }

    /**
     * Parses Date+Time string and converts to timestamp
     *
     * @param string Date+Time string
     * @return UTC timestamp
     */
    static std::int64_t parseDateTime(const std::string &string) noexcept;

    /**
     * Parses Date+Time string and converts to timestamp
     *
     * @param string Date+Time string
     * @return UTC timestamp
     */
    static std::int64_t parseDateTime(const char *string) noexcept {
        return parseDateTime(std::string(string));
    }

    /**
     * Parses Date+Time string and converts to timestamp
     *
     * @param string Date+Time string
     * @return UTC timestamp
     */
    static std::int64_t parseDateTime(std::string_view string) noexcept {
        return parseDateTime(string.data());
    }

    /**
     * Parses Date+Time string and converts to timestamp
     *
     * @param string Date+Time string
     * @return UTC timestamp
     */
    static std::int64_t parseDateTime(std::optional<std::string> string) noexcept {
        if (string.has_value()) {
            return parseDateTime(string.value());
        }

        return -1;
    }
};

} // namespace dxfcpp