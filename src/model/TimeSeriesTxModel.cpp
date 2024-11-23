// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/model/TimeSeriesTxModel.hpp>

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api/DXFeed.hpp>
#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedTimeSeriesTxModel.hpp>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

JavaObjectHandle<TimeSeriesTxModel::Builder>
TimeSeriesTxModel::Builder::withListenerImpl(const JavaObjectHandle<TxModelListenerTag> &listener) const {
    return isolated::model::IsolatedTimeSeriesTxModel::Builder::withListener(handle_, listener);
}

TimeSeriesTxModel::Builder::Builder(LockExternalConstructionTag, JavaObjectHandle<Builder> &&handle)
    : handle_(std::move(handle)) {
}

TimeSeriesTxModel::Builder::Builder(LockExternalConstructionTag, JavaObjectHandle<Builder> &&handle,
                                    std::shared_ptr<TxModelListenerCommon> listener)
    : handle_(std::move(handle)), listener_(std::move(listener)) {
}

std::shared_ptr<TimeSeriesTxModel::Builder>
TimeSeriesTxModel::Builder::withBatchProcessing(bool isBatchProcessing) const {
    return createShared(
        isolated::model::IsolatedTimeSeriesTxModel::Builder::withBatchProcessing(handle_, isBatchProcessing));
}

std::shared_ptr<TimeSeriesTxModel::Builder>
TimeSeriesTxModel::Builder::withSnapshotProcessing(bool isSnapshotProcessing) const {
    return createShared(
        isolated::model::IsolatedTimeSeriesTxModel::Builder::withSnapshotProcessing(handle_, isSnapshotProcessing));
}

std::shared_ptr<TimeSeriesTxModel::Builder> TimeSeriesTxModel::Builder::withFeed(std::shared_ptr<DXFeed> feed) const {
    return createShared(isolated::model::IsolatedTimeSeriesTxModel::Builder::withFeed(handle_, feed->handle_));
}

std::shared_ptr<TimeSeriesTxModel::Builder> TimeSeriesTxModel::Builder::withSymbol(const SymbolWrapper &symbol) const {
    return createShared(isolated::model::IsolatedTimeSeriesTxModel::Builder::withSymbol(handle_, symbol));
}

std::shared_ptr<TimeSeriesTxModel::Builder> TimeSeriesTxModel::Builder::withFromTime(std::int64_t fromTime) const {
    return createShared(isolated::model::IsolatedTimeSeriesTxModel::Builder::withFromTime(handle_, fromTime));
}

std::shared_ptr<TimeSeriesTxModel> TimeSeriesTxModel::Builder::build() const {
    return TimeSeriesTxModel::createShared(isolated::model::IsolatedTimeSeriesTxModel::Builder::build(handle_),
                                           listener_);
}

TimeSeriesTxModel::TimeSeriesTxModel(LockExternalConstructionTag, JavaObjectHandle<TimeSeriesTxModel> &&handle)
    : handle_(std::move(handle)) {
}

TimeSeriesTxModel::TimeSeriesTxModel(LockExternalConstructionTag, JavaObjectHandle<TimeSeriesTxModel> &&handle,
                                     std::shared_ptr<TxModelListenerCommon> listener)
    : handle_(std::move(handle)), listener_(std::move(listener)) {
}

TimeSeriesTxModel::~TimeSeriesTxModel() noexcept {
    close();
}

std::shared_ptr<TimeSeriesTxModel::Builder> TimeSeriesTxModel::newBuilder(const EventTypeEnum &eventType) {
    return Builder::createShared(isolated::model::IsolatedTimeSeriesTxModel::newBuilder(eventType));
}

bool TimeSeriesTxModel::isBatchProcessing() const {
    return isolated::model::IsolatedTimeSeriesTxModel::isBatchProcessing(handle_);
}

bool TimeSeriesTxModel::isSnapshotProcessing() const {
    return isolated::model::IsolatedTimeSeriesTxModel::isSnapshotProcessing(handle_);
}

void TimeSeriesTxModel::attach(std::shared_ptr<DXFeed> feed) const {
    isolated::model::IsolatedTimeSeriesTxModel::attach(handle_, feed->handle_);
}

void TimeSeriesTxModel::detach(std::shared_ptr<DXFeed> feed) const {
    isolated::model::IsolatedTimeSeriesTxModel::detach(handle_, feed->handle_);
}

void TimeSeriesTxModel::close() const {
    isolated::model::IsolatedTimeSeriesTxModel::close(handle_);
}

std::int64_t TimeSeriesTxModel::getFromTime() const {
    return isolated::model::IsolatedTimeSeriesTxModel::getFromTime(handle_);
}

void TimeSeriesTxModel::setFromTime(std::int64_t fromTime) const {
    isolated::model::IsolatedTimeSeriesTxModel::setFromTime(handle_, fromTime);
}

std::string TimeSeriesTxModel::toString() const {
    return isolated::internal::IsolatedObject::toString(handle_.get());
}

std::size_t TimeSeriesTxModel::hashCode() const {
    return isolated::internal::IsolatedObject::hashCode(handle_.get());
}

bool TimeSeriesTxModel::operator==(const TimeSeriesTxModel &other) const noexcept {
    return isolated::internal::IsolatedObject::equals(handle_.get(), other.handle_.get()) == 0;
}

DXFCPP_END_NAMESPACE