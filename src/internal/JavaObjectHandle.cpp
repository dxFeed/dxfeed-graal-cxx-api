// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/candlewebservice/HistoryEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/glossary/CFI.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp"
#include "../../include/dxfeed_graal_cpp_api/logging/Logging.hpp"

#include <mutex>

DXFCPP_BEGIN_NAMESPACE

namespace {

/** Stores the intrusive handle list and its synchronization primitive. */
struct JavaObjectHandleRegistryState final {
    std::mutex mutex{};
    JavaObjectHandleState *first{};
};

JavaObjectHandleRegistryState &getJavaObjectHandleRegistryState() noexcept {
    static JavaObjectHandleRegistryState state{};

    return state;
}

} // namespace

void JavaObjectHandleRegistry::unlinkHandle(JavaObjectHandleState &state) noexcept {
    if (!state.registered_) {
        return;
    }

    auto &registry = getJavaObjectHandleRegistryState();

    if (state.previous_) {
        state.previous_->next_ = state.next_;
    } else {
        registry.first = state.next_;
    }

    if (state.next_) {
        state.next_->previous_ = state.previous_;
    }

    state.previous_ = nullptr;
    state.next_ = nullptr;
    state.registered_ = false;
}

void JavaObjectHandleRegistry::linkHandle(JavaObjectHandleState &state) noexcept {
    auto &registry = getJavaObjectHandleRegistryState();

    state.previous_ = nullptr;
    state.next_ = registry.first;
    state.registered_ = true;

    if (registry.first) {
        registry.first->previous_ = &state;
    }

    registry.first = &state;
}

JavaObjectHandleRegistry &JavaObjectHandleRegistry::getInstance() noexcept {
    // Construct the registry state while the isolate is being constructed. This
    // guarantees that it remains alive until after the isolate is torn down.
    ignoreUnused(getJavaObjectHandleRegistryState());
    static JavaObjectHandleRegistry instance{};

    return instance;
}

void JavaObjectHandleRegistry::registerHandle(JavaObjectHandleState &state) noexcept {
    auto &registry = getJavaObjectHandleRegistryState();
    const std::lock_guard lock{registry.mutex};

    if (!state.registered_ && state.handle_.load(std::memory_order_relaxed)) {
        linkHandle(state);
    }
}

void JavaObjectHandleRegistry::resetHandle(JavaObjectHandleState &state, void *handle) noexcept {
    void *oldHandle{};

    {
        auto &registry = getJavaObjectHandleRegistryState();
        const std::lock_guard lock{registry.mutex};

        unlinkHandle(state);
        oldHandle = state.handle_.exchange(handle, std::memory_order_acq_rel);

        if (handle) {
            linkHandle(state);
        }
    }

    if (oldHandle) {
        state.deleter_(oldHandle);
    }
}

void JavaObjectHandleRegistry::moveHandle(JavaObjectHandleState &from, JavaObjectHandleState &to) noexcept {
    if (&from == &to) {
        return;
    }

    resetHandle(to, nullptr);

    auto &registry = getJavaObjectHandleRegistryState();
    const std::lock_guard lock{registry.mutex};
    const auto handle = from.handle_.exchange(nullptr, std::memory_order_acq_rel);

    unlinkHandle(from);
    to.handle_.store(handle, std::memory_order_release);

    if (handle) {
        linkHandle(to);
    }
}

void JavaObjectHandleRegistry::releaseAll() noexcept {
    while (true) {
        JavaObjectHandleState::Deleter deleter{};
        void *handle{};

        {
            auto &registry = getJavaObjectHandleRegistryState();
            const std::lock_guard lock{registry.mutex};

            if (!registry.first) {
                return;
            }

            auto &state = *registry.first;
            unlinkHandle(state);
            handle = state.handle_.exchange(nullptr, std::memory_order_acq_rel);
            deleter = state.deleter_;
        }

        if (handle) {
            deleter(handle);
        }
    }
}

JavaObjectHandleState::JavaObjectHandleState(void *handle, Deleter deleter) noexcept
    : handle_{handle}, deleter_{deleter} {
    JavaObjectHandleRegistry::registerHandle(*this);
}

JavaObjectHandleState::~JavaObjectHandleState() noexcept {
    reset();
}

