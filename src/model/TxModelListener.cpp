// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

#include <dxfeed_graal_cpp_api/model/TxModelListener.hpp>

#include <dxfeed_graal_cpp_api/event/EventMapper.hpp>
#include <dxfeed_graal_cpp_api/internal/Id.hpp>
#include <dxfeed_graal_cpp_api/internal/context/ApiContext.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp>
#include <dxfeed_graal_cpp_api/isolated/model/IsolatedTxModelListener.hpp>

struct dxfg_indexed_event_source_t;
struct dxfg_event_type_list;

DXFCPP_BEGIN_NAMESPACE

struct TxModelListenerCommon::Impl {
    // typedef void (*dxfg_TxModelListener_function_eventsReceived)(graal_isolatethread_t* thread,
    // dxfg_indexed_event_source_t* source, dxfg_event_type_list* events, int32_t isSnapshot, void* user_data);
    static void onEventsReceived(graal_isolatethread_t * /* thread */, dxfg_indexed_event_source_t *source,
                                 dxfg_event_type_list *events, int32_t isSnapshot, void *userData) {
        if (!source || !events) {
            return;
        }

        auto id = Id<TxModelListenerTag>::from(userData);
        auto listener = ApiContext::getInstance()
                            ->getManager<EntityManager<TxModelListenerCommon, TxModelListenerTag>>()
                            ->getEntity(id);

        if (!listener) {
            return;
        }

        listener->onEventsReceived_(IndexedEventSource::fromGraal(source), EventMapper::fromGraalList(events),
                                    isSnapshot == 1);
    }
};

TxModelListenerCommon::TxModelListenerCommon() : handle_{}, impl_{std::make_unique<Impl>()} {
}

TxModelListenerCommon::~TxModelListenerCommon() noexcept {
}

const JavaObjectHandle<TxModelListenerTag> &TxModelListenerCommon::getHandle() const {
    std::lock_guard guard{mutex_};

    return handle_;
}

std::string TxModelListenerCommon::toString() const {
    return isolated::internal::IsolatedObject::toString(handle_.get());
}

std::size_t TxModelListenerCommon::hashCode() const {
    return isolated::internal::IsolatedObject::hashCode(handle_.get());
}

bool TxModelListenerCommon::operator==(const TxModelListenerCommon &other) const noexcept {
    return isolated::internal::IsolatedObject::equals(handle_.get(), other.handle_.get()) == 0;
}

void TxModelListenerCommon::createHandle(Id<TxModelListenerTag> id) {
    std::lock_guard guard{mutex_};

    handle_ = isolated::model::IsolatedTxModelListener::create(dxfcpp::bit_cast<void *>(&Impl::onEventsReceived),
                                                               dxfcpp::bit_cast<void *>(id.getValue()));
}

DXFCPP_END_NAMESPACE
