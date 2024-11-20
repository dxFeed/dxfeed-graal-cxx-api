// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../event/market/OrderSource.hpp"
#include "../symbols/SymbolWrapper.hpp"
#include "../entity/SharedEntity.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT IndexedTxModel : RequireMakeShared<IndexedTxModel> {

    struct DXFCPP_EXPORT Builder {};

    static std::shared_ptr<Builder> newBuilder(const EventTypeEnum& eventType);


};

DXFCPP_END_NAMESPACE