// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/model/IndexedTxModel.hpp>

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api/DXFeed.hpp>
#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedTxModel.hpp>

DXFCPP_BEGIN_NAMESPACE

void IndexedTxModel::setSourcesImpl(void *graalEventSourceList) const {
    isolated::model::IsolatedIndexedTxModel::setSources(handle_, graalEventSourceList);
}

std::shared_ptr<IndexedTxModel::Builder> IndexedTxModel::newBuilder(const EventTypeEnum &eventType) {
    // TODO: impl

    return {};
}

bool IndexedTxModel::isBatchProcessing() const {
    return isolated::model::IsolatedIndexedTxModel::isBatchProcessing(handle_);
}

bool IndexedTxModel::isSnapshotProcessing() const {
    return isolated::model::IsolatedIndexedTxModel::isSnapshotProcessing(handle_);
}

void IndexedTxModel::attach(std::shared_ptr<DXFeed> feed) const {
    isolated::model::IsolatedIndexedTxModel::attach(handle_, feed->handle_);
}

void IndexedTxModel::detach(std::shared_ptr<DXFeed> feed) const {
    isolated::model::IsolatedIndexedTxModel::detach(handle_, feed->handle_);
}

void IndexedTxModel::close() const {
    isolated::model::IsolatedIndexedTxModel::close(handle_);
}

// workaround for dxfg_CList_IndexedEventSource_release
void releaseGraalSources(void *sources) {
    if (sources == nullptr) {
        return;
    }

    if (auto list = dxfcpp::bit_cast<dxfg_indexed_event_source_list *>(sources); list->size > 0) {
        for (auto i = 0; i < list->size; i++) {
            isolated::event::IsolatedIndexedEventSource::release(list->elements[i]);
        }
    }

    isolated::internal::freeGraal(sources);
}

std::unordered_set<EventSourceWrapper> IndexedTxModel::getSources() const {
    std::unique_ptr<void, decltype(&releaseGraalSources)> graalSources{
        isolated::model::IsolatedIndexedTxModel::getSources(handle_), releaseGraalSources};

    if (graalSources == nullptr) {
        return {};
    }

    auto sources = EventSourceWrapper::ListUtils::fromGraalList(graalSources.get());

    return {sources.begin(), sources.end()};
}

void IndexedTxModel::setSources(std::initializer_list<EventSourceWrapper> collection) const {
    setSources(collection.begin(), collection.end());
}

DXFCPP_END_NAMESPACE