// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

namespace dxfcpp {

/**
 * A helper wrapper class needed to pass heterogeneous string symbols using a container and convert them to internal Graal representation.
 *
 * The current implementation is suboptimal (by memory usage) and will be enhanced.
 */
struct DXFCPP_EXPORT StringSymbol final {
  private:
    std::string data_;

    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    StringSymbol(const StringSymbol &stringSymbol) noexcept;
    StringSymbol(StringSymbol &&) noexcept;
    StringSymbol &operator=(const StringSymbol &stringSymbol) noexcept;
    StringSymbol &operator=(StringSymbol &&) noexcept;
    StringSymbol() noexcept;
    ~StringSymbol() noexcept;

    /**
     * Constructs StringSymbol from a char array
     *
     * @param chars The array of chars
     */
    StringSymbol(const char *chars) noexcept : StringSymbol() {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("StringSymbol(chars = " + toStringAny(chars) + ")");
        }

        data_ = std::string(chars);
    }

    /**
     * Constructs StringSymbol from a std::string_view
     *
     * @param stringView The string view
     */
    StringSymbol(std::string_view stringView) noexcept : StringSymbol() {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("StringSymbol(stringView = " + toStringAny(stringView) + ")");
        }

        data_ = std::string(stringView);
    }

    StringSymbol(std::string string) noexcept : StringSymbol() {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("StringSymbol(string = " + toStringAny(string) + ")");
        }

        data_ = std::move(string);
    }

    void *toGraal() const noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept { return "StringSymbol{" + data_ + "}"; }

    const std::string &getData() const;

    bool operator==(const StringSymbol &stringSymbol) const { return getData() == stringSymbol.getData(); }

    bool operator<(const StringSymbol &stringSymbol) const { return getData() < stringSymbol.getData(); }
};

/**
 * A concept describing a string that can be wrapped.
 *
 * @tparam T Probable string symbol type
 */
template <typename T>
concept ConvertibleToStringSymbol =
    std::is_convertible_v<std::decay_t<T>, std::string> || std::is_convertible_v<std::decay_t<T>, std::string_view>;

inline namespace literals {

/**
 * String literal that helps to construct StringSymbol from a char array.
 *
 * @param string The char array
 * @param length Tha char array's length
 * @return Wrapped string view built on char array
 */
inline StringSymbol operator""_s(const char *string, size_t length) noexcept {
    return {std::string_view{string, length}};
}

} // namespace literals

DXFCPP_EXPORT std::string graalSymbolToString(void *graalSymbol);

} // namespace dxfcpp

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::StringSymbol> {
    std::size_t operator()(const dxfcpp::StringSymbol &stringSymbol) const noexcept {
        return std::hash<std::string>{}(stringSymbol.getData());
    }
};
