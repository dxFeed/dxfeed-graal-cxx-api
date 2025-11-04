// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/candlewebservice/HistoryEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/glossary/CFI.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp"
#include "../../include/dxfeed_graal_cpp_api/logging/Logging.hpp"

DXFCPP_BEGIN_NAMESPACE

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