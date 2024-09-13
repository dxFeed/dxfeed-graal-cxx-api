// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../exceptions/JavaException.hpp"
#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <chrono>
#include <memory>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

struct EventType;
struct JavaException;

template <class T, class U>
concept Derived = std::is_base_of_v<U, T>;

template <typename EBase, Derived<EBase> EDerived>
std::shared_ptr<EDerived> convertEvent(const std::shared_ptr<EBase> &source) {
    return source->template sharedAs<EDerived>();
}

template <typename EBase, Derived<EBase> EDerived>
std::vector<std::shared_ptr<EDerived>> convertEvents(const std::vector<std::shared_ptr<EBase>> &source) {
    std::vector<std::shared_ptr<EDerived>> result{};

    result.reserve(source.size());

    for (const auto &e : source) {
        result.emplace_back(e->template sharedAs<EDerived>());
    }

    return result;
}

struct PromiseImpl {
  protected:
    void *handle = nullptr;

  public:
    explicit PromiseImpl(void *handle);

    bool isDone() const;
    bool hasResult() const;
    bool hasException() const;
    bool isCancelled() const;
    JavaException getException() const;
    void await() const;
    void await(std::int32_t timeoutInMilliseconds) const;
    bool awaitWithoutException(std::int32_t timeoutInMilliseconds) const;
    void cancel() const;
};

struct EventPromiseImpl : PromiseImpl {
    void *handle = nullptr;

    explicit EventPromiseImpl(void *handle);
    ~EventPromiseImpl();
    std::shared_ptr<EventType> getResult() const;
};

struct EventsPromiseImpl : PromiseImpl {
    void *handle = nullptr;

    explicit EventsPromiseImpl(void *handle);
    ~EventsPromiseImpl();
    std::vector<std::shared_ptr<EventType>> getResult() const;
};

/**
 * Mixin for wrapping calls to common promise methods.
 *
 * @tparam P The promise type
 */
