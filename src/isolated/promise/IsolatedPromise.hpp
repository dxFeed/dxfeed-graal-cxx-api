// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/JavaException.hpp"

#include <memory>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::promise::IsolatedPromise {

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_isDone` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation has completed normally or exceptionally or was cancelled.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool isDone(/* dxfg_v4_promise_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_hasResult` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation has completed normally
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool hasResult(/* dxfg_v4_promise_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_hasException` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation has completed exceptionally or was cancelled.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool hasException(/* dxfg_v4_promise_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_isCancelled` in isolation.
 *
 * @param promise The promise's handle.
 * @return `true` when computation was cancelled.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool isCancelled(/* dxfg_v4_promise_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_EventType_getResult` in isolation.
 *
 * @param promise The promise's handle.
 * @return event by promise
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::shared_ptr<EventType> getResult(/* dxfg_v4_promise_event_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_List_EventType_getResult` in isolation.
 *
 * @param promise The promise's handle.
 * @return events by promise
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
std::vector<std::shared_ptr<EventType>> getResults(/* dxfg_v4_promise_events_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_getException` in isolation.
 *
 * @param promise The promise's handle.
 * @return exception by promise
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
JavaException getException(/* dxfg_v4_promise_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_await` in isolation.
 *
 * @param promise The promise's handle.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void await(/* dxfg_v4_promise_t * */ void *promise);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_awaitWithTimeout` in isolation.
 *
 * @param promise The promise's handle.
 * @param timeoutInMilliseconds The promise's timeout.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void await(/* dxfg_v4_promise_t * */ void *promise, std::int32_t timeoutInMilliseconds);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_awaitWithoutException` in isolation.
 *
 * @param promise The promise's handle.
 * @param timeoutInMilliseconds The promise's timeout.
 * @return `true` if the computation has completed normally; `false` when wait timed out.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool awaitWithoutException(/* dxfg_v4_promise_t * */ void *promise, std::int32_t timeoutInMilliseconds);

/**
 * Calls the Graal SDK function `dxfg_v4_Promise_cancel` in isolation.
 *
 * @param promise The promise's handle.
 * @throws InvalidArgumentException if promise handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void cancel(/* dxfg_v4_promise_t * */ void *promise);

/*
 * Not wrapped yet: dxfg_v4_Promise_List_EventType_complete, dxfg_v4_Promise_EventType_complete,
 * dxfg_v4_Promise_completeExceptionally, dxfg_v4_Promise_whenDone, dxfg_v4_Promise_whenDoneAsync,
 * dxfg_v4_Promise_completed, dxfg_v4_Promise_failed.
 */

/**
 * Calls the Graal SDK function `dxfg_v4_Promises_allOf` in isolation.
 *
 * @param promises The handles of the promises.
 * @return The handle of the promise that completes when all the promises complete.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void * /* dxfg_v4_promise_t* */ allOf(const std::vector<void *> &promises);

} // namespace isolated::promise::IsolatedPromise

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
