// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/promise/IsolatedPromise.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::promise::IsolatedPromise {

bool /* int32_t */ isDone(/* dxfg_promise_t * */ void* promise) {
    return {};
}

bool /* int32_t */ hasResult(/* dxfg_promise_t * */ void* promise) {
    return {};
}

bool /* int32_t */ hasException(/* dxfg_promise_t * */ void* promise) {
    return {};
}

std::shared_ptr<EventType> /* dxfg_event_type_t* */ getResult(/* dxfg_promise_event_t * */ void* promise) {
    return {};
}

std::vector<std::shared_ptr<EventType>> /* dxfg_event_type_list* */ getResults(/* dxfg_promise_events_t * */ void* promise) {
    return {};
}

JavaException /* dxfg_exception_t* */ getException(/* dxfg_promise_t * */ void* promise) {
    return {"", "", ""};
}

void /* int32_t */ await(/* dxfg_promise_t * */ void* promise) {

}

void /* int32_t */ await(/* dxfg_promise_t * */ void* promise, std::int32_t timeoutInMilliseconds) {

}

}

DXFCPP_END_NAMESPACE
