// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeedSubscription.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeedSubscription {
JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */
create(/* dxfg_event_clazz_t */ const EventTypeEnum &eventType) {
    return JavaObjectHandle<dxfcpp::DXFeedSubscription>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeedSubscription_new, dxfcpp::bit_cast<dxfg_event_clazz_t>(eventType.getId())));
}

JavaObjectHandle<DXFeedSubscription> /* dxfg_subscription_t* */
create(/* dxfg_event_clazz_list_t * */ const std::unique_ptr<EventClassList> &eventClassList) {
    if (!eventClassList) {
        throw InvalidArgumentException("The eventClassList is nullptr");
    }

    return JavaObjectHandle<DXFeedSubscription>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeedSubscription_new2, static_cast<dxfg_event_clazz_list_t *>(eventClassList->getHandle())));
}

bool /* int32_t */ isClosed(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_isClosed`. The `sub` handle is invalid");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_isClosed,
                                                  static_cast<dxfg_subscription_t *>(sub.get())) == 1;
}

void /* int32_t */ close(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_close`. The `sub` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_close,
                                           static_cast<dxfg_subscription_t *>(sub.get()));
}

void /* int32_t */ clear(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_clear`. The `sub` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_clear,
                                           static_cast<dxfg_subscription_t *>(sub.get()));
}

std::vector<SymbolWrapper> /* dxfg_symbol_list* */
getSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_getSymbols`. The `sub` handle is invalid");
    }

    dxfg_symbol_list *list = runGraalFunctionAndThrowIfNullptr(dxfg_DXFeedSubscription_getSymbols,
                                                               static_cast<dxfg_subscription_t *>(sub.get()));

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(static_cast<void *>(list));

    runGraalFunctionAndThrowIfLessThanZero(dxfg_CList_symbol_release, list);

    return result;
}

std::vector<SymbolWrapper> /* dxfg_symbol_list* */
getDecoratedSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_getDecoratedSymbols`. The `sub` handle is invalid");
    }

    dxfg_symbol_list *list = runGraalFunctionAndThrowIfNullptr(dxfg_DXFeedSubscription_getDecoratedSymbols,
                                                               static_cast<dxfg_subscription_t *>(sub.get()));

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(static_cast<void *>(list));

    runGraalFunctionAndThrowIfLessThanZero(dxfg_CList_symbol_release, list);

    return result;
}

void /* int32_t */ setSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                              /* dxfg_symbol_list * */ void *symbols) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_setSymbols`. The `sub` handle is invalid");
    }

    if (!symbols) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_setSymbols`. The "
                                    "`symbols` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_setSymbols,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_symbol_list *>(symbols));
}

void /* int32_t */ addSymbol(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                             /* dxfg_symbol_t * */ void *symbol) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_addSymbol`. The `sub` handle is invalid");
    }

    if (!symbol) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_addSymbol`. The "
                                    "`symbol` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_addSymbol,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_symbol_t *>(symbol));
}

void /* int32_t */ addSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                              /* dxfg_symbol_list * */ void *symbols) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_addSymbols`. The `sub` handle is invalid");
    }

    if (!symbols) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_addSymbols`. The "
                                    "`symbols` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_addSymbols,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_symbol_list *>(symbols));
}

void /* int32_t */ removeSymbol(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                /* dxfg_symbol_t * */ void *symbol) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_removeSymbol`. The `sub` handle is invalid");
    }

    if (!symbol) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_removeSymbol`. The "
                                    "`symbol` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_removeSymbol,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_symbol_t *>(symbol));
}

void /* int32_t */ removeSymbols(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                 /* dxfg_symbol_list * */ void *symbols) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_removeSymbols`. The `sub` handle is invalid");
    }

    if (!symbols) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_removeSymbols`. The "
                                    "`symbols` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_removeSymbols,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_symbol_list *>(symbols));
}

/* dxfg_time_period_t* */ JavaObjectHandle<TimePeriod>
getAggregationPeriod(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_getAggregationPeriod`. The `sub` handle is invalid");
    }

    return JavaObjectHandle<TimePeriod>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeedSubscription_getAggregationPeriod, static_cast<dxfg_subscription_t *>(sub.get())));
}

/* int32_t */ void setAggregationPeriod(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                        /* dxfg_time_period_t * */ const JavaObjectHandle<TimePeriod> &period) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_setAggregationPeriod`. The `sub` handle is invalid");
    }

    if (!period) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_setAggregationPeriod`. The "
                                    "`period` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_setAggregationPeriod,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_time_period_t *>(period.get()));
}

void /* int32_t */
addEventListener(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                 /* dxfg_feed_event_listener_t * */ const JavaObjectHandle<dxfcpp::DXFeedEventListener> &listener) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_addEventListener`. The `sub` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_addEventListener`. The "
                                    "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_addEventListener,
                                           static_cast<dxfg_subscription_t *>(sub.get()),
                                           static_cast<dxfg_feed_event_listener_t *>(listener.get()));
}

void /* int32_t */ addChangeListener(
    /* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeedSubscription_addChangeListener`. The `sub` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_addChangeListener`. The "
                                    "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_DXFeedSubscription_addChangeListener, static_cast<dxfg_subscription_t *>(sub.get()),
        static_cast<dxfg_observable_subscription_change_listener_t *>(listener.get()));
}

void /* int32_t */ removeChangeListener(
    /* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener) {
    if (!sub) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_removeChangeListener`. "
                                    "The `sub` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_removeChangeListener`. The "
                                    "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_DXFeedSubscription_removeChangeListener, static_cast<dxfg_subscription_t *>(sub.get()),
        static_cast<dxfg_observable_subscription_change_listener_t *>(listener.get()));
}

std::int32_t getEventsBatchLimit(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_getEventsBatchLimit`. "
                                    "The `sub` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_DXFeedSubscription_getEventsBatchLimit,
                                              static_cast<dxfg_subscription_t *>(sub.get()));
}

/* int32_t */ void setEventsBatchLimit(/* dxfg_subscription_t * */ const JavaObjectHandle<DXFeedSubscription> &sub,
                                       std::int32_t eventsBatchLimit) {
    if (!sub) {
        throw InvalidArgumentException("Unable to execute function `dxfg_DXFeedSubscription_setEventsBatchLimit`. "
                                    "The `sub` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXFeedSubscription_setEventsBatchLimit,
                                           static_cast<dxfg_subscription_t *>(sub.get()), eventsBatchLimit);
}

namespace DXFeedEventListener {

JavaObjectHandle<dxfcpp::DXFeedEventListener> /* dxfg_feed_event_listener_t* */
create(/* dxfg_feed_event_listener_function */ void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException("Unable to create DXFeedEventListener. The `userFunc` parameter is nullptr");
    }

    return JavaObjectHandle<dxfcpp::DXFeedEventListener>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXFeedEventListener_new, dxfcpp::bit_cast<dxfg_feed_event_listener_function>(userFunc), userData));
}

} // namespace DXFeedEventListener

} // namespace isolated::api::IsolatedDXFeedSubscription

DXFCPP_END_NAMESPACE
