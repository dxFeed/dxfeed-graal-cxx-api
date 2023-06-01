// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "../api/osub/WildcardSymbol.hpp"
#include "StringSymbol.hpp"

namespace dxfcpp {

/**
 * A helper wrapper class needed to pass heterogeneous symbols using a container and convert them to internal Graal
 * representation.
 */
struct DXFCPP_EXPORT SymbolWrapper final {
    using DataType = typename std::variant<WildcardSymbol, StringSymbol>;

  private:
    DataType data_;

  public:
    SymbolWrapper(const SymbolWrapper &) noexcept = default;
    SymbolWrapper(SymbolWrapper &&) noexcept = delete;
    SymbolWrapper &operator=(const SymbolWrapper &) noexcept = default;
    SymbolWrapper &operator=(SymbolWrapper &&) noexcept = delete;
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
     * @param wildcardSymbol The wrapped wildcard symbl
     */
    SymbolWrapper(const WildcardSymbol &wildcardSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper(wildcardSymbol = " + toStringAny(wildcardSymbol) + ")");
        }

        data_ = wildcardSymbol;
    }

    void *toGraal() const noexcept {
        return std::visit([](const auto &symbol) { return symbol.toGraal(); }, data_);
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept {
        return "SymbolWrapper{" + std::visit([](const auto &symbol) { return toStringAny(symbol); }, data_) + "}";
    }

    /**
     * @return `true` if current SymbolWrapper holds a WildcardSymbol
     */
    bool isWildcardSymbol() const noexcept { return std::holds_alternative<WildcardSymbol>(data_); }

    /**
     * @return WildcardSymbol (optional) or nullopt if current SymbolWrapper doesn't hold WildcardSymbol
     */
    std::optional<WildcardSymbol> asWildcardSymbol() const noexcept {
        return isWildcardSymbol() ? std::make_optional(WildcardSymbol::ALL) : std::nullopt;
    }

    /**
     * @return `true` if current SymbolWrapper holds a StringSymbol
     */
    bool isStringSymbol() const noexcept { return std::holds_alternative<StringSymbol>(data_); }

    /**
     * @return String representation of StringSymbol or an empty string
     */
    std::string asStringSymbol() const noexcept {
        return isStringSymbol() ? std::get<StringSymbol>(data_).getData() : String::EMPTY;
    }

    const DataType &getData() const noexcept { return data_; }

    bool operator==(const SymbolWrapper &symbolWrapper) const { return getData() == symbolWrapper.getData(); }

    auto operator<(const SymbolWrapper &symbolWrapper) const { return getData() < symbolWrapper.getData(); }
};

/**
 * A concept describing a symbol that can be wrapped.
 *
 * @tparam T Probable symbol type
 */
template <typename T>
concept ConvertibleToSymbolWrapper =
    ConvertibleToStringSymbol<std::decay_t<T>> || std::is_same_v<std::decay_t<T>, WildcardSymbol>;

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
            { *std::begin(c) } -> std::convertible_to<SymbolWrapper>;
        } ||
        requires(Collection c) {
            { *std::begin(c) } -> ConvertibleToSymbolWrapper;
        });

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

} // namespace dxfcpp

template <> struct std::hash<dxfcpp::SymbolWrapper> {
    std::size_t operator()(const dxfcpp::SymbolWrapper &symbolWrapper) const noexcept {
        return std::hash<dxfcpp::SymbolWrapper::DataType>{}(symbolWrapper.getData());
    }
};