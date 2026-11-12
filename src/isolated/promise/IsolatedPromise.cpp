// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedPromise.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

#include <cstdint>
#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::promise::IsolatedPromise {

namespace {

// The handle of an event promise (dxfg_v4_promise_event_t, dxfg_v4_promise_events_t) starts with its
// dxfg_v4_promise_t, so the same handle is passed to the functions of both types.
template <typename Promise = dxfg_v4_promise_t> Promise *toGraal(void *promise, const char *function) {
    if (!promise) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `promise` is nullptr");
    }

    return static_cast<Promise *>(promise);
}

} // namespace

bool isDone(/* dxfg_v4_promise_t * */ void *promise) {
    return runGraalV4FunctionWithResult(dxfg_v4_Promise_isDone, toGraal(promise, "dxfg_v4_Promise_isDone")) == 1;
}

bool hasResult(/* dxfg_v4_promise_t * */ void *promise) {
    return runGraalV4FunctionWithResult(dxfg_v4_Promise_hasResult, toGraal(promise, "dxfg_v4_Promise_hasResult")) ==
           1;
}

bool hasException(/* dxfg_v4_promise_t * */ void *promise) {
    return runGraalV4FunctionWithResult(dxfg_v4_Promise_hasException,
                                        toGraal(promise, "dxfg_v4_Promise_hasException")) == 1;
}

bool isCancelled(/* dxfg_v4_promise_t * */ void *promise) {
    return runGraalV4FunctionWithResult(dxfg_v4_Promise_isCancelled,
                                        toGraal(promise, "dxfg_v4_Promise_isCancelled")) == 1;
}

std::shared_ptr<EventType> getResult(/* dxfg_v4_promise_event_t * */ void *promise) {
    auto *graalEvent = runGraalV4FunctionWithResult(
        dxfg_v4_Promise_EventType_getResult,
        toGraal<dxfg_v4_promise_event_t>(promise, "dxfg_v4_Promise_EventType_getResult"));

    // NULL: the promise has no result, there is nothing to release.
    if (graalEvent == nullptr) {
        return {};
    }

    std::shared_ptr<EventType> result{};

    try {
        result = EventMapper::fromGraal(graalEvent);
    } catch (...) {
        runGraalV4Function(dxfg_v4_EventType_release, graalEvent);

        throw;
    }

    runGraalV4Function(dxfg_v4_EventType_release, graalEvent);

    return result;
}

std::vector<std::shared_ptr<EventType>> getResults(/* dxfg_v4_promise_events_t * */ void *promise) {
    // A promise without a result gives an empty list, which is released as well.
    auto *graalEvents = runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Promise_List_EventType_getResult,
        toGraal<dxfg_v4_promise_events_t>(promise, "dxfg_v4_Promise_List_EventType_getResult"));

    std::vector<std::shared_ptr<EventType>> result{};

    try {
        result = EventMapper::fromGraalList(graalEvents);
    } catch (...) {
        runGraalV4Function(dxfg_v4_event_type_list_release, graalEvents);

        throw;
    }

    runGraalV4Function(dxfg_v4_event_type_list_release, graalEvents);

    return result;
}

JavaException getException(/* dxfg_v4_promise_t * */ void *promise) {
    auto *graalException =
        runGraalV4FunctionWithResult(dxfg_v4_Promise_getException, toGraal(promise, "dxfg_v4_Promise_getException"));

    // NULL: the promise has no exception, there is nothing to release.
    if (graalException == nullptr) {
        return JavaException::create(nullptr);
    }

    return JavaException::createAndRelease(graalException);
}

void await(/* dxfg_v4_promise_t * */ void *promise) {
    runGraalV4Function(dxfg_v4_Promise_await, toGraal(promise, "dxfg_v4_Promise_await"));
}

void await(/* dxfg_v4_promise_t * */ void *promise, std::int32_t timeoutInMilliseconds) {
    runGraalV4Function(dxfg_v4_Promise_awaitWithTimeout, toGraal(promise, "dxfg_v4_Promise_awaitWithTimeout"),
                       timeoutInMilliseconds);
}

