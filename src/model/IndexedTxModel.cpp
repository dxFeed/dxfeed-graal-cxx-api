// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/model/IndexedTxModel.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedTxModel.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

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

JavaObjectHandle<IndexedTxModelBuilderTag>
IndexedTxModelImpl::Builder::withBatchProcessingImpl(const JavaObjectHandle<IndexedTxModelBuilderTag> &handle,
                                                     bool isBatchProcessing) {
    return isolated::model::IsolatedIndexedTxModel::Builder::withBatchProcessing(handle, isBatchProcessing);
}

JavaObjectHandle<IndexedTxModelBuilderTag>
IndexedTxModelImpl::Builder::withSnapshotProcessingImpl(const JavaObjectHandle<IndexedTxModelBuilderTag> &handle,
                                                        bool isSnapshotProcessing) {
    return isolated::model::IsolatedIndexedTxModel::Builder::withSnapshotProcessing(handle, isSnapshotProcessing);
}

JavaObjectHandle<IndexedTxModelBuilderTag>
IndexedTxModelImpl::Builder::withFeedImpl(const JavaObjectHandle<IndexedTxModelBuilderTag> &handle,
                                          const std::shared_ptr<DXFeed> &feed) {
    return isolated::model::IsolatedIndexedTxModel::Builder::withFeed(handle, feed->handle_);
}

JavaObjectHandle<IndexedTxModelBuilderTag>
IndexedTxModelImpl::Builder::withSymbolImpl(const JavaObjectHandle<IndexedTxModelBuilderTag> &handle,
                                            const SymbolWrapper &symbol) {
    return isolated::model::IsolatedIndexedTxModel::Builder::withSymbol(handle, symbol);
}

JavaObjectHandle<IndexedTxModelBuilderTag>
IndexedTxModelImpl::Builder::withListenerImpl(const JavaObjectHandle<IndexedTxModelBuilderTag> &handle,
                                              const JavaObjectHandle<TxModelListenerTag> &listener) {
    return isolated::model::IsolatedIndexedTxModel::Builder::withListener(handle, listener);
}

JavaObjectHandle<IndexedTxModelBuilderTag>
IndexedTxModelImpl::Builder::withSourcesImpl(const JavaObjectHandle<IndexedTxModelBuilderTag> &handle,
                                             void *graalEventSourceList) {
    return isolated::model::IsolatedIndexedTxModel::Builder::withSources(handle, graalEventSourceList);
}

JavaObjectHandle<IndexedTxModelTag>
IndexedTxModelImpl::Builder::buildImpl(const JavaObjectHandle<IndexedTxModelBuilderTag> &handle) {
    return isolated::model::IsolatedIndexedTxModel::Builder::build(handle);
}

JavaObjectHandle<IndexedTxModelBuilderTag> IndexedTxModelImpl::newBuilderImpl(const EventTypeEnum &eventType) {
    return isolated::model::IsolatedIndexedTxModel::newBuilder(eventType);
}

bool IndexedTxModelImpl::isBatchProcessingImpl(const JavaObjectHandle<IndexedTxModelTag> &handle) {
    return isolated::model::IsolatedIndexedTxModel::isBatchProcessing(handle);
}

bool IndexedTxModelImpl::isSnapshotProcessingImpl(const JavaObjectHandle<IndexedTxModelTag> &handle) {
    return isolated::model::IsolatedIndexedTxModel::isSnapshotProcessing(handle);
}

void IndexedTxModelImpl::attachImpl(const JavaObjectHandle<IndexedTxModelTag> &handle,
                                    const std::shared_ptr<DXFeed> &feed) {
    isolated::model::IsolatedIndexedTxModel::attach(handle, feed->handle_);
}

void IndexedTxModelImpl::detachImpl(const JavaObjectHandle<IndexedTxModelTag> &handle,
                                    const std::shared_ptr<DXFeed> &feed) {
    isolated::model::IsolatedIndexedTxModel::detach(handle, feed->handle_);
}

void IndexedTxModelImpl::closeImpl(const JavaObjectHandle<IndexedTxModelTag> &handle) {
    isolated::model::IsolatedIndexedTxModel::close(handle);
}

std::unordered_set<EventSourceWrapper>
IndexedTxModelImpl::getSourcesImpl(const JavaObjectHandle<IndexedTxModelTag> &handle) {
    std::unique_ptr<void, decltype(&releaseGraalSources)> graalSources{
        isolated::model::IsolatedIndexedTxModel::getSources(handle), releaseGraalSources};

    if (graalSources == nullptr) {
        return {};
    }

    auto sources = EventSourceWrapper::ListUtils::fromGraalList(graalSources.get());

    return {sources.begin(), sources.end()};
}

void IndexedTxModelImpl::setSourcesImpl(const JavaObjectHandle<IndexedTxModelTag> &handle, void *graalEventSourceList) {
    isolated::model::IsolatedIndexedTxModel::setSources(handle, graalEventSourceList);
}

DXFCPP_END_NAMESPACE