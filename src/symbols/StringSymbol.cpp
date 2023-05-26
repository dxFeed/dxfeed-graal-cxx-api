// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

namespace dxfcpp {

struct StringSymbol::Impl {
    dxfg_string_symbol_t graalSymbol{{STRING}, nullptr};
};

StringSymbol::StringSymbol() noexcept : impl_{std::make_unique<StringSymbol::Impl>()} {}

StringSymbol::~StringSymbol() noexcept = default;

void *StringSymbol::toGraal() {
    if (impl_->graalSymbol.symbol == nullptr) {
        std::visit(
            [this]<typename SymbolType>(SymbolType symbol) {
                if constexpr (std::is_same_v<SymbolType, std::string>) {
                    impl_->graalSymbol.symbol = symbol.c_str();
                } else if constexpr (std::is_same_v<SymbolType, std::string_view>) {
                    impl_->graalSymbol.symbol = symbol.data();
                }
            },
            data_);
    }

    return bit_cast<void *>(&impl_->graalSymbol);
}

} // namespace dxfcpp