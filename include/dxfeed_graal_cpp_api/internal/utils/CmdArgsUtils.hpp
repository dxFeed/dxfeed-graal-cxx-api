// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../event/EventTypeEnum.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "../../symbols/SymbolWrapper.hpp"
#include "../../event/EventSourceWrapper.hpp"

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
    static std::unordered_set<SymbolWrapper> parseSymbols(const std::string &symbols);

    /**
     * Parses an input string and returns a set of symbols.
     *
     * The symbol "all" or "*" can be passed. This will add WildcardSymbol (all symbols)..
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    static std::unordered_set<SymbolWrapper> parseSymbols(const char *symbols) noexcept {
        return parseSymbols(std::string(symbols));
    }

    /**
     * Parses an input string and returns a set of symbols.
     *
     * The symbol "all" or "*" can be passed. This will add WildcardSymbol (all symbols).
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    static std::unordered_set<SymbolWrapper> parseSymbols(std::string_view symbols) noexcept {
        return parseSymbols(symbols.data());
    }

    /**
     * Parses an input string and returns a set of symbols.
     *
     * The symbol "all" or "*" can be passed. This will add WildcardSymbol (all symbols).
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed symbols
     */
    static std::unordered_set<SymbolWrapper> parseSymbols(std::optional<std::string> symbols) noexcept;

    /**
     * Parses an input string and returns a set of candle symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed candle symbols
     */
    static std::unordered_set<CandleSymbol> parseCandleSymbols(const std::string &symbols);

    /**
     * Parses an input string and returns a set of candle symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed candle symbols
     */
    static std::unordered_set<CandleSymbol> parseCandleSymbols(const char *symbols) noexcept {
        return parseCandleSymbols(std::string(symbols));
    }

    /**
     * Parses an input string and returns a set of candle symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed candle symbols
     */
    static std::unordered_set<CandleSymbol> parseCandleSymbols(std::string_view symbols) noexcept {
        return parseCandleSymbols(symbols.data());
    }

    /**
     * Parses an input string and returns a set of candle symbols.
     *
     * @param symbols The coma-separated list of symbols.
     * @return The created set of parsed candle symbols
     */
    static std::unordered_set<CandleSymbol> parseCandleSymbols(std::optional<std::string> symbols) noexcept;

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created pair of set of parsed types and vector unknown types.
     */
    static std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
    parseTypes(const std::string &types);

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created pair of set of parsed types and vector unknown types.
     */
    static std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
    parseTypes(const char *types) {
        return parseTypes(std::string(types));
    }

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created pair of set of parsed types and vector unknown types.
     */
    static std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
    parseTypes(std::string_view types) {
        return parseTypes(types.data());
    }

    /**
     * Parses an input string and returns a set of event types.
     *
     * "all" or "*" will be converted to all types.
     *
     * @param types The comma-separated list of event types.
     * @return The created pair of set of parsed types and vector unknown types.
     */
    static std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
    parseTypes(std::optional<std::string> types);

    /**
     * Parses the input string and returns a collection of key-value properties.
     * The input string should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(const std::string &properties) noexcept;

    /**
     * Parses the input string and returns a collection of key-value properties.
     * The input string should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(const char *properties) noexcept {
        if (properties == nullptr) {
            return {};
        }

        return parseProperties(std::string(properties));
    }

    /**
     * Parses the input string and returns a collection of key-value properties.
     * The input string should look like comma-separated: "key=value".
     *
     * @param properties The input comma-separated key-value pairs.
     * @return The collection of key-value properties.
     */
    static std::unordered_map<std::string, std::string> parseProperties(std::string_view properties) noexcept {
        return parseProperties(properties.data());
    }

    /**
     * Parses the input string and returns a collection of key-value properties.
     * The input string should look like comma-separated: "key=value".
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
     * Parses the input string and returns a wrapped set of event sources (EventSourceWrapper).
     * The input string should look like comma-separated: "name1,name2".
     *
     * @param sources The input source names.
     * @return The set of sources or wrapped IndexedEventSource::DEFAULT if `sources` is empty.
     */
    static std::unordered_set<EventSourceWrapper> parseEventSources(const std::string &sources) noexcept;

    /**
     * Parses the input string and returns a wrapped set of event sources (EventSourceWrapper).
     * The input string should look like comma-separated: "name1,name2".
     *
     * @param sources The input source names.
     * @return The set of sources or wrapped IndexedEventSource::DEFAULT if `sources` is empty.
     */
    static std::unordered_set<EventSourceWrapper> parseEventSources(const char *sources) noexcept {
        if (sources == nullptr) {
            return {EventSourceWrapper{IndexedEventSource::DEFAULT}};
        }

        return parseEventSources(std::string(sources));
    }

    /**
     * Parses the input string and returns a wrapped set of event sources (EventSourceWrapper).
     * The input string should look like comma-separated: "name1,name2".
     *
     * @param sources The input source names.
     * @return The set of sources or wrapped IndexedEventSource::DEFAULT if `sources` is empty.
     */
    static std::unordered_set<EventSourceWrapper> parseEventSources(std::string_view sources) noexcept {
        return parseEventSources(sources.data());
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()