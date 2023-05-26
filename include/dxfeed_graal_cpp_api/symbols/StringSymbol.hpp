// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

namespace dxfcpp {

struct StringSymbol {
    using DataType = typename std::variant<std::string_view, std::string>;

  private:
    DataType data_;

    struct Impl;
    std::unique_ptr<Impl> impl_;

  public:
    StringSymbol() noexcept;
    ~StringSymbol() noexcept;

    template <typename SymbolType> StringSymbol(SymbolType &&symbol) noexcept : StringSymbol() {
        if constexpr (std::is_convertible_v<SymbolType, std::string_view>) {
            data_ = std::string_view(std::forward<SymbolType>(symbol));
        } else if constexpr (std::is_convertible_v<SymbolType, std::string>) {
            data_ = std::string(std::forward<SymbolType>(symbol));
        }
    }

    void *toGraal();

    std::string toString() const noexcept {
        return "StringSymbol{" + std::visit([](auto &&d) { return std::string(d); }, data_) + "}";
    }

    operator std::string() const noexcept { return toString(); }
};

template <typename T>
concept ConvertibleToStringSymbol = std::is_convertible_v<T, std::string> || std::is_convertible_v<T, std::string_view>;

inline namespace literals {

inline StringSymbol operator""_s(const char *string, size_t length) noexcept { return {std::string_view{string, length}}; }

} // namespace literals

} // namespace dxfcpp