void JavaObjectHandleState::moveFrom(JavaObjectHandleState &other) noexcept {
    JavaObjectHandleRegistry::moveHandle(other, *this);
}

void JavaObjectHandleState::reset(void *handle) noexcept {
    JavaObjectHandleRegistry::resetHandle(*this, handle);
}

struct PriceIncrements;
struct CFI;
struct AdditionalUnderlyings;
struct HistoryEndpoint;
struct TimeSeriesTxModelBuilderTag;
struct TimeSeriesTxModelTag;
struct IndexedTxModelBuilderTag;
struct IndexedTxModelTag;
struct TxModelListenerTag;
struct AuthToken;
struct DXPublisherObservableSubscription;
struct TimeFormat;
struct DayFilter;
struct Session;
struct Day;
struct Schedule;
class IterableInstrumentProfile;
class InstrumentProfileConnection;
class InstrumentProfileCollector;
class InstrumentProfileReader;
struct InstrumentProfile;
class DXFeedSubscription;
struct DXFeed;
struct DXPublisher;

std::string JavaObject::toString(void *handle) {
    return isolated::internal::IsolatedObject::toString(handle);
}

std::size_t JavaObject::hashCode(void *handle) noexcept {
    try {
        return isolated::internal::IsolatedObject::hashCode(handle);
    } catch (...) {
        return 0;
    }
}

bool JavaObject::equals(void *objectHandle1, void *objectHandle2) {
    return isolated::internal::IsolatedObject::equals(objectHandle1, objectHandle2) == 1;
}

template struct JavaObjectHandle<DXEndpoint>;
template struct JavaObjectHandle<DXEndpoint::Builder>;
template struct JavaObjectHandle<DXEndpointStateChangeListener>;

template struct JavaObjectHandle<DXFeed>;
template struct JavaObjectHandle<DXPublisher>;
template struct JavaObjectHandle<DXFeedSubscription>;
template struct JavaObjectHandle<DXFeedEventListener>;

template struct JavaObjectHandle<InstrumentProfile>;
template struct JavaObjectHandle<InstrumentProfileReader>;
template struct JavaObjectHandle<InstrumentProfileCollector>;
template struct JavaObjectHandle<InstrumentProfileConnection>;
template struct JavaObjectHandle<IpfPropertyChangeListener>;
template struct JavaObjectHandle<InstrumentProfileUpdateListener>;
template struct JavaObjectHandle<IterableInstrumentProfile>;

template struct JavaObjectHandle<OnDemandService>;

template struct JavaObjectHandle<Schedule>;
template struct JavaObjectHandle<Day>;
template struct JavaObjectHandle<Session>;
template struct JavaObjectHandle<SessionFilter>;
template struct JavaObjectHandle<DayFilter>;

template struct JavaObjectHandle<TimeFormat>;

template struct JavaObjectHandle<ObservableSubscriptionChangeListener>;
template struct JavaObjectHandle<DXPublisherObservableSubscription>;

template struct JavaObjectHandle<PromiseImpl>;
template struct JavaObjectHandleList<PromiseListImpl>;
template struct JavaObjectHandle<EventPromiseImpl>;
template struct JavaObjectHandle<EventsPromiseImpl>;

template struct JavaObjectHandle<TimePeriod>;

template struct JavaObjectHandle<AuthToken>;

template struct JavaObjectHandle<TxModelListenerTag>;
template struct JavaObjectHandle<IndexedTxModelTag>;
template struct JavaObjectHandle<IndexedTxModelBuilderTag>;
template struct JavaObjectHandle<TimeSeriesTxModelTag>;
template struct JavaObjectHandle<TimeSeriesTxModelBuilderTag>;

template struct JavaObjectHandle<ExecutorTag>;

template struct JavaObjectHandle<Logging::ListenerTag>;

template struct JavaObjectHandle<HistoryEndpoint>;
template struct JavaObjectHandle<HistoryEndpoint::Builder>;

template struct JavaObjectHandle<AdditionalUnderlyings>;
template struct JavaObjectHandle<CFI>;
template struct JavaObjectHandle<CFI::Attribute>;
template struct JavaObjectHandle<CFI::Value>;
template struct JavaObjectHandle<PriceIncrements>;

DXFCPP_END_NAMESPACE
