// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/orcs/AuthOrderSource.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/orcs/IsolatedPriceLevelService.hpp"

#include <utility>

DXFCPP_BEGIN_NAMESPACE

AuthOrderSource::AuthOrderSource(LockExternalConstructionTag, JavaObjectHandle<AuthOrderSource> &&handle)
    : handle_(std::move(handle)) {
}

AuthOrderSource::~AuthOrderSource() noexcept = default;

std::unordered_map<std::int32_t, std::unordered_set<std::string>> AuthOrderSource::getByIds() const {
    return isolated::orcs::IsolatedAuthOrderSource::getByIds(handle_);
}

std::unordered_map<OrderSource, std::unordered_set<std::string>> AuthOrderSource::getByOrderSources() const {
    std::unordered_map<OrderSource, std::unordered_set<std::string>> result{};

    for (auto &&[id, symbols] : getByIds()) {
        result.emplace(OrderSource::valueOf(id), std::move(symbols));
    }

    return result;
}

DXFCPP_END_NAMESPACE
