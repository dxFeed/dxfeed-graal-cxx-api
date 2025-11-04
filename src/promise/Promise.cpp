// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/promise/Promise.hpp"

#include "../../include/dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/promise/IsolatedPromise.hpp"

#include <dxfg_api.h>
#include <memory>

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

VoidPromiseImpl::VoidPromiseImpl(void *handle, bool own) : PromiseImpl(handle), handle(handle), own(own) {
}

VoidPromiseImpl::~VoidPromiseImpl() {
    if (own) {
        JavaObjectHandle<PromiseImpl>::deleter(handle);
    }
}

void VoidPromiseImpl::getResult() const {
}

EventPromiseImpl::EventPromiseImpl(void *handle, bool own) : PromiseImpl(handle), handle(handle), own(own) {
}

EventPromiseImpl::~EventPromiseImpl() {
    if (own) {
        JavaObjectHandle<EventPromiseImpl>::deleter(handle);
    }
}

std::shared_ptr<EventType> EventPromiseImpl::getResult() const {
    return isolated::promise::IsolatedPromise::getResult(handle);
}

EventsPromiseImpl::EventsPromiseImpl(void *handle, bool own) : PromiseImpl(handle), handle(handle), own(own) {
}

EventsPromiseImpl::~EventsPromiseImpl() {
    if (own) {
        JavaObjectHandle<EventsPromiseImpl>::deleter(handle);
    }
}

std::vector<std::shared_ptr<EventType>> EventsPromiseImpl::getResult() const {
    return isolated::promise::IsolatedPromise::getResults(handle);
}

PromiseListImpl::PromiseListImpl(void *handle) : handle(handle) {
}

PromiseListImpl::~PromiseListImpl() {
    JavaObjectHandleList<PromiseListImpl>::deleter(handle);
}

std::size_t PromiseListImpl::getSize(void *handle) {
    if (!handle) {
        return {};
    }

    using ListType = dxfg_java_object_handler_list;

    auto list = static_cast<ListType *>(handle);

    if (list->elements == nullptr) {
        return 0;
    }

    return list->size;
}

void *PromiseListImpl::getElement(void *handle, std::size_t index) {
    if (!handle) {
        return {};
    }

    using ListType = dxfg_java_object_handler_list;
    auto list = static_cast<ListType *>(handle);

    return list->elements[index];
}

DXFCPP_END_NAMESPACE