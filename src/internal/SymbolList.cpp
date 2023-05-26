// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

// TODO: StringSymbol, WildcardSymbol, CandleSymbol, indexed_event_subscription_symbol, time_series_subscription_symbol,
// ::toGraal

struct SymbolList::Impl
    : public RawListWrapper<dxfg_symbol_list,
                            []<typename Symbol>(dxfg_symbol_list &list, std::size_t index, Symbol &&symbol) {
                                dxfg_symbol_t *dxfgSymbol;

                                if constexpr (std::is_same_v<std::decay_t<Symbol>, std::string>) {
                                    dxfg_string_symbol_t s{{STRING}, symbol.c_str()};

                                    dxfgSymbol = bit_cast<dxfg_symbol_t *>(&s);
                                }
                                else if constexpr (std::is_same_v<std::decay_t<Symbol>, std::string_view>) {
                                    dxfg_string_symbol_t s{{STRING}, symbol.data()};

                                    dxfgSymbol = bit_cast<dxfg_symbol_t *>(&s);
                                }
                                else {
                                    dxfg_string_symbol_t s{{STRING}, symbol};

                                    dxfgSymbol = bit_cast<dxfg_symbol_t *>(&s);
                                }

                                list.elements[index] = dxfgSymbol;
                            }> {
};

SymbolList::SymbolList() noexcept : impl_(std::make_unique<SymbolList::Impl>()) {}

std::unique_ptr<SymbolList> SymbolList::create(std::size_t size) noexcept {
    auto result = std::unique_ptr<SymbolList>(new SymbolList{});

    result->impl_->init(static_cast<std::uint32_t>(size));

    return result;
}

template <typename Symbol> void SymbolList::set(std::size_t index, Symbol &&symbol) noexcept {
    impl_->set(index, symbol);
}

bool SymbolList::isEmpty() const noexcept { return impl_->isEmpty(); }

std::size_t SymbolList::size() const noexcept { return impl_->size(); }

void *SymbolList::getHandler() noexcept { return impl_->getHandler(); }

SymbolList::~SymbolList() noexcept = default;

} // namespace dxfcpp