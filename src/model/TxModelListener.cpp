// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/event/EventMapper.hpp"

#include <dxfeed_graal_cpp_api/internal/Id.hpp>
#include <dxfeed_graal_cpp_api/internal/context/ApiContext.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedTxModelListener.hpp>
#include <dxfeed_graal_cpp_api/model/TxModelListener.hpp>

struct graal_isolatethread_t;
struct dxfg_indexed_event_source_t;
struct dxfg_event_type_list;

DXFCPP_BEGIN_NAMESPACE

struct TxModelListener::Impl {
    // typedef void (*dxfg_TxModelListener_function_eventsReceived)(graal_isolatethread_t* thread,
    // dxfg_indexed_event_source_t* source, dxfg_event_type_list* events, int32_t isSnapshot, void* user_data);
    static void onEventsReceived(graal_isolatethread_t * /* thread */, dxfg_indexed_event_source_t *source,
                                 dxfg_event_type_list *events, int32_t isSnapshot, void *userData) {
        if (!source || !events) {
            return;
        }

        auto id = Id<TxModelListener>::from(userData);
        auto listener = ApiContext::getInstance()->getManager<EntityManager<TxModelListener>>()->getEntity(id);

        if (!listener) {
            return;
        }

        listener->onEventsReceived_(IndexedEventSource::fromGraal(source), EventMapper::fromGraalList(events),
                                    isSnapshot == 1);
    }
};

TxModelListener::TxModelListener(LockExternalConstructionTag) : handle_{}, impl_{std::make_unique<Impl>()} {
}

TxModelListener::~TxModelListener() noexcept {
}

std::shared_ptr<TxModelListener> TxModelListener::create(
    std::function<void(const IndexedEventSource & /* source */,
                       const std::vector<std::shared_ptr<EventType>> & /* events */, bool /* isSnapshot */)>
        onEventsReceived) {
    auto listener = createShared();
    auto id = ApiContext::getInstance()->getManager<EntityManager<TxModelListener>>()->registerEntity(listener);

    listener->handle_ = isolated::model::IsolatedTxModelListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onEventsReceived), dxfcpp::bit_cast<void *>(id.getValue()));
    listener->onEventsReceived_ += std::move(onEventsReceived);

    return listener;
}

const JavaObjectHandle<TxModelListenerTag> &TxModelListener::getHandle() const {
    std::lock_guard guard{mutex_};

    return handle_;
}

DXFCPP_END_NAMESPACE
