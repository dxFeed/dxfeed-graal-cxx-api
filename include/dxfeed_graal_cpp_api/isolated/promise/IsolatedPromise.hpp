// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../event/EventType.hpp"
#include "../../exceptions/JavaException.hpp"

#include <memory>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::promise::IsolatedPromise {

/**
 * Calls the Graal SDK function `dxfg_Promise_isDone` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation has completed normally or exceptionally or was cancelled.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ isDone(/* dxfg_promise_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_hasResult` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation has completed normally
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ hasResult(/* dxfg_promise_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_hasException` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation has completed exceptionally or was cancelled.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ hasException(/* dxfg_promise_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_isCancelled` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation was cancelled.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ isCancelled(/* dxfg_promise_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_EventType_getResult` in isolation.
 *
 * @param promise The promise's handle.
 * @return event by promise
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::shared_ptr<EventType> /* dxfg_event_type_t* */ getResult(/* dxfg_promise_event_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_List_EventType_getResult` in isolation.
 *
 * @param promise The promise's handle.
 * @return events by promise
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::vector<std::shared_ptr<EventType>> /* dxfg_event_type_list* */ getResults(/* dxfg_promise_events_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_getException` in isolation.
 *
 * @param promise The promise's handle.
 * @return exception by promise
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaException /* dxfg_exception_t* */ getException(/* dxfg_promise_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_await` in isolation.
 *
 * @param promise The promise's handle.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ await(/* dxfg_promise_t * */ void* promise);

/**
 * Calls the Graal SDK function `dxfg_Promise_await2` in isolation.
 *
 * @param promise The promise's handle.
 * @param timeoutInMilliseconds The promise's timeout.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ await(/* dxfg_promise_t * */ void* promise, std::int32_t timeoutInMilliseconds);

/**
 * Calls the Graal SDK function `dxfg_Promise_awaitWithoutException` in isolation.
 *
 * @param promise The promise's handle.
 * @param timeoutInMilliseconds The promise's timeout.
 * @return `true` if the computation has completed normally; `false` when wait timed out.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */ awaitWithoutException(/* dxfg_promise_t * */ void* promise, std::int32_t timeoutInMilliseconds);

/**
 * Calls the Graal SDK function `dxfg_Promise_cancel` in isolation.
 *
 * @param promise The promise's handle.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ cancel(/* dxfg_promise_t * */ void* promise);

/*

int32_t               dxfg_Promise_List_EventType_complete(graal_isolatethread_t *thread, dxfg_promise_t *promise, dxfg_event_type_list* events);
int32_t               dxfg_Promise_EventType_complete(graal_isolatethread_t *thread, dxfg_promise_t *promise, dxfg_event_type_t* event);
int32_t               dxfg_Promise_completeExceptionally(graal_isolatethread_t *thread, dxfg_promise_t *promise, dxfg_exception_t* exception);
int32_t               dxfg_Promise_whenDone(graal_isolatethread_t *thread, dxfg_promise_t *promise, dxfg_promise_handler_function promise_handler_function, void *user_data);
int32_t               dxfg_Promise_whenDoneAsync(graal_isolatethread_t *thread, dxfg_promise_t *promise, dxfg_promise_handler_function promise_handler_function, void *user_data, dxfg_executor_t* executor);
dxfg_promise_t*       dxfg_Promise_completed(graal_isolatethread_t *thread, dxfg_promise_t *promise, dxfg_java_object_handler *handler);
dxfg_promise_t*       dxfg_Promise_failed(graal_isolatethread_t *thread, dxfg_promise_t *promise, dxfg_exception_t* exception);

dxfg_promise_t*       dxfg_Promises_allOf(graal_isolatethread_t *thread, dxfg_promise_list *promises);

*/


}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()