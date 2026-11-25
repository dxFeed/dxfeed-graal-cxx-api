// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0


#include "../../include/dxfeed_graal_cpp_api/model/TimeSeriesTxModel.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../include/dxfeed_graal_cpp_api/exceptions/ErrorManager.hpp"
#include "../../include/dxfeed_graal_cpp_api/util/Time.hpp"
#include "../internal/utils/StringUtils.hpp"
#include "../isolated/event/IsolatedEventType.hpp"
#include "../isolated/model/IsolatedTimeSeriesTxModel.hpp"

#include <limits>

DXFCPP_BEGIN_NAMESPACE

namespace {

// A fromTime that is now or in the future gives no events until that time: the user gets a warning, not silence.
// Long.MAX_VALUE means "no subscription" and is not warned about. For an existing model, setting the time that is
// already set changes nothing, so it is not warned about again.
void warnIfFromTimeIsNotInPast(std::int64_t fromTime,
                               const JavaObjectHandle<TimeSeriesTxModelTag> *model = nullptr) noexcept {
    try {
        if (fromTime == std::numeric_limits<std::int64_t>::max() || fromTime < now()) {
            return;
        }

        if (model != nullptr && isolated::model::IsolatedTimeSeriesTxModel::getFromTime(*model) == fromTime) {
            return;
        }

        ErrorManager::reportWarning("TimeSeriesTxModel", "fromTime " + formatTimeStampWithMillis(fromTime) +
                                                             " is not in the past: no events before this time");
    } catch (...) {
        // A warning must not break setting fromTime.
    }
}

} // namespace

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
TimeSeriesTxModelImpl::Builder::withBatchProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                                                        bool isBatchProcessing) {
    return isolated::model::IsolatedTimeSeriesTxModel::Builder::withBatchProcessing(handle, isBatchProcessing);
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
TimeSeriesTxModelImpl::Builder::withSnapshotProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                                                           bool isSnapshotProcessing) {
    return isolated::model::IsolatedTimeSeriesTxModel::Builder::withSnapshotProcessing(handle, isSnapshotProcessing);
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
TimeSeriesTxModelImpl::Builder::withFeedImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                                             const std::shared_ptr<DXFeed> &feed) {
    return isolated::model::IsolatedTimeSeriesTxModel::Builder::withFeed(handle, feed->handle_);
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
TimeSeriesTxModelImpl::Builder::withSymbolImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                                               const SymbolWrapper &symbol) {
    return isolated::model::IsolatedTimeSeriesTxModel::Builder::withSymbol(handle, symbol);
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
TimeSeriesTxModelImpl::Builder::withListenerImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                                                 const JavaObjectHandle<TxModelListenerTag> &listener) {
    return isolated::model::IsolatedTimeSeriesTxModel::Builder::withListener(handle, listener);
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag>
TimeSeriesTxModelImpl::Builder::withFromTimeImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                                                 std::int64_t fromTime) {
    warnIfFromTimeIsNotInPast(fromTime);

    return isolated::model::IsolatedTimeSeriesTxModel::Builder::withFromTime(handle, fromTime);
}

JavaObjectHandle<TimeSeriesTxModelTag>
TimeSeriesTxModelImpl::Builder::buildImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle) {
    return isolated::model::IsolatedTimeSeriesTxModel::Builder::build(handle);
}

JavaObjectHandle<TimeSeriesTxModelBuilderTag> TimeSeriesTxModelImpl::newBuilderImpl(const EventTypeEnum &eventType) {
    return isolated::model::IsolatedTimeSeriesTxModel::newBuilder(eventType);
}

bool TimeSeriesTxModelImpl::isBatchProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle) {
    return isolated::model::IsolatedTimeSeriesTxModel::isBatchProcessing(handle);
}

bool TimeSeriesTxModelImpl::isSnapshotProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle) {
    return isolated::model::IsolatedTimeSeriesTxModel::isSnapshotProcessing(handle);
}

void TimeSeriesTxModelImpl::attachImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle,
                                       const std::shared_ptr<DXFeed> &feed) {
    isolated::model::IsolatedTimeSeriesTxModel::attach(handle, feed->handle_);
}

void TimeSeriesTxModelImpl::detachImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle,
                                       const std::shared_ptr<DXFeed> &feed) {
    isolated::model::IsolatedTimeSeriesTxModel::detach(handle, feed->handle_);
}

void TimeSeriesTxModelImpl::closeImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle) {
    isolated::model::IsolatedTimeSeriesTxModel::close(handle);
}

std::int64_t TimeSeriesTxModelImpl::getFromTimeImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle) {
    return isolated::model::IsolatedTimeSeriesTxModel::getFromTime(handle);
}

void TimeSeriesTxModelImpl::setFromTimeImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle,
                                            std::int64_t fromTime) {
    warnIfFromTimeIsNotInPast(fromTime, &handle);

    isolated::model::IsolatedTimeSeriesTxModel::setFromTime(handle, fromTime);
}

DXFCPP_END_NAMESPACE