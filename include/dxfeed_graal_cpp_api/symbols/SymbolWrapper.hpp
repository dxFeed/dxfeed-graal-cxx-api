// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "../api/osub/IndexedEventSubscriptionSymbol.hpp"
#include "../api/osub/TimeSeriesSubscriptionSymbol.hpp"
#include "../api/osub/WildcardSymbol.hpp"
#include "../event/candle/CandleSymbol.hpp"
#include "../internal/Common.hpp"
#include "StringSymbol.hpp"

DXFCPP_BEGIN_NAMESPACE

/**
 * A concept describing a symbol that can be wrapped.
 *
 * @tparam T Probable symbol type
 */
template <typename T>
concept ConvertibleToSymbolWrapper =
    ConvertibleToStringSymbol<std::decay_t<T>> || std::is_same_v<std::decay_t<T>, WildcardSymbol> ||
    std::is_same_v<std::decay_t<T>, IndexedEventSubscriptionSymbol> ||
    std::is_same_v<std::decay_t<T>, TimeSeriesSubscriptionSymbol> || std::is_same_v<std::decay_t<T>, CandleSymbol>;

/**
 * A concept that defines a collection of wrapped or wrapping symbols.
 *
 * @tparam Collection The collection type
 */
template <typename Collection>
concept ConvertibleToSymbolWrapperCollection =
    requires(Collection c) {
        std::begin(c);
        std::end(c);
    } &&
    (
        requires(Collection c) {
            { *std::begin(c) } -> dxfcpp::ConvertibleTo<SymbolWrapper>;
        } ||
        requires(Collection c) {
            { *std::begin(c) } -> ConvertibleToSymbolWrapper;
        });

/**
 * A helper wrapper class needed to pass heterogeneous symbols using a container and convert them to internal Graal
 * representation.
 */
struct DXFCPP_EXPORT SymbolWrapper final {
    using DataType = typename std::variant<WildcardSymbol, StringSymbol, IndexedEventSubscriptionSymbol,
                                           TimeSeriesSubscriptionSymbol, CandleSymbol>;

    class DXFCPP_EXPORT SymbolListUtils final {
        static std::ptrdiff_t calculateGraalListSize(std::ptrdiff_t initSize) noexcept;
        static void *newGraalList(std::ptrdiff_t size);
        static bool setGraalListElement(void *graalList, std::ptrdiff_t elementIdx, void *element) noexcept;
        static bool freeGraalListElements(void *graalList, std::ptrdiff_t count);

      public:
        template <typename SymbolIt> static void *toGraalList(SymbolIt begin, SymbolIt end) {
            if constexpr (Debugger::isDebug) {
                Debugger::debug("SymbolWrapper::toGraalList(symbols = " + elementsToString(begin, end) + ")");
            }

            auto size = calculateGraalListSize(std::distance(begin, end));

            // Zero size is needed, for example, to clear the list of symbols.
            auto *list = newGraalList(size);

            if (!list || size == 0) {
                return list;
            }

            std::ptrdiff_t elementIdx = 0;
            bool needToFree = false;

            for (auto it = begin; it != end && elementIdx < size; it++, elementIdx++) {
                if constexpr (requires { it->toGraal(); }) {
                    needToFree = setGraalListElement(list, elementIdx, it->toGraal()) == false;
                } else if constexpr (std::is_convertible_v<decltype(*it), SymbolWrapper> ||
                                     dxfcpp::ConvertibleToSymbolWrapper<decltype(*it)>) {
                    needToFree = setGraalListElement(list, elementIdx, SymbolWrapper(*it).toGraal()) == false;
                }

                if (needToFree) {
                    break;
                }
            }

            if (needToFree) {
                freeGraalListElements(list, elementIdx);

                return nullptr;
            }

            return list;
        }

        template <ConvertibleToSymbolWrapperCollection SymbolsCollection>
        static void *toGraalList(const SymbolsCollection &collection) noexcept {
            return SymbolListUtils::toGraalList(std::begin(collection), std::end(collection));
        }

        static void *toGraalList(std::initializer_list<SymbolWrapper> collection) noexcept {
            return SymbolListUtils::toGraalList(collection.begin(), collection.end());
        }

