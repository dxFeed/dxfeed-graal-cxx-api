// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace dxfcpp::detail {

/**
 * A thread-safe class that allows to asynchronously notify listeners with a given signature.
 * Listeners can be any callable entities.
 * This class executes incoming events sequentially using a simple fixed-length circular buffer implementation.
 *
 * @tparam Signature The arguments "signature" (example: `void(int, int)`)
 */
template <typename Signature> struct Handler;

/**
 * A thread-safe class that allows to asynchronously notify listeners with a given signature.
 * Listeners can be any callable entities.
 * This class executes incoming events sequentially using a simple fixed-length circular buffer implementation.
 *
 * @tparam ArgTypes The arguments "signature" (example: `void(int, int)`)
 */
template <typename... ArgTypes> struct Handler<void(ArgTypes...)> final {
    /// The listener type
    using ListenerType = std::function<void(ArgTypes...)>;

  private:
    static const unsigned MAIN_FUTURES_DEFAULT_SIZE = 1024;

    std::recursive_mutex listenersMutex_{};
    std::unordered_map<std::size_t, ListenerType> listeners_{};
    std::unordered_map<std::size_t, ListenerType> lowPriorityListeners_{};
    std::size_t lastId_{};

    std::recursive_mutex mainFuturesMutex_{};
    std::vector<std::shared_future<void>> mainFutures_{};
    std::size_t mainFuturesCurrentIndex_{};
    std::size_t mainFuturesSize_{};

    std::shared_future<void> handleImpl(ArgTypes &&...args) {
        return std::async(
            std::launch::async,
            [this](ArgTypes &&...args) {
                std::lock_guard guard{listenersMutex_};

                for (auto &listener : listeners_) {
                    listener.second(args...);
                }

                for (auto &listener : lowPriorityListeners_) {
                    listener.second(args...);
                }
            },
            std::forward<ArgTypes>(args)...);
    }

  public:
    /**
     * Creates the new handler by specified size of circular buffer of futures
     *
     * @param mainFuturesSize The size of the circular buffer of futures
     */
    explicit Handler(std::size_t mainFuturesSize = MAIN_FUTURES_DEFAULT_SIZE)
        : mainFuturesCurrentIndex_{0ULL}, mainFuturesSize_{mainFuturesSize} {
        mainFutures_.reserve(mainFuturesSize);
    }

    /**
     * Calls the listeners and pass the args to them
     *
     * @param args The listeners arguments
     */
    void handle(ArgTypes &&...args) {
        auto f = handleImpl(std::forward<ArgTypes>(args)...);

        {
            std::lock_guard guard{mainFuturesMutex_};

            if (mainFutures_.size() < mainFuturesSize_) {
                mainFutures_.emplace_back(f);
            } else {
                mainFuturesCurrentIndex_ = mainFuturesCurrentIndex_ % mainFuturesSize_;
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
    void operator()(ArgTypes &&...args) { return handle(std::forward<ArgTypes>(args)...); }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t add(ListenerType &&listener) {
        std::lock_guard guard{listenersMutex_};

        lastId_++;
        listeners_.emplace(lastId_, std::forward<ListenerType>(listener));

        return lastId_;
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

        lastId_++;
        lowPriorityListeners_.emplace(lastId_, std::forward<ListenerType>(listener));

        return lastId_;
    }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t operator+=(ListenerType &&listener) { return add(std::forward<ListenerType>(listener)); }

    /**
     * Adds the low priority listener (to the "low priority" group).
     * It will be called after the "main" listeners
     *
     * @param listener The listener
     * @return The listener id
     */
    std::size_t operator%=(ListenerType &&listener) { return addLowPriority(std::forward<ListenerType>(listener)); }

    /**
     * Removes a listener by the id
     *
     * @param id The listener id
     */
    void remove(std::size_t id) {
        std::lock_guard guard{listenersMutex_};

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
    void operator-=(std::size_t id) { return remove(id); }
};

} // namespace dxfcpp::detail