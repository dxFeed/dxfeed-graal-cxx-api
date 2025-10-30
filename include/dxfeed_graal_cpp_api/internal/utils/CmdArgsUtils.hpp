// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../event/EventSourceWrapper.hpp"
#include "../../event/EventTypeEnum.hpp"
#include "../../symbols/SymbolWrapper.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT CmdArgsUtils final {
    /**
     * Parses an input string and returns a set of symbols.
     *
     * The symbol "all" or "*" can be passed. This will add WildcardSymbol (all symbols).
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    static std::unordered_set<SymbolWrapper> parseSymbols(const StringLike &symbols);

    /**
     * Parses an input string and returns a set of symbols.
     *
     * The symbol "all" or "*" can be passed. This will add WildcardSymbol (all symbols).
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    template <typename S>
    static std::unordered_set<SymbolWrapper> parseSymbols(const std::optional<S> &symbols) {
        if (symbols.has_value()) {
            return parseSymbols(symbols.value());
        }

        return {};
    }
    /**
     * Parses an input string and returns a vector of symbols in the same order.
     *
     * The symbol "all" or "*" can be passed. This will add WildcardSymbol (all symbols).
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created vector of parsed symbols
     */
    static std::vector<SymbolWrapper> parseSymbolsAndSaveOrder(const StringLike &symbols);

    /**
     * Parses an input string and returns a set of candle symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed candle symbols
     */
    static std::unordered_set<CandleSymbol> parseCandleSymbols(const StringLike &symbols);

    /**
     * Parses an input string and returns a set of candle symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed candle symbols
     */
    template <typename S>
    static std::unordered_set<CandleSymbol> parseCandleSymbols(const std::optional<S> &symbols) {
        if (symbols.has_value()) {
            return parseCandleSymbols(symbols.value());
        }

        return {};
    }

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created pair: set of parsed types and vector unknown types.
     */
    static std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
    parseTypes(const StringLike &types);

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created pair of set of parsed types and vector unknown types.
     */
    template <typename S>
    static std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
    parseTypes(const std::optional<S> &types) {
        if (types.has_value()) {
            return parseTypes(types.value());
        }

        return {};
    }
    /**
     * Parses the input string and returns a collection of key-value properties.
     * The input string should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(const StringLike &properties) noexcept;

    /**
     * Parses the input string and returns a collection of key-value properties.
     * The input string should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    template <typename S>
    static std::unordered_map<std::string, std::string>
    parseProperties(const std::optional<S>& properties) noexcept {
        if (properties.has_value()) {
            return parseProperties(properties.value());
        }

        return {};
    }

    /**
     * Parses the input string and returns a wrapped set of event sources (EventSourceWrapper).
     * The input string should look like comma-separated: "name1,name2".
     *
     * @param sources The input source names.
     * @return The set of sources or wrapped IndexedEventSource::DEFAULT if `sources` is empty.
     */
    static std::unordered_set<EventSourceWrapper> parseEventSources(const StringLike &sources) noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()