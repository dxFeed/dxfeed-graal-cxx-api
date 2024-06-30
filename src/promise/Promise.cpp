// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

PromiseImpl::PromiseImpl(void *handle) : handle(handle) {
}

bool PromiseImpl::isDone() const {
    return isolated::promise::IsolatedPromise::isDone(handle);
}

bool PromiseImpl::hasResult() const {
    return isolated::promise::IsolatedPromise::hasResult(handle);
}

bool PromiseImpl::hasException() const {
    return isolated::promise::IsolatedPromise::hasException(handle);
}

bool PromiseImpl::isCancelled() const {
    return isolated::promise::IsolatedPromise::isCancelled(handle);
}

JavaException PromiseImpl::getException() const {
    return isolated::promise::IsolatedPromise::getException(handle);
}

void PromiseImpl::await() const {
    isolated::promise::IsolatedPromise::await(handle);
}

void PromiseImpl::await(std::int32_t timeoutInMilliseconds) const {
    isolated::promise::IsolatedPromise::await(handle, timeoutInMilliseconds);
}

bool PromiseImpl::awaitWithoutException(std::int32_t timeoutInMilliseconds) const {
    return isolated::promise::IsolatedPromise::awaitWithoutException(handle, timeoutInMilliseconds);
}

void PromiseImpl::cancel() const {
    isolated::promise::IsolatedPromise::cancel(handle);
}

EventPromiseImpl::EventPromiseImpl(void *handle) : PromiseImpl(handle), handle(handle) {
}

EventPromiseImpl::~EventPromiseImpl() {
    JavaObjectHandle<EventPromiseImpl>::deleter(handle);
}

std::shared_ptr<EventType> EventPromiseImpl::getResult() const {
    return isolated::promise::IsolatedPromise::getResult(handle);
}

EventsPromiseImpl::EventsPromiseImpl(void *handle) : PromiseImpl(handle), handle(handle) {
}

EventsPromiseImpl::~EventsPromiseImpl() {
    JavaObjectHandle<EventsPromiseImpl>::deleter(handle);
}

std::vector<std::shared_ptr<EventType>> EventsPromiseImpl::getResult() const {
    return isolated::promise::IsolatedPromise::getResults(handle);
}

DXFCPP_END_NAMESPACE