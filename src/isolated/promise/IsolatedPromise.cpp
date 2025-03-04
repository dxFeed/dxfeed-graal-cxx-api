// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/event/EventMapper.hpp>
#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/promise/IsolatedPromise.hpp>

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

    auto result = dxfcpp::EventMapper::fromGraal(graalEvent);

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

    auto result = dxfcpp::EventMapper::fromGraalList(graalEvents);

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

    auto result = JavaException::create(graalException);

    runGraalFunction(dxfg_Exception_release, graalException);

    return result;
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

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_awaitWithoutException,
                                                  static_cast<dxfg_promise_t *>(promise), timeoutInMilliseconds) == 0;
}

void /* int32_t */ cancel(/* dxfg_promise_t * */ void *promise) {
    if (!promise) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Promise_cancel`. The `promise` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_Promise_cancel, static_cast<dxfg_promise_t *>(promise));
}

template <typename ListType, typename ElementType, typename SizeType = decltype(ListType::size)>
struct GraalListWrapper {
    void *handle = nullptr;
    bool own = false;

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

        list->elements = new ElementType *[size] {
            nullptr
        };

        return list;
    }

    bool setElement(std::ptrdiff_t elementIdx, void *element) noexcept {
        if (handle == nullptr || elementIdx < 0 || elementIdx >= std::numeric_limits<SizeType>::max() ||
            element == nullptr) {
            return false;
        }

        static_cast<ListType *>(handle)->elements[elementIdx] = static_cast<ElementType *>(element);

        return true;
    }

    static GraalListWrapper create(const std::vector<void *> &handles) {
        auto list = GraalListWrapper{create(calculateSize(handles.size()))};

        for (std::size_t i = 0; i < handles.size(); i++) {
            list.setElement(i, handles[i]);
        }

        return GraalListWrapper{list.handle, true};
    }

    void free() {
        if (handle == nullptr) {
            return;
        }

        auto list = static_cast<ListType *>(handle);

        if (list->size > 0 && list->elements != nullptr) {
            delete[] list->elements;
        }

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
