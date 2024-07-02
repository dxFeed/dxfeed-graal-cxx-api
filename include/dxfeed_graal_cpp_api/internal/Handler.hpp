// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <unordered_map>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * A thread-safe class that allows to asynchronously notify listeners with a given signature.
 * Listeners can be any callable entities.
 * Listeners are placed in a future. Within one batch, listeners will be called sequentially. Futures are placed in
 * a circular buffer and executed asynchronously.
 *
 * If you need synchronous execution of listeners, but it was possible to execute them in another thread, then limit
 * the buffer size to one.
 *
 * @tparam Signature The arguments "signature" (example: `void(int, int)`)
 */
template <typename Signature> struct Handler;

/**
 * A thread-safe class that allows to asynchronously notify listeners with a given signature.
 * Listeners can be any callable entities.
 * Listeners are placed in a future. Within one batch, listeners will be called sequentially. Futures are placed in
 * a circular buffer and executed asynchronously.
 *
 * If you need synchronous execution of listeners, but it was possible to execute them in another thread, then limit
 * the buffer size to one.
 *
 * @tparam ArgTypes The arguments "signature" (example: `void(int, int)`)
 */
template <typename... ArgTypes> struct Handler<void(ArgTypes...)> final {
    /// The listener type
    using ListenerType = std::function<void(ArgTypes...)>;
    static constexpr std::size_t FAKE_ID{static_cast<std::size_t>(-1)};

  private:
    static constexpr unsigned MAIN_FUTURES_DEFAULT_SIZE = 1;

    std::recursive_mutex listenersMutex_{};
    std::unordered_map<std::size_t, ListenerType> listeners_{};
    std::unordered_map<std::size_t, ListenerType> lowPriorityListeners_{};

    inline static std::atomic<std::size_t> lastId_{};

    std::recursive_mutex mainFuturesMutex_{};
    std::vector<std::shared_future<void>> mainFutures_{};
    std::size_t mainFuturesCurrentIndex_{};
    std::size_t mainFuturesSize_{};

    std::shared_future<void> handleImpl(ArgTypes... args) {
        return std::async(
            std::launch::async,
            [this](ArgTypes... args) {
                std::lock_guard guard{listenersMutex_};

                for (auto &listener : listeners_) {
                    listener.second(args...);
                }

                for (auto &listener : lowPriorityListeners_) {
                    listener.second(args...);
                }
            },
            args...);
    }

  public:
    /**
     * Creates the new handler by specified size of circular buffer of futures
     *
     * @param mainFuturesSize The size of the circular buffer of futures
     */
    explicit Handler(std::size_t mainFuturesSize = MAIN_FUTURES_DEFAULT_SIZE) noexcept
        : mainFuturesCurrentIndex_{0ULL}, mainFuturesSize_{mainFuturesSize} {
        if (mainFuturesSize > 1) {
            mainFutures_.reserve(mainFuturesSize);
        }
    }

    Handler(const Handler &) = delete;

    Handler(Handler &&other) noexcept {
        std::scoped_lock lock(other.listenersMutex_, other.mainFuturesMutex_);

        listeners_.swap(other.listeners_);
        lowPriorityListeners_.swap(other.lowPriorityListeners_);
        mainFutures_.swap(other.mainFutures_);
        mainFuturesCurrentIndex_ = other.mainFuturesCurrentIndex_;
        mainFuturesSize_ = other.mainFuturesSize_;
    }

    Handler &operator=(const Handler &) = delete;

    Handler &operator=(Handler &&other) noexcept {
        std::scoped_lock lock(listenersMutex_, mainFuturesMutex_, other.listenersMutex_, other.mainFuturesMutex_);

        listeners_.swap(other.listeners_);
        lowPriorityListeners_.swap(other.lowPriorityListeners_);
        mainFutures_.swap(other.mainFutures_);
        mainFuturesCurrentIndex_ = other.mainFuturesCurrentIndex_;
        mainFuturesSize_ = other.mainFuturesSize_;

        return *this;
    }

    /**
     * Calls the listeners and pass the args to them
     *
     * @param args The listeners arguments
     */
    void handle(ArgTypes... args) {
        auto f = handleImpl(args...);

        if (mainFuturesSize_ <= 1) {
            f.wait();
        } else {
            std::lock_guard guard{mainFuturesMutex_};

            if (mainFutures_.size() < mainFuturesSize_) {
                mainFutures_.emplace_back(f);
            } else {
                mainFuturesCurrentIndex_ %= mainFuturesSize_;
                mainFutures_[mainFuturesCurrentIndex_].wait();
                mainFutures_[mainFuturesCurrentIndex_] = f;
                mainFuturesCurrentIndex_++;
            }
        }
    }

    /**
     * Calls the listeners and pass the ars to them
     *
     * @param args The listeners arguments
     */
    void operator()(ArgTypes... args) {
        return handle(args...);
    }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t add(ListenerType &&listener) {
        std::lock_guard guard{listenersMutex_};

        if (lastId_ >= FAKE_ID - 1) {
            return FAKE_ID;
        }

        auto id = ++lastId_;

        listeners_.emplace(id, std::forward<ListenerType>(listener));

        return id;
    }

    /**
     * Adds the low priority listener (to the "low priority" group)
     * It will be called after the "main" listeners
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t addLowPriority(ListenerType &&listener) {
        std::lock_guard guard{listenersMutex_};

        if (lastId_ >= FAKE_ID - 1) {
            return FAKE_ID;
        }

        auto id = ++lastId_;

        lowPriorityListeners_.emplace(id, std::forward<ListenerType>(listener));

        return id;
    }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t operator+=(ListenerType &&listener) {
        return add(std::forward<ListenerType>(listener));
    }

    /**
     * Adds the low priority listener (to the "low priority" group).
     * It will be called after the "main" listeners
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t operator%=(ListenerType &&listener) {
        return addLowPriority(std::forward<ListenerType>(listener));
    }

    /**
     * Removes a listener by the id
     *
     * @param id The listener id
     */
    void remove(std::size_t id) {
        std::lock_guard guard{listenersMutex_};

        if (id == FAKE_ID) {
            return;
        }

        if (listeners_.count(id) > 0) {
            listeners_.erase(id);
        } else if (lowPriorityListeners_.count(id) > 0) {
            lowPriorityListeners_.erase(id);
        }
    }

    /**
     * Removes a listener by the id
     *
     * @param id The listener id
     */
    void operator-=(std::size_t id) {
        return remove(id);
    }
};

