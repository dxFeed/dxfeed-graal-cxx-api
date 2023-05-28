// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

#include "StringSymbol.hpp"
#include "WildcardSymbol.hpp"

namespace dxfcpp {

struct SymbolWrapper final {
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

    template <ConvertibleToStringSymbol Symbol>
    SymbolWrapper(Symbol &&symbol) noexcept : SymbolWrapper(StringSymbol(std::forward<Symbol>(symbol))) {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper(symbol = " + toStringAny(symbol) + ")");
        }
    }

    SymbolWrapper(const StringSymbol &stringSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper(stringSymbol = " + toStringAny(stringSymbol) + ")");
        }

        data_ = stringSymbol;
    }

    SymbolWrapper(const WildcardSymbol &wildcardSymbol) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("SymbolWrapper(wildcardSymbol = " + toStringAny(wildcardSymbol) + ")");
        }

        data_ = wildcardSymbol;
    }

    void *toGraal() const noexcept {
        return std::visit([](const auto &symbol) { return symbol.toGraal(); }, data_);
    }

    std::string toString() const noexcept {
        return "SymbolWrapper{" + std::visit([](const auto &symbol) { return toStringAny(symbol); }, data_) + "}";
    }
};

template <typename T>
concept ConvertibleToSymbolWrapper =
    ConvertibleToStringSymbol<std::decay_t<T>> || std::is_same_v<std::decay_t<T>, WildcardSymbol>;

} // namespace dxfcpp