        static void freeGraalList(void *graalList);

        static std::vector<SymbolWrapper> fromGraalList(void *graalList);
    };

  private:
    DataType data_;

  public:
    SymbolWrapper(const SymbolWrapper &) noexcept = default;
    SymbolWrapper(SymbolWrapper &&) noexcept = default;
    SymbolWrapper &operator=(const SymbolWrapper &) noexcept = default;
    SymbolWrapper &operator=(SymbolWrapper &&) noexcept = default;
    SymbolWrapper() noexcept = default;
    ~SymbolWrapper() noexcept = default;

    /**
     * Constructor for any wrapped symbol.
     * Must be implicit to wrap symbols passed to collection or container
     *
     * @tparam Symbol The symbol type
     * @param symbol The symbol
     */
    template <ConvertibleToStringSymbol Symbol>
    SymbolWrapper(Symbol &&symbol) noexcept : SymbolWrapper(StringSymbol(std::forward<Symbol>(symbol))) {
        if constexpr (Debugger::isDebug) {
            // Could be "perfectly" moved, so it won't show up in the logs
            Debugger::debug("SymbolWrapper(symbol = " + toStringAny(symbol) + ")");
        }
    }

    /**
     * Constructor for any wrapped string symbol.
     * Must be implicit to wrap symbols passed to collection or container
     *
     * @param stringSymbol The wrapped string (std::string, std::string_view, const char*) symbol
     */
    SymbolWrapper(const StringSymbol &stringSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper(stringSymbol = " + toStringAny(stringSymbol) + ")");
        }

