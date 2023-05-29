// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

namespace dxfcpp {

struct StringSymbol final {
    using DataType = typename std::variant<std::string_view, std::string>;

  private:
    //DataType data_;
    std::string data_;

    struct Impl;
    std::shared_ptr<Impl> impl_;

  public:
    StringSymbol(const StringSymbol &stringSymbol) noexcept;
    StringSymbol(StringSymbol &&) noexcept = delete;
    StringSymbol &operator=(const StringSymbol &stringSymbol) noexcept;
    StringSymbol &operator=(StringSymbol &&) noexcept = delete;
    StringSymbol() noexcept;
    ~StringSymbol() noexcept;

    StringSymbol(const char *chars) noexcept : StringSymbol() {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("StringSymbol(chars = " + toStringAny(chars) + ")");
        }

        data_ = std::string(chars);
    }

    StringSymbol(std::string_view stringView) noexcept : StringSymbol() {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("StringSymbol(stringView = " + toStringAny(stringView) + ")");
        }

        data_ = std::string(stringView);
    }

    StringSymbol(const std::string &string) noexcept : StringSymbol() {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("StringSymbol(string = " + toStringAny(string) + ")");
        }

        data_ = string;
    }

    void *toGraal() const noexcept;

    std::string toString() const noexcept {
        //return "StringSymbol{" + std::visit([](auto &&d) { return toStringAny(d); }, data_) + "}";
        return "StringSymbol{" + data_ + "}";
    }
};

template <typename T>
concept ConvertibleToStringSymbol =
    std::is_convertible_v<std::decay_t<T>, std::string> || std::is_convertible_v<std::decay_t<T>, std::string_view>;

inline namespace literals {

inline StringSymbol operator""_s(const char *string, size_t length) noexcept {
    return {std::string_view{string, length}};
}

} // namespace literals

std::string graalSymbolToString(void* graalSymbol);

} // namespace dxfcpp
