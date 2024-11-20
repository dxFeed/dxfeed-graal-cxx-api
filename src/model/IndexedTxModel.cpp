// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/model/IndexedTxModel.hpp>

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedTxModel.hpp>

DXFCPP_BEGIN_NAMESPACE

void IndexedTxModel::setSourcesImpl(void *graalEventSourceList) const {
    // TODO: impl
}

std::shared_ptr<IndexedTxModel::Builder> IndexedTxModel::newBuilder(const EventTypeEnum &eventType) {
    // TODO: impl

    return {};
}

std::unordered_set<EventSourceWrapper> IndexedTxModel::getSources() const {
    // TODO: impl

    return {};
}

void IndexedTxModel::setSources(std::initializer_list<EventSourceWrapper> collection) const {
    setSources(collection.begin(), collection.end());
}

DXFCPP_END_NAMESPACE