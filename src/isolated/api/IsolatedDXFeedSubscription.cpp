// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedDXFeedSubscription.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/CallbackBarrier.hpp"
#include "../../internal/Common.hpp"
#include "../IsolatedCommon.hpp"

#include <dxfg_api.h>

#include <memory>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeedSubscription {

namespace {

dxfg_v4_subscription_t *toGraal(const JavaObjectHandle<DXFeedSubscription> &sub, const char *function) {
    if (!sub) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `sub` handle is invalid");
    }

    return static_cast<dxfg_v4_subscription_t *>(sub.get());
}

template <typename T> T *checkNotNull(void *pointer, const char *function, const char *name) {
    if (!pointer) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function + "`. The `" + name +
                                       "` is nullptr");
    }

    return static_cast<T *>(pointer);
}

// Takes the symbols of a list returned by the SDK and releases the list with dxfg_v4_symbol_list_release. A failure to
// release the list is reported by runRelease(), it does not replace the result or an exception of the conversion.
std::vector<SymbolWrapper> takeSymbols(dxfg_v4_symbol_list *list) {
    const auto releaseList = [](dxfg_v4_symbol_list *toRelease) noexcept {
        runRelease("DXFeedSubscription", [toRelease] {
            runGraalV4Function(dxfg_v4_symbol_list_release, toRelease);
        });
    };
    const std::unique_ptr<dxfg_v4_symbol_list, decltype(releaseList)> guard{list, releaseList};

    return SymbolWrapper::SymbolListUtils::fromGraalList(list);
}

} // namespace

/// dxfg_v4_DXFeedSubscription_new
JavaObjectHandle<DXFeedSubscription> create(const EventTypeEnum &eventType) {
    return JavaObjectHandle<DXFeedSubscription>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeedSubscription_new, static_cast<dxfg_v4_event_clazz_t>(eventType.getId())));
}

/// dxfg_v4_DXFeedSubscription_newWithEventTypes
JavaObjectHandle<DXFeedSubscription> create(const std::unique_ptr<EventClassList> &eventClassList) {
    if (!eventClassList) {
        throw InvalidArgumentException("The eventClassList is nullptr");
    }

    return JavaObjectHandle<DXFeedSubscription>(
        runGraalV4FunctionWithNonNullResult(dxfg_v4_DXFeedSubscription_newWithEventTypes,
                                            static_cast<dxfg_v4_event_clazz_list *>(eventClassList->getHandle())));
}

/// dxfg_v4_DXFeedSubscription_isClosed
bool isClosed(const JavaObjectHandle<DXFeedSubscription> &sub) {
    return runGraalV4FunctionWithResult(dxfg_v4_DXFeedSubscription_isClosed,
                                        toGraal(sub, "dxfg_v4_DXFeedSubscription_isClosed")) == 1;
}

/// dxfg_v4_DXFeedSubscription_close
void close(const JavaObjectHandle<DXFeedSubscription> &sub) {
    runGraalV4Function(dxfg_v4_DXFeedSubscription_close, toGraal(sub, "dxfg_v4_DXFeedSubscription_close"));
}

/// dxfg_v4_DXFeedSubscription_clear
void clear(const JavaObjectHandle<DXFeedSubscription> &sub) {
    runGraalV4Function(dxfg_v4_DXFeedSubscription_clear, toGraal(sub, "dxfg_v4_DXFeedSubscription_clear"));
}

/// dxfg_v4_DXFeedSubscription_getSymbols
std::vector<SymbolWrapper> getSymbols(const JavaObjectHandle<DXFeedSubscription> &sub) {
    return takeSymbols(runGraalV4FunctionWithNonNullResult(dxfg_v4_DXFeedSubscription_getSymbols,
                                                           toGraal(sub, "dxfg_v4_DXFeedSubscription_getSymbols")));
}

