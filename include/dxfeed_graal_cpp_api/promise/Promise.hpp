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

struct VoidPromiseImpl : PromiseImpl {
    void *handle = nullptr;
    bool own = true;

    explicit VoidPromiseImpl(void *handle, bool own = true);
    ~VoidPromiseImpl();
    void getResult() const;
};

struct EventPromiseImpl : PromiseImpl {
    void *handle = nullptr;
    bool own = true;

    explicit EventPromiseImpl(void *handle, bool own = true);
    ~EventPromiseImpl();
    std::shared_ptr<EventType> getResult() const;
};

struct EventsPromiseImpl : PromiseImpl {
    void *handle = nullptr;
    bool own = true;

    explicit EventsPromiseImpl(void *handle, bool own = true);
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
 * Mixin for wrapping Promise method calls for a void.
 *
 * @tparam P The promise type.
 */
template <typename P> struct VoidPromiseMixin {
    /**
     * Returns result of computation.
     *
     * @return The result of computation.
     * @see CommonPromiseMixin::hasResult()
     */
    void getResult() const {
        return static_cast<const P *>(this)->impl.getResult();
    }

    /**
     * Wait for computation to complete and return its result or throw an exception in case of exceptional completion.
     * @return result of computation.
     * @throws CancellationException if computation was cancelled.
     * @throws PromiseException if computation has completed exceptionally.
     */
    void await() const {
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
    void await(std::int32_t timeoutInMilliseconds) const & {
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
    void await(const std::chrono::milliseconds &timeoutInMilliseconds) const & {
        auto timeout = timeoutInMilliseconds.count();

        if (timeout > std::numeric_limits<std::int32_t>::max()) {
            timeout = std::numeric_limits<std::int32_t>::max();
        }

        static_cast<const P *>(this)->impl.await(timeout);

        return getResult();
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
     * this method returns an empty collection.
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
 * Result of an void receiving that will be completed normally or exceptionally in the future.
 */
template <> struct Promise<void> : CommonPromiseMixin<Promise<void>>, VoidPromiseMixin<Promise<void>> {
    friend struct CommonPromiseMixin<Promise>;
    friend struct VoidPromiseMixin<Promise>;

    VoidPromiseImpl impl;

    explicit Promise(void *handle) : impl(handle) {
    }

    Promise(const Promise &) = delete;
    Promise &operator=(const Promise &) = delete;
    Promise(Promise &&) noexcept = delete;
    Promise &operator=(Promise &&) noexcept = delete;
};

/**
 * Result of an event receiving that will be completed normally or exceptionally in the future.
 * @tparam E The event type.
 */
template <typename E>
struct Promise<std::shared_ptr<E>> : CommonPromiseMixin<Promise<std::shared_ptr<E>>>,
                                     EventPromiseMixin<E, Promise<std::shared_ptr<E>>> {
    friend struct CommonPromiseMixin<Promise>;
    friend struct EventPromiseMixin<E, Promise>;

    EventPromiseImpl impl;

    explicit Promise(void *handle) : impl(handle) {
    }

    Promise(const Promise &) = delete;
    Promise &operator=(const Promise &) = delete;
    Promise(Promise &&) noexcept = delete;
    Promise &operator=(Promise &&) noexcept = delete;
};

struct PromiseListImpl {
    void *handle = nullptr;

    explicit PromiseListImpl(void *handle);
    ~PromiseListImpl();

    static std::size_t getSize(void *handle);
    static void *getElement(void *handle, std::size_t index);
};

/**
 * A list of event receiving results that will be completed normally or exceptionally in the future.
 * It is a std::vector<Promise<std::shared_ptr<E>>> wrapper with Graal semantics.
 * @tparam E The event type.
 */
template <typename E> struct PromiseList {
    using data_type = std::vector<Promise<std::shared_ptr<E>>>;

    using iterator_category = std::random_access_iterator_tag;

    using value_type = typename data_type::value_type;
    using allocator_type = typename data_type::allocator_type;
    using pointer = typename data_type::pointer;
    using const_pointer = typename data_type::const_pointer;
    using reference = typename data_type::reference;
    using const_reference = typename data_type::const_reference;
    using size_type = typename data_type::size_type;
    using difference_type = typename data_type::difference_type;

    using iterator = typename data_type::iterator;
    using const_iterator = typename data_type::const_iterator;
    using reverse_iterator = typename data_type::reverse_iterator;
    using const_reverse_iterator = typename data_type::const_reverse_iterator;

    PromiseListImpl impl;

    data_type data_;

    explicit PromiseList(void *handle = nullptr) : impl(handle) {
    }

    std::shared_ptr<PromiseList> create(void *handle) {
        if (!handle) {
            return {};
        }

        std::vector<Promise<std::shared_ptr<E>>> result{};
        auto size = PromiseListImpl::getSize(handle);
        auto promiseList = std::make_shared<PromiseList<E>>(handle);

        for (std::size_t elementIndex = 0; elementIndex < size; elementIndex++) {
            if (auto element = PromiseListImpl::getElement(handle, elementIndex)) {
                promiseList->data_.emplace_back(element, false);
            }
        }

        return promiseList;
    }

    pointer data() noexcept {
        return data_.data();
    }

    const_pointer data() const noexcept {
        return data_.data();
    }

    iterator begin() noexcept {
        return data_.begin();
    }

    const_iterator begin() const noexcept {
        return data_.begin();
    }

    iterator end() noexcept {
        return data_.end();
    }

    const_iterator end() const noexcept {
        return data_.end();
    }

    const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    const_iterator cend() const noexcept {
        return data_.cend();
    }

    reverse_iterator rbegin() noexcept {
        return data_.rbegin();
    }

    reverse_iterator rend() noexcept {
        return data_.rend();
    }

    const_reverse_iterator crbegin() const noexcept {
        return data_.crbegin();
    }

    const_reverse_iterator crend() const noexcept {
        return data_.crend();
    }

    bool empty() const noexcept {
        return data_.empty();
    }

    size_type size() const noexcept {
        return data_.size();
    }

    size_type max_size() const noexcept {
        return data_.max_size();
    }

    size_type capacity() const noexcept {
        return data_.capacity();
    }

    reference operator[](const size_type pos) noexcept {
        return data_[pos];
    }

    const_reference operator[](const size_type pos) const noexcept {
        return data_[pos];
    }

    reference at(const size_type pos) {
        return data_.at(pos);
    }

    const_reference at(const size_type pos) const {
        return data_.at(pos);
    }

    reference front() noexcept {
        return data_.front();
    }

    const_reference front() const noexcept {
        return data_.front();
    }

    reference back() noexcept {
        return data_.back();
    }

    const_reference back() const noexcept {
        return data_.back();
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

    Promise(const Promise &) = delete;
    Promise &operator=(const Promise &) = delete;
    Promise(Promise &&) noexcept = delete;
    Promise &operator=(Promise &&) noexcept = delete;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()