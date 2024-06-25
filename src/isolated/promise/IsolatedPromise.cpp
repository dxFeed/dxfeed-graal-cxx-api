// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/promise/IsolatedPromise.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::promise::IsolatedPromise {

bool /* int32_t */ isDone(/* dxfg_promise_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_isDone`. The `promise` is nullptr");
    }

    return {};
}

bool /* int32_t */ hasResult(/* dxfg_promise_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_hasResult`. The `promise` is nullptr");
    }

    return {};
}

bool /* int32_t */ hasException(/* dxfg_promise_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_hasException`. The `promise` is nullptr");
    }

    return {};
}

bool /* int32_t */ isCancelled(/* dxfg_promise_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_isCancelled`. The `promise` is nullptr");
    }

    return {};
}

std::shared_ptr<EventType> /* dxfg_event_type_t* */ getResult(/* dxfg_promise_event_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_EventType_getResult`. The `promise` is nullptr");
    }

    return {};
}

std::vector<std::shared_ptr<EventType>> /* dxfg_event_type_list* */ getResults(/* dxfg_promise_events_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_List_EventType_getResult`. The `promise` is nullptr");
    }

    return {};
}

JavaException /* dxfg_exception_t* */ getException(/* dxfg_promise_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_getException`. The `promise` is nullptr");
    }

    return {"", "", ""};
}

void /* int32_t */ await(/* dxfg_promise_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_await`. The `promise` is nullptr");
    }

}

void /* int32_t */ await(/* dxfg_promise_t * */ void* promise, std::int32_t timeoutInMilliseconds) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_await2`. The `promise` is nullptr");
    }

}

bool /* int32_t */ awaitWithoutException(/* dxfg_promise_t * */ void* promise, std::int32_t timeoutInMilliseconds) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_awaitWithoutException`. The `promise` is nullptr");
    }

    return {};
}

void /* int32_t */ cancel(/* dxfg_promise_t * */ void* promise) {
    if (!promise) {
        throw std::invalid_argument(
    "Unable to execute function `dxfg_Promise_cancel`. The `promise` is nullptr");
    }
}

}

DXFCPP_END_NAMESPACE
