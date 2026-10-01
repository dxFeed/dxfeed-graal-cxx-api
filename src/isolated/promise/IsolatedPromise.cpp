// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/promise/IsolatedPromise.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::promise::IsolatedPromise {

bool /* int32_t */ isDone(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Promise_isDone`. The `promise` is nullptr");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_isDone, static_cast<dxfg_promise_t *>(promise)) == 1;
}

bool /* int32_t */ hasResult(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Promise_hasResult`. The `promise` is nullptr");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_hasResult, static_cast<dxfg_promise_t *>(promise)) == 1;
}

bool /* int32_t */ hasException(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Promise_hasException`. The `promise` is nullptr");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_hasException, static_cast<dxfg_promise_t *>(promise)) ==
           1;
}

bool /* int32_t */ isCancelled(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Promise_isCancelled`. The `promise` is nullptr");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_isCancelled, static_cast<dxfg_promise_t *>(promise)) ==
           1;
}

std::shared_ptr<EventType> /* dxfg_event_type_t* */ getResult(/* dxfg_promise_event_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Promise_EventType_getResult`. The `promise` is nullptr");
    }

    auto *graalEvent = runGraalFunctionAndThrowIfNullptr(dxfg_Promise_EventType_getResult,
                                                         static_cast<dxfg_promise_event_t *>(promise));

    auto result = EventMapper::fromGraal(graalEvent);

    runGraalFunctionAndThrowIfLessThanZero(dxfg_EventType_release, graalEvent);

    return result;
}

std::vector<std::shared_ptr<EventType>> /* dxfg_event_type_list* */
getResults(/* dxfg_promise_events_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Promise_List_EventType_getResult`. The `promise` is nullptr");
    }

    auto *graalEvents = runGraalFunctionAndThrowIfNullptr(dxfg_Promise_List_EventType_getResult,
                                                          static_cast<dxfg_promise_events_t *>(promise));

    auto result = EventMapper::fromGraalList(graalEvents);

    runGraalFunctionAndThrowIfLessThanZero(dxfg_CList_EventType_release, graalEvents);

    return result;
}

JavaException /* dxfg_exception_t* */ getException(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Promise_getException`. The `promise` is nullptr");
    }

    auto *graalException =
        runGraalFunctionAndThrowIfNullptr(dxfg_Promise_getException, static_cast<dxfg_promise_t *>(promise));

    return JavaException::createAndRelease(graalException);
}

void /* int32_t */ await(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Promise_await`. The `promise` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_await, static_cast<dxfg_promise_t *>(promise));
}

void /* int32_t */ await(/* dxfg_promise_t * */ void *promise, std::int32_t timeoutInMilliseconds) {
    if (!promise) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Promise_await2`. The `promise` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_await2, static_cast<dxfg_promise_t *>(promise),
                                           timeoutInMilliseconds);
}

bool /* int32_t */ awaitWithoutException(/* dxfg_promise_t * */ void *promise, std::int32_t timeoutInMilliseconds) {
    if (!promise) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_Promise_awaitWithoutException`. The `promise` is nullptr");
    }

    // DXFG_EXECUTE_SUCCESSFULLY (0): the promise has completed; DXFG_PROMISE_AWAIT_TIMED_OUT (1, Graal SDK v3.5.0+):
    // the wait timed out and the promise is cancelled. Graal SDK before v3.5.0 returned 0 on timeout as well.
    return runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_awaitWithoutException,
                                                  static_cast<dxfg_promise_t *>(promise),
                                                  timeoutInMilliseconds) == DXFG_EXECUTE_SUCCESSFULLY;
}

void /* int32_t */ cancel(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Promise_cancel`. The `promise` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_cancel, static_cast<dxfg_promise_t *>(promise));
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

// dxfg_promise_t*       dxfg_Promises_allOf(graal_isolatethread_t *thread, dxfg_promise_list *promises);
void *allOf(const std::vector<void *> &promises) {
    auto list = GraalListWrapper<dxfg_java_object_handler_list, dxfg_java_object_handler>::create(promises);

    return dxfcpp::bit_cast<void *>(
        runGraalFunctionAndThrowIfNullptr(dxfg_Promises_allOf, dxfcpp::bit_cast<dxfg_promise_list *>(list.handle)));
}

} // namespace isolated::promise::IsolatedPromise

DXFCPP_END_NAMESPACE
