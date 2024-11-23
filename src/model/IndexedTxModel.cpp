// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/model/IndexedTxModel.hpp>

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api/DXFeed.hpp>
#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedTxModel.hpp>
#include <utility>

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

JavaObjectHandle<IndexedTxModel::Builder> IndexedTxModel::Builder::withSourcesImpl(void *graalEventSourceList) const {
    return isolated::model::IsolatedIndexedTxModel::Builder::withSources(handle_, graalEventSourceList);
}

JavaObjectHandle<IndexedTxModel::Builder>
IndexedTxModel::Builder::withListenerImpl(const JavaObjectHandle<TxModelListenerTag> &listener) const {
    return isolated::model::IsolatedIndexedTxModel::Builder::withListener(handle_, listener);
}

IndexedTxModel::Builder::Builder(LockExternalConstructionTag, JavaObjectHandle<Builder> &&handle)
    : handle_(std::move(handle)) {
}

IndexedTxModel::Builder::Builder(LockExternalConstructionTag, JavaObjectHandle<Builder> &&handle,
                                 std::shared_ptr<TxModelListenerCommon> listener)
    : handle_(std::move(handle)), listener_(std::move(listener)) {
}

std::shared_ptr<IndexedTxModel::Builder> IndexedTxModel::Builder::withBatchProcessing(bool isBatchProcessing) const {
    return createShared(
        isolated::model::IsolatedIndexedTxModel::Builder::withBatchProcessing(handle_, isBatchProcessing));
}

std::shared_ptr<IndexedTxModel::Builder>
IndexedTxModel::Builder::withSnapshotProcessing(bool isSnapshotProcessing) const {
    return createShared(
        isolated::model::IsolatedIndexedTxModel::Builder::withSnapshotProcessing(handle_, isSnapshotProcessing));
}

std::shared_ptr<IndexedTxModel::Builder> IndexedTxModel::Builder::withFeed(std::shared_ptr<DXFeed> feed) const {
    return createShared(isolated::model::IsolatedIndexedTxModel::Builder::withFeed(handle_, feed->handle_));
}

std::shared_ptr<IndexedTxModel::Builder> IndexedTxModel::Builder::withSymbol(const SymbolWrapper &symbol) const {
    return createShared(isolated::model::IsolatedIndexedTxModel::Builder::withSymbol(handle_, symbol));
}

std::shared_ptr<IndexedTxModel::Builder>
IndexedTxModel::Builder::withSources(std::initializer_list<EventSourceWrapper> collection) const {
    return withSources(collection.begin(), collection.end());
}

std::shared_ptr<IndexedTxModel> IndexedTxModel::Builder::build() const {
    return IndexedTxModel::createShared(isolated::model::IsolatedIndexedTxModel::Builder::build(handle_),
                                        listener_);
}

void IndexedTxModel::setSourcesImpl(void *graalEventSourceList) const {
    isolated::model::IsolatedIndexedTxModel::setSources(handle_, graalEventSourceList);
}

IndexedTxModel::IndexedTxModel(LockExternalConstructionTag, JavaObjectHandle<IndexedTxModel> &&handle)
    : handle_(std::move(handle)) {
}

IndexedTxModel::IndexedTxModel(LockExternalConstructionTag, JavaObjectHandle<IndexedTxModel> &&handle,
                               std::shared_ptr<TxModelListenerCommon> listener)
    : handle_(std::move(handle)), listener_(std::move(listener)) {
}

IndexedTxModel::~IndexedTxModel() noexcept {
    close();
}

std::shared_ptr<IndexedTxModel::Builder> IndexedTxModel::newBuilder(const EventTypeEnum &eventType) {
    return Builder::createShared(isolated::model::IsolatedIndexedTxModel::newBuilder(eventType));
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

std::string IndexedTxModel::toString() const {
    return isolated::internal::IsolatedObject::toString(handle_.get());
}

std::size_t IndexedTxModel::hashCode() const {
    return isolated::internal::IsolatedObject::hashCode(handle_.get());
}

bool IndexedTxModel::operator==(const IndexedTxModel &other) const noexcept {
    return isolated::internal::IsolatedObject::equals(handle_.get(), other.handle_.get()) == 0;
}

DXFCPP_END_NAMESPACE