/// dxfg_v4_DXFeedSubscription_getDecoratedSymbols
std::vector<SymbolWrapper> getDecoratedSymbols(const JavaObjectHandle<DXFeedSubscription> &sub) {
    return takeSymbols(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeedSubscription_getDecoratedSymbols, toGraal(sub, "dxfg_v4_DXFeedSubscription_getDecoratedSymbols")));
}

/// dxfg_v4_DXFeedSubscription_setSymbols
void setSymbols(const JavaObjectHandle<DXFeedSubscription> &sub, void *symbols) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_setSymbols");

    runGraalV4Function(dxfg_v4_DXFeedSubscription_setSymbols, graalSub,
                       checkNotNull<dxfg_v4_symbol_list>(symbols, "dxfg_v4_DXFeedSubscription_setSymbols", "symbols"));
}

/// dxfg_v4_DXFeedSubscription_addSymbol
void addSymbol(const JavaObjectHandle<DXFeedSubscription> &sub, void *symbol) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_addSymbol");

    runGraalV4Function(dxfg_v4_DXFeedSubscription_addSymbol, graalSub,
                       checkNotNull<dxfg_v4_symbol_t>(symbol, "dxfg_v4_DXFeedSubscription_addSymbol", "symbol"));
}

/// dxfg_v4_DXFeedSubscription_addSymbols
void addSymbols(const JavaObjectHandle<DXFeedSubscription> &sub, void *symbols) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_addSymbols");

    runGraalV4Function(dxfg_v4_DXFeedSubscription_addSymbols, graalSub,
                       checkNotNull<dxfg_v4_symbol_list>(symbols, "dxfg_v4_DXFeedSubscription_addSymbols", "symbols"));
}

/// dxfg_v4_DXFeedSubscription_removeSymbol
void removeSymbol(const JavaObjectHandle<DXFeedSubscription> &sub, void *symbol) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_removeSymbol");

    runGraalV4Function(dxfg_v4_DXFeedSubscription_removeSymbol, graalSub,
                       checkNotNull<dxfg_v4_symbol_t>(symbol, "dxfg_v4_DXFeedSubscription_removeSymbol", "symbol"));
}

/// dxfg_v4_DXFeedSubscription_removeSymbols
void removeSymbols(const JavaObjectHandle<DXFeedSubscription> &sub, void *symbols) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_removeSymbols");

    runGraalV4Function(
        dxfg_v4_DXFeedSubscription_removeSymbols, graalSub,
        checkNotNull<dxfg_v4_symbol_list>(symbols, "dxfg_v4_DXFeedSubscription_removeSymbols", "symbols"));
}

/// dxfg_v4_DXFeedSubscription_getAggregationPeriod; an empty handle if the period is not set.
JavaObjectHandle<TimePeriod> getAggregationPeriod(const JavaObjectHandle<DXFeedSubscription> &sub) {
    return JavaObjectHandle<TimePeriod>(runGraalV4FunctionWithResult(
        dxfg_v4_DXFeedSubscription_getAggregationPeriod, toGraal(sub, "dxfg_v4_DXFeedSubscription_getAggregationPeriod")));
}

/// dxfg_v4_DXFeedSubscription_setAggregationPeriod
void setAggregationPeriod(const JavaObjectHandle<DXFeedSubscription> &sub, const JavaObjectHandle<TimePeriod> &period) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_setAggregationPeriod");

    if (!period) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_DXFeedSubscription_setAggregationPeriod`. "
                                       "The `period` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXFeedSubscription_setAggregationPeriod, graalSub,
                       static_cast<dxfg_v4_time_period_t *>(period.get()));
}

/// dxfg_v4_DXFeedSubscription_getAggregationPeriodInfo
std::pair<std::int64_t, std::int64_t> getAggregationPeriodInfo(const JavaObjectHandle<DXFeedSubscription> &sub) {
    std::int64_t min{};
    std::int64_t max{};

    runGraalV4Function(dxfg_v4_DXFeedSubscription_getAggregationPeriodInfo,
                       toGraal(sub, "dxfg_v4_DXFeedSubscription_getAggregationPeriodInfo"), &min, &max);

    return {min, max};
}

/// dxfg_v4_DXFeedSubscription_getTotalDroppedEvents (experimental)
std::int64_t getTotalDroppedEvents(const JavaObjectHandle<DXFeedSubscription> &sub) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_getTotalDroppedEvents");

    Isolate::checkAdvancedRevision();

    return runGraalV4FunctionWithResult(dxfg_v4_DXFeedSubscription_getTotalDroppedEvents, graalSub);
}

/// dxfg_v4_DXFeedSubscription_addEventListener
void addEventListener(const JavaObjectHandle<DXFeedSubscription> &sub,
                      const JavaObjectHandle<dxfcpp::DXFeedEventListener> &listener) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_addEventListener");

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_DXFeedSubscription_addEventListener`. The "
                                       "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXFeedSubscription_addEventListener, graalSub,
                       static_cast<dxfg_v4_feed_event_listener_t *>(listener.get()));
}

