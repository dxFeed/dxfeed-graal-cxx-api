// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

namespace dxfcpp {

struct StringSymbol::Impl final {
    dxfg_string_symbol_t graalSymbol;

    Impl() noexcept : graalSymbol{{STRING}, nullptr} {}

    Impl(const Impl &) noexcept {
        graalSymbol.supper = {STRING};
        graalSymbol.symbol = nullptr;
    }

    Impl(Impl &&) noexcept {
        graalSymbol.supper = {STRING};
        graalSymbol.symbol = nullptr;
    }

    Impl &operator=(const Impl &impl) noexcept {
        if (this == &impl) {
            return *this;
        }

        graalSymbol.supper = {STRING};
        graalSymbol.symbol = nullptr;

        return *this;
    }

    Impl &operator=(Impl &&impl) noexcept {
        if (this == &impl) {
            return *this;
        }

        graalSymbol.supper = {STRING};
        graalSymbol.symbol = nullptr;

        return *this;
    }
};

StringSymbol::StringSymbol(const StringSymbol &stringSymbol) noexcept : impl_{std::make_unique<StringSymbol::Impl>()} {
    data_ = stringSymbol.data_;
}

StringSymbol::StringSymbol(StringSymbol &&stringSymbol) noexcept : impl_{std::make_unique<StringSymbol::Impl>()} {
    data_ = std::move(stringSymbol.data_);
}

StringSymbol &StringSymbol::operator=(const StringSymbol &stringSymbol) noexcept {
    if (this == &stringSymbol) {
        return *this;
    }

    impl_ = std::make_unique<StringSymbol::Impl>();
    data_ = stringSymbol.data_;

    return *this;
}

StringSymbol &StringSymbol::operator=(StringSymbol &&stringSymbol) noexcept {
    if (this == &stringSymbol) {
        return *this;
    }

    impl_ = std::make_unique<StringSymbol::Impl>();
    data_ = std::move(stringSymbol.data_);

    return *this;
}

StringSymbol::StringSymbol() noexcept : impl_{std::make_unique<StringSymbol::Impl>()} {}

StringSymbol::~StringSymbol() noexcept = default;

void *StringSymbol::toGraal() const noexcept {
    if (impl_->graalSymbol.symbol == nullptr) {
        impl_->graalSymbol.symbol = data_.c_str();

        // std::visit([this](auto symbol) { impl_->graalSymbol.symbol = symbol.data(); }, data_);
    }

    return bit_cast<void *>(&impl_->graalSymbol);
}

const std::string &StringSymbol::getData() const { return data_; }

std::string toString(const dxfg_symbol_t *graalSymbol);

std::string graalSymbolToString(void *graalSymbol) {
    return !graalSymbol ? "null" : toString(bit_cast<const dxfg_symbol_t *>(graalSymbol));
}

} // namespace dxfcpp