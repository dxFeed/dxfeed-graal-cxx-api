// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

/**
 * A helper wrapper class needed to pass heterogeneous string symbols using a container and convert them to internal
 * Graal representation.
 *
 * The current implementation is suboptimal (by memory usage) and will be enhanced.
 */
struct DXFCPP_EXPORT StringSymbol final {
  private:
    std::string data_;

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

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const;

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
     */
    static StringSymbol fromGraal(void *graalNative);

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const {
        if constexpr (Debugger::isDebug) {
            return "StringSymbol{" + data_ + "}";
        } else {
            return data_;
        }
    }

    const std::string &getData() const;

    bool operator==(const StringSymbol &stringSymbol) const {
        return getData() == stringSymbol.getData();
    }

    bool operator<(const StringSymbol &stringSymbol) const {
        return getData() < stringSymbol.getData();
    }
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

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::StringSymbol> {
    std::size_t operator()(const dxfcpp::StringSymbol &stringSymbol) const noexcept {
        return std::hash<std::string>{}(stringSymbol.getData());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