/**
 * A thread-safe class that allows to asynchronously notify listeners with a given signature.
 * Listeners can be any callable entities.
 * Listeners are placed in one future without queues.
 *
 * @tparam Signature The arguments "signature" (example: `void(int, int)`)
 */
template <typename Signature> struct SimpleHandler;

/**
 * A thread-safe class that allows to asynchronously notify listeners with a given signature.
 * Listeners can be any callable entities.
 * Listeners are placed in one future without queues.
 *
 * @tparam ArgTypes The arguments "signature" (example: `void(int, int)`)
 */
template <typename... ArgTypes> struct SimpleHandler<void(ArgTypes...)> final {
    using ListenerType = std::function<void(ArgTypes...)>;
    static constexpr std::size_t FAKE_ID{static_cast<std::size_t>(-1)};

  private:
    std::recursive_mutex listenersMutex_{};
    std::unordered_map<std::size_t, ListenerType> listeners_{};
    std::unordered_map<std::size_t, ListenerType> lowPriorityListeners_{};

    inline static std::atomic<std::size_t> lastId_{};

    std::shared_future<void> handleImpl(ArgTypes... args) {
        return std::async(
            std::launch::async,
            [this](ArgTypes... args) {
                std::lock_guard guard{listenersMutex_};

                for (auto &listener : listeners_) {
                    listener.second(args...);
                }

                for (auto &listener : lowPriorityListeners_) {
                    listener.second(args...);
                }
            },
            args...);
    }

  public:
    /**
     * Creates the new handler
     */
    SimpleHandler() noexcept = default;
    SimpleHandler(const SimpleHandler &) = delete;

    SimpleHandler(SimpleHandler &&other) noexcept {
        std::scoped_lock lock(other.listenersMutex_);

        listeners_.swap(other.listeners_);
        lowPriorityListeners_.swap(other.lowPriorityListeners_);
    }

    SimpleHandler &operator=(const SimpleHandler &) = delete;

    SimpleHandler &operator=(SimpleHandler &&other) noexcept {
        std::scoped_lock lock(listenersMutex_, other.listenersMutex_);

        listeners_.swap(other.listeners_);
        lowPriorityListeners_.swap(other.lowPriorityListeners_);

        return *this;
    }

    /**
     * Calls the listeners and pass the args to them
     *
     * @param args The listeners arguments
     */
    void handle(ArgTypes... args) {
        auto f = handleImpl(args...);

        f.wait();
    }

    /**
     * Calls the listeners and pass the ars to them
     *
     * @param args The listeners arguments
     */
    void operator()(ArgTypes... args) {
        return handle(args...);
    }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t add(ListenerType &&listener) {
        std::lock_guard guard{listenersMutex_};

        if (lastId_ >= FAKE_ID - 1) {
            return FAKE_ID;
        }

        auto id = ++lastId_;

        listeners_.emplace(id, std::forward<ListenerType>(listener));

        return id;
    }

    /**
     * Adds the low priority listener (to the "low priority" group)
     * It will be called after the "main" listeners
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t addLowPriority(ListenerType &&listener) {
        std::lock_guard guard{listenersMutex_};

        if (lastId_ >= FAKE_ID - 1) {
            return FAKE_ID;
        }

        auto id = ++lastId_;

        lowPriorityListeners_.emplace(id, std::forward<ListenerType>(listener));

        return id;
    }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t operator+=(ListenerType &&listener) {
        return add(std::forward<ListenerType>(listener));
    }

    /**
     * Adds the low priority listener (to the "low priority" group).
     * It will be called after the "main" listeners
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t operator%=(ListenerType &&listener) {
        return addLowPriority(std::forward<ListenerType>(listener));
    }

    /**
     * Removes a listener by the id
     *
     * @param id The listener id
     */
    void remove(std::size_t id) {
        std::lock_guard guard{listenersMutex_};

        if (id == FAKE_ID) {
            return;
        }

        if (listeners_.count(id) > 0) {
            listeners_.erase(id);
        } else if (lowPriorityListeners_.count(id) > 0) {
            lowPriorityListeners_.erase(id);
        }
    }

    /**
     * Removes a listener by the id
     *
     * @param id The listener id
     */
    void operator-=(std::size_t id) {
        return remove(id);
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()