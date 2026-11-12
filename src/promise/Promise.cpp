// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/promise/Promise.hpp"

#include "../../include/dxfeed_graal_cpp_api/detail/JavaObjectHandle.hpp"
#include "../isolated/promise/IsolatedPromise.hpp"
#include "PromiseContext.hpp"

#include <chrono>
#include <dxfg_api.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace {

// The exact text of QD's Promise.await(timeout) on a timeout: cancel() gives "cancel", an interrupt "interrupted".
constexpr std::string_view AWAIT_TIMED_OUT_MESSAGE = "await timed out";
constexpr std::string_view CANCELLATION_EXCEPTION = "CancellationException";

bool isAwaitTimeout(const JavaException &e) {
    const auto &className = e.getJavaClassName();

    return className.size() >= CANCELLATION_EXCEPTION.size() &&
           std::string_view{className}.substr(className.size() - CANCELLATION_EXCEPTION.size()) ==
               CANCELLATION_EXCEPTION &&
           e.getJavaMessage() == AWAIT_TIMED_OUT_MESSAGE;
}

} // namespace

PromiseImpl::PromiseImpl(void *promiseHandle, std::shared_ptr<const PromiseContext> promiseContext)
    : handle(promiseHandle), context(std::move(promiseContext)) {
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
    const auto start = std::chrono::steady_clock::now();

    try {
        isolated::promise::IsolatedPromise::await(handle, timeoutInMilliseconds);
    } catch (const JavaException &e) {
        if (!context || !isAwaitTimeout(e)) {
            throw;
        }

        // The exception of the same Java class with the same Java stack trace, but the text is added with the
        // description of the request. If the description cannot be built, the original exception is thrown.
        std::optional<JavaException> decorated{};

        try {
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

            decorated.emplace(e.getJavaMessage() + "; " +
                                  describePromiseTimeout(*context, elapsed.count(), timeoutInMilliseconds),
                              e.getJavaClassName(), e.getJavaStackTrace());
        } catch (...) {
        }

        if (!decorated) {
            throw;
        }

        throw *decorated;
    }
}

bool PromiseImpl::awaitWithoutException(std::int32_t timeoutInMilliseconds) const {
    return isolated::promise::IsolatedPromise::awaitWithoutException(handle, timeoutInMilliseconds);
}

void PromiseImpl::cancel() const {
    isolated::promise::IsolatedPromise::cancel(handle);
}

VoidPromiseImpl::VoidPromiseImpl(void *promiseHandle, bool isOwned,
                                 std::shared_ptr<const PromiseContext> promiseContext)
    : PromiseImpl(promiseHandle, std::move(promiseContext)), handle(promiseHandle), own(isOwned) {
}

VoidPromiseImpl::~VoidPromiseImpl() {
    if (own) {
        JavaObjectHandle<PromiseImpl>::deleter(handle);
    }
}

void VoidPromiseImpl::getResult() const {
}

EventPromiseImpl::EventPromiseImpl(void *promiseHandle, bool isOwned,
                                   std::shared_ptr<const PromiseContext> promiseContext)
    : PromiseImpl(promiseHandle, std::move(promiseContext)), handle(promiseHandle), own(isOwned) {
}

EventPromiseImpl::~EventPromiseImpl() {
    if (own) {
        JavaObjectHandle<EventPromiseImpl>::deleter(handle);
    }
}

std::shared_ptr<EventType> EventPromiseImpl::getResult() const {
    return isolated::promise::IsolatedPromise::getResult(handle);
}

EventsPromiseImpl::EventsPromiseImpl(void *promiseHandle, bool isOwned,
                                     std::shared_ptr<const PromiseContext> promiseContext)
    : PromiseImpl(promiseHandle, std::move(promiseContext)), handle(promiseHandle), own(isOwned) {
}

EventsPromiseImpl::~EventsPromiseImpl() {
    if (own) {
        JavaObjectHandle<EventsPromiseImpl>::deleter(handle);
    }
}

std::vector<std::shared_ptr<EventType>> EventsPromiseImpl::getResult() const {
    return isolated::promise::IsolatedPromise::getResults(handle);
}

PromiseListImpl::PromiseListImpl(void *promiseListHandle) : handle(promiseListHandle) {
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

    if (list->elements == nullptr || list->size <= 0) {
        return 0;
    }

    return static_cast<std::size_t>(list->size);
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