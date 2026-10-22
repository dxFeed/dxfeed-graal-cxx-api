// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/model/IndexedEventTxModel.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedEventTxModel.hpp"

#include <dxfg_api.h>

#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct IndexedEventTxModelImpl::Listener {
    // Shared with the calls in progress: a listener released from its own callback is not called after that call.
    std::shared_ptr<EventsReceived> eventsReceived;
    // Declared last to be released first: releasing the native listener deactivates it (waits for its calls in
    // progress on other threads), so `this` (the user data of the native listener) is not used after that.
    JavaObjectHandle<IndexedEventTxModelListenerTag> handle;

    explicit Listener(EventsReceived &&eventsReceived)
        : eventsReceived(std::make_shared<EventsReceived>(std::move(eventsReceived))) {
    }

    // typedef void (*dxfg_IndexedEventTxModel_Listener_eventsReceived_f)(graal_isolatethread_t *thread,
    // dxfg_event_type_list *events, int32_t isSnapshot, void *userData);
    static void onEventsReceived(graal_isolatethread_t * /* thread */, dxfg_event_type_list *events, int32_t isSnapshot,
                                 void *userData) {
        if (!events || !userData) {
            return;
        }

        try {
            // The list belongs to the Graal SDK: it is freed after this call.
            const auto f = static_cast<Listener *>(userData)->eventsReceived;

            (*f)(EventMapper::fromGraalList(events), isSnapshot == 1);
        } catch (...) {
            // An exception must not cross the Graal SDK.
        }
    }
};

JavaObjectHandle<IndexedEventTxModelBuilderTag>
IndexedEventTxModelImpl::newBuilderImpl(const EventTypeEnum &eventType) {
    return isolated::model::IsolatedIndexedEventTxModel::newBuilder(eventType);
}

void IndexedEventTxModelImpl::withFeedImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                           const std::shared_ptr<DXFeed> &feed) {
    if (!feed) {
        throw InvalidArgumentException("The `feed` is nullptr");
    }

    isolated::model::IsolatedIndexedEventTxModel::Builder::withFeed(builder, feed->handle_);
}

void IndexedEventTxModelImpl::withSymbolImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                             const IndexedEventSubscriptionSymbol &symbol) {
    isolated::model::IsolatedIndexedEventTxModel::Builder::withSymbol(builder, symbol);
}

void IndexedEventTxModelImpl::withSymbolImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                             const StringLike &symbol) {
    isolated::model::IsolatedIndexedEventTxModel::Builder::withSymbol(builder, symbol);
}

void IndexedEventTxModelImpl::withSourceImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                             const EventSourceWrapper &source) {
    isolated::model::IsolatedIndexedEventTxModel::Builder::withSource(builder, source);
}

std::shared_ptr<IndexedEventTxModelImpl::Listener>
IndexedEventTxModelImpl::withListenerImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                          EventsReceived eventsReceived) {
    if (!eventsReceived) {
        throw InvalidArgumentException("The `listener` is empty");
    }

    auto listener = std::make_shared<Listener>(std::move(eventsReceived));

    listener->handle = isolated::model::IsolatedIndexedEventTxModel::Listener::create(
        dxfcpp::bit_cast<void *>(&Listener::onEventsReceived), listener.get());
    isolated::model::IsolatedIndexedEventTxModel::Builder::withListener(builder, listener->handle);

    return listener;
}

void IndexedEventTxModelImpl::withEventsBatchLimitImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                                       std::int32_t eventsBatchLimit) {
    isolated::model::IsolatedIndexedEventTxModel::Builder::withEventsBatchLimit(builder, eventsBatchLimit);
}

void IndexedEventTxModelImpl::withAggregationPeriodImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                                        std::int64_t aggregationPeriod) {
    isolated::model::IsolatedIndexedEventTxModel::Builder::withAggregationPeriod(builder, aggregationPeriod);
}

void IndexedEventTxModelImpl::withExecutorImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                               void *executor) {
    isolated::model::IsolatedIndexedEventTxModel::Builder::withExecutor(builder, executor);
}

JavaObjectHandle<IndexedEventTxModelTag>
IndexedEventTxModelImpl::buildImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder) {
    return isolated::model::IsolatedIndexedEventTxModel::Builder::build(builder);
}

IndexedEventSubscriptionSymbol
IndexedEventTxModelImpl::getIndexedEventSubscriptionSymbolImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    return isolated::model::IsolatedIndexedEventTxModel::getIndexedEventSubscriptionSymbol(model);
}

std::string IndexedEventTxModelImpl::getSymbolImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    return isolated::model::IsolatedIndexedEventTxModel::getSymbol(model);
}

EventSourceWrapper IndexedEventTxModelImpl::getSourceImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    return isolated::model::IsolatedIndexedEventTxModel::getSource(model);
}

std::shared_ptr<SubscriptionController>
IndexedEventTxModelImpl::getSubscriptionControllerImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    return SubscriptionController::createShared(
        isolated::model::IsolatedIndexedEventTxModel::getSubscriptionController(model));
}

bool IndexedEventTxModelImpl::isClosedImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    return isolated::model::IsolatedIndexedEventTxModel::isClosed(model);
}

void IndexedEventTxModelImpl::closeImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model) {
    isolated::model::IsolatedIndexedEventTxModel::close(model);
}

DXFCPP_END_NAMESPACE