template <typename P> struct CommonPromiseMixin {
    /**
     * Returns `true` when computation has completed normally, or exceptionally, or was cancelled.
     *
     * @return `true` when computation has completed.
     */
    bool isDone() const {
        return static_cast<const P *>(this)->impl.isDone();
    }

    /**
     * Returns `true` when computation has completed normally.
     * Use ::getResult() method to get the result of the computation.
     * @return `true` when computation has completed normally.
     * @see ::getResult()
     */
    bool hasResult() const {
        return static_cast<const P *>(this)->impl.hasResult();
    }

    /**
     * Returns `true` when computation has completed exceptionally or was cancelled.
     * Use ::getException() method to get the exceptional outcome of the computation.
     * @return `true` when computation has completed exceptionally or was cancelled.
     */
    bool hasException() const {
        return static_cast<const P *>(this)->impl.hasException();
    }

    /**
     * Returns `true` when computation was cancelled.
     * Use ::getException() method to get the corresponding CancellationException.
     * @return `true` when computation was cancelled.
     * @see ::getException()
     */
    bool isCancelled() const {
        return static_cast<const P *>(this)->impl.isCancelled();
    }

    /**
     * Returns exceptional outcome of computation. If computation has no ::hasException() exception,
     * then this method returns an exception with a message "null". If computation has completed exceptionally or was
     * cancelled, then the result of this method is not an exception with a message "null". If computation was @ref
     * ::isCancelled() "cancelled", then this method returns "an instance of CancellationException".
     *
     * @return exceptional outcome of computation.
     * @see ::hasException()
     */
    JavaException getException() const {
        return static_cast<const P *>(this)->impl.getException();
    }

    /**
     * Wait for computation to complete or timeout or throw an exception in case of exceptional completion.
     * If the wait is interrupted, then the computation is @ref ::cancel() "cancelled",
     * the interruption flag on the current thread is set, and "CancellationException" is thrown.
     *
     * <p>If the wait times out, then the computation is @ref ::cancel() "cancelled" and this method returns `false`.
     * Use this method in the code that shall continue normal execution in case of timeout.
     *
     * @param timeoutInMilliseconds The timeout.
     * @return `true` if the computation has completed normally; `false` when wait timed out.
     * @throws CancellationException if computation was cancelled.
     * @throws PromiseException if computation has completed exceptionally.
     */
    bool awaitWithoutException(std::int32_t timeoutInMilliseconds) const {
        return static_cast<const P *>(this)->impl.awaitWithoutException(timeoutInMilliseconds);
    }

    /**
     * Wait for computation to complete or timeout or throw an exception in case of exceptional completion.
     * If the wait is interrupted, then the computation is @ref ::cancel() "cancelled",
     * the interruption flag on the current thread is set, and "CancellationException" is thrown.
     *
     * <p>If the wait times out, then the computation is @ref ::cancel() "cancelled" and this method returns `false`.
     * Use this method in the code that shall continue normal execution in case of timeout.
     *
     * @param timeoutInMilliseconds The timeout.
     * @return `true` if the computation has completed normally; `false` when wait timed out.
     * @throws CancellationException if computation was cancelled.
     * @throws PromiseException if computation has completed exceptionally.
     */
    bool awaitWithoutException(const std::chrono::milliseconds &timeoutInMilliseconds) const {
        auto timeout = timeoutInMilliseconds.count();

        if (timeout > std::numeric_limits<std::int32_t>::max()) {
            timeout = std::numeric_limits<std::int32_t>::max();
        }

        return static_cast<const P *>(this)->impl.awaitWithoutException(timeout);
    }

    /**
     * Cancels computation. This method does nothing if computation has already @ref ::isDone() "completed".
     *
     * <p>If cancelled, then ::getException() will return "CancellationException",
     * @ref ::isDone() "isDone", @ref ::isCancelled() "isCancelled", and @ref ::hasException() "hasException" will
     * return `true`, all handlers that were installed with `whenDone` method are notified by invoking their
     * `promiseDone` method, and all waiters on @ref ::await() "join" method throw "CancellationException".
     */
    void cancel() const {
        static_cast<const P *>(this)->impl.cancel();
    }
};

/**
 * Mixin for wrapping Promise method calls for a single event.
 * @tparam E The event type.
 * @tparam P The promise type.
 */
template <typename E, typename P> struct EventPromiseMixin {
    /**
     * Returns result of computation. If computation has no @ref CommonPromiseMixin::hasResult() "result", then
     * this method returns `std::shared_ptr<E>(nullptr)`.
     *
     * @return The result of computation.
     * @see CommonPromiseMixin::hasResult()
     */
    std::shared_ptr<E> getResult() const {
        return convertEvent<EventType, E>(static_cast<const P *>(this)->impl.getResult());
    }

    /**
     * Wait for computation to complete and return its result or throw an exception in case of exceptional completion.
     * @return result of computation.
     * @throws CancellationException if computation was cancelled.
     * @throws PromiseException if computation has completed exceptionally.
     */
    std::shared_ptr<E> await() const {
        static_cast<const P *>(this)->impl.await();

        return getResult();
    }

    /**
     * Wait for computation to complete or timeout and return its result or throw an exception in case of exceptional
     * completion or timeout.
     *
     * @param timeoutInMilliseconds The timeout.
     * @return The result of computation.
     * @throws CancellationException if computation was cancelled or timed out.
     * @throws PromiseException if computation has completed exceptionally.
     */
    std::shared_ptr<E> await(std::int32_t timeoutInMilliseconds) const & {
        static_cast<const P *>(this)->impl.await(timeoutInMilliseconds);

        return getResult();
    }

    /**
     * Wait for computation to complete or timeout and return its result or throw an exception in case of exceptional
     * completion or timeout.
     *
     * @param timeoutInMilliseconds The timeout.
     * @return The result of computation.
     * @throws CancellationException if computation was cancelled or timed out.
     * @throws PromiseException if computation has completed exceptionally.
     */
    std::shared_ptr<E> await(const std::chrono::milliseconds &timeoutInMilliseconds) const & {
        auto timeout = timeoutInMilliseconds.count();

        if (timeout > std::numeric_limits<std::int32_t>::max()) {
            timeout = std::numeric_limits<std::int32_t>::max();
        }

        static_cast<const P *>(this)->impl.await(timeout);

        return getResult();
    }
};