/// dxfg_v4_DXFeedSubscription_addChangeListener
void addChangeListener(const JavaObjectHandle<DXFeedSubscription> &sub,
                       const JavaObjectHandle<ObservableSubscriptionChangeListener> &listener) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_addChangeListener");

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_DXFeedSubscription_addChangeListener`. The "
                                       "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXFeedSubscription_addChangeListener, graalSub,
                       static_cast<dxfg_v4_observable_subscription_change_listener_t *>(listener.get()));
}

/// dxfg_v4_DXFeedSubscription_removeChangeListener
void removeChangeListener(const JavaObjectHandle<DXFeedSubscription> &sub,
                          const JavaObjectHandle<ObservableSubscriptionChangeListener> &listener) {
    auto *graalSub = toGraal(sub, "dxfg_v4_DXFeedSubscription_removeChangeListener");

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_DXFeedSubscription_removeChangeListener`. "
                                       "The `listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXFeedSubscription_removeChangeListener, graalSub,
                       static_cast<dxfg_v4_observable_subscription_change_listener_t *>(listener.get()));
}

/// dxfg_v4_DXFeedSubscription_getEventsBatchLimit
std::int32_t getEventsBatchLimit(const JavaObjectHandle<DXFeedSubscription> &sub) {
    return runGraalV4FunctionWithResult(dxfg_v4_DXFeedSubscription_getEventsBatchLimit,
                                        toGraal(sub, "dxfg_v4_DXFeedSubscription_getEventsBatchLimit"));
}

/// dxfg_v4_DXFeedSubscription_setEventsBatchLimit
void setEventsBatchLimit(const JavaObjectHandle<DXFeedSubscription> &sub, std::int32_t eventsBatchLimit) {
    runGraalV4Function(dxfg_v4_DXFeedSubscription_setEventsBatchLimit,
                       toGraal(sub, "dxfg_v4_DXFeedSubscription_setEventsBatchLimit"), eventsBatchLimit);
}

namespace DXFeedEventListener {

/// dxfg_v4_DXFeedEventListener_new
JavaObjectHandle<dxfcpp::DXFeedEventListener> create(void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException("Unable to create DXFeedEventListener. The `userFunc` parameter is nullptr");
    }

    return JavaObjectHandle<dxfcpp::DXFeedEventListener>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXFeedEventListener_new, dxfcpp::bit_cast<dxfg_v4_feed_event_listener_function>(userFunc), userData));
}

} // namespace DXFeedEventListener

} // namespace isolated::api::IsolatedDXFeedSubscription

/// dxfg_v4_DXFeedTimeSeriesSubscription_setFromTime
void isolated::api::IsolatedDXFeedTimeSeriesSubscription::setFromTime(const JavaObjectHandle<DXFeedSubscription> &sub,
                                                                      std::int64_t fromTime) {
    if (!sub) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_DXFeedTimeSeriesSubscription_setFromTime`. "
                                       "The `sub` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXFeedTimeSeriesSubscription_setFromTime,
                       static_cast<dxfg_v4_time_series_subscription_t *>(sub.get()), fromTime);
}

DXFCPP_END_NAMESPACE