        data_ = stringSymbol;
    }

    /**
     * Constructor for any wrapped wildcard (*) symbol.
     *
     * @param wildcardSymbol The wrapped wildcard symbol
     */
    SymbolWrapper(const WildcardSymbol &wildcardSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper(wildcardSymbol = " + toStringAny(wildcardSymbol) + ")");
        }

        data_ = wildcardSymbol;
    }

    /**
     * Constructor for IndexedEventSubscriptionSymbol.
     *
     * @param indexedEventSubscriptionSymbol The IndexedEventSubscriptionSymbol
     */
    SymbolWrapper(const IndexedEventSubscriptionSymbol &indexedEventSubscriptionSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(
                "SymbolWrapper(indexedEventSubscriptionSymbol = " + toStringAny(indexedEventSubscriptionSymbol) + ")");
        }

        data_ = indexedEventSubscriptionSymbol;
    }

    /**
     * Constructor for TimeSeriesSubscriptionSymbol.
     *
     * @param timeSeriesSubscriptionSymbol The TimeSeriesSubscriptionSymbol
     */
    SymbolWrapper(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug(
                "SymbolWrapper(timeSeriesSubscriptionSymbol = " + toStringAny(timeSeriesSubscriptionSymbol) + ")");
        }

        data_ = timeSeriesSubscriptionSymbol;
    }

    /**
     * Constructor for CandleSymbol.
     *
     * @param candleSymbol The CandleSymbol
     */
    SymbolWrapper(const CandleSymbol &candleSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper(candleSymbol = " + toStringAny(candleSymbol) + ")");
        }

        data_ = candleSymbol;
    }

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     * @throws RuntimeException if symbol type is unknown
     */
    static SymbolWrapper fromGraal(void *graalNative);

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper::toGraal()");
        }

        return std::visit(
            [](const auto &symbol) {
                return symbol.toGraal();
            },
            data_);
    }

    std::unique_ptr<void, decltype(&SymbolWrapper::freeGraal)> toGraalUnique() const noexcept {
        return {toGraal(), SymbolWrapper::freeGraal};
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const {
        return "SymbolWrapper{" +
               std::visit(
                   [](const auto &symbol) {
                       return toStringAny(symbol);
                   },
                   data_) +
               "}";
    }

    /**
     * Returns a string representation of the underlying object.
     *
     * @return a string representation of the underlying object.
     */
    std::string toStringUnderlying() const {
        return std::visit(
                   [](const auto &symbol) {
                       return toStringAny(symbol);
                   },
                   data_);
    }

    /**
     * @return `true` if current SymbolWrapper holds a StringSymbol
     */
    bool isStringSymbol() const noexcept {
        return std::holds_alternative<StringSymbol>(data_);
    }

    /**
     * @return String representation of StringSymbol or an empty string
     */
    std::string asStringSymbol() const noexcept {
        return isStringSymbol() ? std::get<StringSymbol>(data_).getData() : String::EMPTY;
    }

    /**
     * @return `true` if current SymbolWrapper holds a WildcardSymbol
     */
    bool isWildcardSymbol() const noexcept {
        return std::holds_alternative<WildcardSymbol>(data_);
    }

    /**
     * @return WildcardSymbol (optional) or std::nullopt if current SymbolWrapper doesn't hold WildcardSymbol
     */
    std::optional<WildcardSymbol> asWildcardSymbol() const noexcept {
        return isWildcardSymbol() ? std::make_optional(WildcardSymbol::ALL) : std::nullopt;
    }

    /**
     * @return `true` if current SymbolWrapper holds a IndexedEventSubscriptionSymbol
     */
    bool isIndexedEventSubscriptionSymbol() const noexcept {
        return std::holds_alternative<IndexedEventSubscriptionSymbol>(data_);
    }

    /**
     * @return IndexedEventSubscriptionSymbol (optional) or std::nullopt if current SymbolWrapper doesn't hold
     * IndexedEventSubscriptionSymbol
     */
    std::optional<IndexedEventSubscriptionSymbol> asIndexedEventSubscriptionSymbol() const noexcept {
        return isIndexedEventSubscriptionSymbol()
                   ? std::make_optional<IndexedEventSubscriptionSymbol>(std::get<IndexedEventSubscriptionSymbol>(data_))
                   : std::nullopt;
    }

    /**
     * @return `true` if current SymbolWrapper holds a TimeSeriesSubscriptionSymbol
     */
    bool isTimeSeriesSubscriptionSymbol() const noexcept {
        return std::holds_alternative<TimeSeriesSubscriptionSymbol>(data_);
    }

    /**
     * @return TimeSeriesSubscriptionSymbol (optional) or std::nullopt if current SymbolWrapper doesn't hold
     * TimeSeriesSubscriptionSymbol
     */
    std::optional<TimeSeriesSubscriptionSymbol> asTimeSeriesSubscriptionSymbol() const noexcept {
        return isTimeSeriesSubscriptionSymbol()
                   ? std::make_optional<TimeSeriesSubscriptionSymbol>(std::get<TimeSeriesSubscriptionSymbol>(data_))
                   : std::nullopt;
    }

    /**
     * @return `true` if current SymbolWrapper holds a CandleSymbol
     */
    bool isCandleSymbol() const noexcept {
        return std::holds_alternative<CandleSymbol>(data_);
    }

    /**
     * @return CandleSymbol (optional) or std::nullopt if current SymbolWrapper doesn't hold
     * CandleSymbol
     */
    std::optional<CandleSymbol> asCandleSymbol() const noexcept {
        return isCandleSymbol() ? std::make_optional<CandleSymbol>(std::get<CandleSymbol>(data_)) : std::nullopt;
    }

    const DataType &getData() const noexcept {
        return data_;
    }

    bool operator==(const SymbolWrapper &symbolWrapper) const {
        return getData() == symbolWrapper.getData();
    }

    auto operator<(const SymbolWrapper &symbolWrapper) const {
        return getData() < symbolWrapper.getData();
    }

    using GraalPtr = std::unique_ptr<void, decltype(&SymbolWrapper::freeGraal)>;
};

inline namespace literals {

/**
 * String literal that helps to construct SymbolWrapper from a char array.
 *
 * @param string The char array
 * @param length Tha char array's length
 * @return Wrapped string view built on char array
 */
inline SymbolWrapper operator""_sw(const char *string, size_t length) noexcept {
    return {std::string_view{string, length}};
}

} // namespace literals

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::SymbolWrapper> {
    std::size_t operator()(const dxfcpp::SymbolWrapper &symbolWrapper) const noexcept {
        return std::hash<dxfcpp::SymbolWrapper::DataType>{}(symbolWrapper.getData());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