template <typename E, typename P> struct EventsPromiseMixin {
    /**
     * Returns result of computation. If computation has no @ref CommonPromiseMixin::hasResult() "result", then
     * this method returns an empty ollection.
     *
     * @return The result of computation.
     * @see CommonPromiseMixin::hasResult()
     */
    std::vector<std::shared_ptr<E>> getResult() const {
        return convertEvents<EventType, E>(static_cast<const P *>(this)->impl.getResult());
    }

    /**
     * Wait for computation to complete and return its result or throw an exception in case of exceptional completion.
     * @return result of computation.
     * @throws CancellationException if computation was cancelled.
     * @throws PromiseException if computation has completed exceptionally.
     */
    std::vector<std::shared_ptr<E>> await() const {
        static_cast<const P *>(this)->impl.await();

        return getResult();
    }

    /**
     * Wait for computation to complete or timeout and return its result or throw an exception in case of exceptional
     * completion or timeout.
     *
     * @param timeoutInMilliseconds The timeout.
     * @return The result of computation.
     * @throws CancellationException if computation was cancelled or timed out.
     * @throws PromiseException if computation has completed exceptionally.
     */
    std::vector<std::shared_ptr<E>> await(std::int32_t timeoutInMilliseconds) const & {
        static_cast<const P *>(this)->impl.await(timeoutInMilliseconds);

        return getResult();
    }

    /**
     * Wait for computation to complete or timeout and return its result or throw an exception in case of exceptional
     * completion or timeout.
     *
     * @param timeoutInMilliseconds The timeout.
     * @return The result of computation.
     * @throws CancellationException if computation was cancelled or timed out.
     * @throws PromiseException if computation has completed exceptionally.
     */
    std::vector<std::shared_ptr<E>> await(const std::chrono::milliseconds &timeoutInMilliseconds) const & {
        auto timeout = timeoutInMilliseconds.count();

        if (timeout > std::numeric_limits<std::int32_t>::max()) {
            timeout = std::numeric_limits<std::int32_t>::max();
        }

        static_cast<const P *>(this)->impl.await(static_cast<std::int32_t>(timeout));

        return getResult();
    }
};

/**
 * Result of a computation that will be completed normally or exceptionally in the future.
 * @tparam T The result type.
 */
template <typename T> struct Promise {};

/**
 * Result of an event receiving that will be completed normally or exceptionally in the future.
 * @tparam E The event type.
 */
template <typename E>
struct Promise<std::shared_ptr<E>> : CommonPromiseMixin<Promise<std::shared_ptr<E>>>,
                                     EventPromiseMixin<E, Promise<std::shared_ptr<E>>> {
    friend struct CommonPromiseMixin<Promise>;
    friend struct EventPromiseMixin<E, Promise>;

    EventsPromiseImpl impl;

    explicit Promise(void *handle) : impl(handle) {
    }
};

/**
 * Result of an collection of events receiving that will be completed normally or exceptionally in the future.
 * @tparam E The event type.
 */
template <typename E>
struct Promise<std::vector<std::shared_ptr<E>>> : CommonPromiseMixin<Promise<std::vector<std::shared_ptr<E>>>>,
                                                  EventsPromiseMixin<E, Promise<std::vector<std::shared_ptr<E>>>> {
    friend struct CommonPromiseMixin<Promise>;
    friend struct EventsPromiseMixin<E, Promise>;

    EventsPromiseImpl impl;

    explicit Promise(void *handle) : impl(handle) {
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()