bool awaitWithoutException(/* dxfg_v4_promise_t * */ void *promise, std::int32_t timeoutInMilliseconds) {
    // DXFG_EXECUTE_SUCCESSFULLY (0): the promise has completed; DXFG_PROMISE_AWAIT_TIMED_OUT (1): the wait timed out
    // and the promise is cancelled.
    return runGraalV4FunctionWithStatus(dxfg_v4_Promise_awaitWithoutException,
                                        toGraal(promise, "dxfg_v4_Promise_awaitWithoutException"),
                                        timeoutInMilliseconds) == DXFG_EXECUTE_SUCCESSFULLY;
}

void cancel(/* dxfg_v4_promise_t * */ void *promise) {
    runGraalV4Function(dxfg_v4_Promise_cancel, toGraal(promise, "dxfg_v4_Promise_cancel"));
}

// A list of the C API allocated by C++. The owning wrapper frees it; it is not copyable, so that a list is not freed
// twice.
template <typename ListType, typename ElementType, typename SizeType = decltype(ListType::size)>
struct GraalListWrapper {
    void *handle = nullptr;
    bool own = false;

    GraalListWrapper(void *listHandle, bool isOwned) noexcept : handle{listHandle}, own{isOwned} {
    }

    GraalListWrapper(const GraalListWrapper &) = delete;
    GraalListWrapper &operator=(const GraalListWrapper &) = delete;

    GraalListWrapper(GraalListWrapper &&other) noexcept
        : handle{std::exchange(other.handle, nullptr)}, own{std::exchange(other.own, false)} {
    }

    GraalListWrapper &operator=(GraalListWrapper &&) = delete;

    static std::ptrdiff_t calculateSize(std::ptrdiff_t initSize) noexcept {
        if (initSize < 0) {
            return 0;
        }

        if (initSize > std::numeric_limits<SizeType>::max()) {
            return std::numeric_limits<SizeType>::max();
        }

        return initSize;
    }

    static void *create(std::ptrdiff_t size) {
        auto *list = new ListType{static_cast<SizeType>(size), nullptr};

        if (size == 0) {
            return static_cast<void *>(list);
        }

        list->elements = new ElementType *[static_cast<std::size_t>(size)] {
            nullptr
        };

        return list;
    }

    bool setElement(std::ptrdiff_t elementIdx, void *element) noexcept {
        if (handle == nullptr || elementIdx < 0 || element == nullptr) {
            return false;
        }

        auto *list = static_cast<ListType *>(handle);

        if (list->elements == nullptr || elementIdx >= list->size) {
            return false;
        }

        list->elements[elementIdx] = static_cast<ElementType *>(element);

        return true;
    }

    static GraalListWrapper create(const std::vector<void *> &handles) {
        GraalListWrapper list{create(calculateSize(static_cast<std::ptrdiff_t>(handles.size()))), true};

        for (std::size_t i = 0; i < handles.size(); i++) {
            list.setElement(static_cast<std::ptrdiff_t>(i), handles[i]);
        }

        return list;
    }

    void free() {
        if (handle == nullptr) {
            return;
        }

        auto list = static_cast<ListType *>(handle);

        delete[] list->elements;
        delete list;
    }

    ~GraalListWrapper() {
        if (own) {
            free();
        }
    }
};

void * /* dxfg_v4_promise_t* */ allOf(const std::vector<void *> &promises) {
    auto list = GraalListWrapper<dxfg_v4_java_object_handler_list, dxfg_v4_java_object_handler>::create(promises);

    return runGraalV4FunctionWithNonNullResult(dxfg_v4_Promises_allOf,
                                               dxfcpp::bit_cast<dxfg_v4_promise_list *>(list.handle));
}

} // namespace isolated::promise::IsolatedPromise

DXFCPP_END_NAMESPACE
