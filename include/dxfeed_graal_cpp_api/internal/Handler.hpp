// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../exceptions/ErrorManager.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

/**
 * The call of a listener of SimpleHandler in progress on the current thread.
 *
 * SimpleHandler::remove() does not wait for the calls in progress when the current thread calls a listener or a
 * callback of the library: the wait could deadlock (a listener that removes itself, two listeners that remove each
 * other on two threads). The counter is in the library, so it is one for the process, not one for each module.
 */
struct DXFCPP_EXPORT ListenerCallScope final {
    ListenerCallScope() noexcept;
    ~ListenerCallScope() noexcept;

    ListenerCallScope(const ListenerCallScope &) = delete;
    ListenerCallScope &operator=(const ListenerCallScope &) = delete;

    /**
     * @return `true` if the current thread calls a listener of SimpleHandler or a callback of the library.
     */
    static bool isActive() noexcept;
};

/**
 * A thread-safe list of listeners with a given signature. Listeners can be any callable entities.
 *
 * handle() calls the listeners on the current thread (the thread that notifies): the "main" listeners, then the "low
 * priority" ones, in the order of addition. It takes a snapshot of the list under a lock and calls the listeners
 * outside of it, so a listener can add or remove listeners and call handle() of the same handler. A listener added
 * during a notification is called from the next one. If the source notifies on several threads at once, the listeners
 * are called on these threads at once.
 *
 * An exception of a listener is reported to ErrorManager; the other listeners are still called.
 *
 * @tparam Signature The arguments "signature" (example: `void(int, int)`)
 */
template <typename Signature> struct SimpleHandler;

/**
 * A thread-safe list of listeners with a given signature. See SimpleHandler.
 *
 * @tparam ArgTypes The arguments "signature" (example: `void(int, int)`)
 */
template <typename... ArgTypes> struct SimpleHandler<void(ArgTypes...)> final {
    using ListenerType = std::function<void(ArgTypes...)>;
    static constexpr std::size_t FAKE_ID{static_cast<std::size_t>(-1)};

    private:
    struct Listener final {
        const std::size_t id;
        const ListenerType function;

        std::mutex mutex{};
        std::condition_variable callsEnded{};
        std::size_t calls{};
        bool removed{};

        Listener(std::size_t listenerId, ListenerType &&listener)
            : id{listenerId}, function{std::forward<ListenerType>(listener)} {
        }
    };

    // The list is not changed after it is published: add() and remove() publish a new one, so handle() takes a
    // snapshot by copying the pointer. A null list is empty.
    using List = std::vector<std::shared_ptr<Listener>>;

    const char *component_{"SimpleHandler"};

    std::mutex mutex_{};
    std::shared_ptr<const List> listeners_{};
    std::size_t mainListenersCount_{}; // the "main" listeners go first in the list, then the "low priority" ones

    // The listener ids are unique within this handler only: they identify the listener for remove(). The counter is not
    // static: a static member of a template has a copy in every module that instantiates it (on Windows: the shared
    // library and each executable), so the ids of listeners added from the application and from the library collided.
    // Guarded by mutex_.
    std::size_t lastId_{};

    // After the listeners are swapped, both handlers continue after the larger of the two counters, so that no id that
    // is in use in either handler is given out again. Called with the mutexes of both handlers held.
    template <typename OtherHandler> void syncLastIds(OtherHandler &other) noexcept {
        const auto last = std::max(lastId_, other.lastId_);

        lastId_ = last;
        other.lastId_ = last;
    }

    void call(Listener &listener, ArgTypes &...args) noexcept {
        {
            std::lock_guard guard{listener.mutex};

            if (listener.removed) {
                return;
            }

            listener.calls++;
        }

        try {
            listener.function(args...);
        } catch (...) {
            // A view of the name: copying it into a string could throw std::bad_alloc out of this noexcept function.
            ErrorManager::reportException(std::string_view{component_}, std::current_exception());
        }

        std::lock_guard guard{listener.mutex};

        if (--listener.calls == 0 && listener.removed) {
            listener.callsEnded.notify_all();
        }
    }

    std::size_t addImpl(ListenerType &&listener, bool lowPriority) {
        std::lock_guard guard{mutex_};

        if (lastId_ >= FAKE_ID - 1) {
            return FAKE_ID;
        }

        const auto id = lastId_ + 1;
        auto list = listeners_ ? std::make_shared<List>(*listeners_) : std::make_shared<List>();
        const auto position = lowPriority ? list->size() : mainListenersCount_;

        list->insert(list->begin() + static_cast<typename List::difference_type>(position),
                     std::make_shared<Listener>(id, std::forward<ListenerType>(listener)));

        listeners_ = std::move(list);
        lastId_ = id;

        if (!lowPriority) {
            mainListenersCount_++;
        }

        return id;
    }

    public:
    /**
     * Creates the new handler. An exception of a listener is reported to ErrorManager with the component
     * "SimpleHandler".
     */
    SimpleHandler() noexcept = default;

    /**
     * Creates the new handler.
     *
     * @param component The part of the library that owns the handler, for ErrorInfo::component of an exception of a
     * listener. The string must outlive the handler (a string literal).
     */
    explicit SimpleHandler(const char *component) noexcept : component_{component} {
    }

    SimpleHandler(const SimpleHandler &) = delete;

    SimpleHandler(SimpleHandler &&other) noexcept : component_{other.component_} {
        std::lock_guard guard{other.mutex_};

        listeners_ = std::exchange(other.listeners_, nullptr);
        mainListenersCount_ = std::exchange(other.mainListenersCount_, 0);
        lastId_ = other.lastId_; // the moved listeners keep their ids
    }

    SimpleHandler &operator=(const SimpleHandler &) = delete;

    SimpleHandler &operator=(SimpleHandler &&other) noexcept {
        std::scoped_lock lock(mutex_, other.mutex_);

        listeners_.swap(other.listeners_);
        std::swap(mainListenersCount_, other.mainListenersCount_);
        syncLastIds(other);

        return *this;
    }

    ~SimpleHandler() noexcept = default;

    /**
     * Calls the listeners on the current thread and passes the args to them.
     *
     * @param args The listeners arguments
     */
    void handle(ArgTypes... args) noexcept {
        std::shared_ptr<const List> listeners{};

        {
            std::lock_guard guard{mutex_};

            listeners = listeners_;
        }

        if (!listeners) {
            return;
        }

        const ListenerCallScope callScope{};

        for (const auto &listener : *listeners) {
            call(*listener, args...);
        }
    }

    /**
     * Calls the listeners on the current thread and passes the args to them.
     *
     * @param args The listeners arguments
     */
    void operator()(ArgTypes... args) noexcept {
        return handle(args...);
    }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id, unique within this handler (not across handlers)
     */
    std::size_t add(ListenerType &&listener) {
        return addImpl(std::forward<ListenerType>(listener), false);
    }

    /**
     * Adds the low priority listener (to the "low priority" group)
     * It will be called after the "main" listeners
     *
     * @param listener The listener
     * @return The listener id, unique within this handler (not across handlers)
     */
    std::size_t addLowPriority(ListenerType &&listener) {
        return addImpl(std::forward<ListenerType>(listener), true);
    }

    /**
     * Adds the listener to "main" group
     *
     * @param listener The listener
     * @return The listener id, unique within this handler (not across handlers)
     */
    std::size_t operator+=(ListenerType &&listener) {
        return add(std::forward<ListenerType>(listener));
    }

    /**
     * Adds the low priority listener (to the "low priority" group).
     * It will be called after the "main" listeners
     *
     * @param listener The listener
     * @return The listener id, unique within this handler (not across handlers)
     */
    std::size_t operator%=(ListenerType &&listener) {
        return addLowPriority(std::forward<ListenerType>(listener));
    }

    /**
     * Removes a listener by the id.
     *
     * The listener is not called after this function returns: it waits for the calls of the listener in progress on
     * the other threads. Called from a listener or a callback of the library, it does not wait (see
     * ListenerCallScope): a call in progress on another thread may then still run.
     *
     * @param id The listener id
     */
    void remove(std::size_t id) {
        if (id == FAKE_ID) {
            return;
        }

        std::shared_ptr<Listener> removed{};

        {
            std::lock_guard guard{mutex_};

            if (!listeners_) {
                return;
            }

            const auto found = std::find_if(listeners_->begin(), listeners_->end(), [id](const auto &listener) {
                return listener->id == id;
            });

            if (found == listeners_->end()) {
                return;
            }

            const auto index = static_cast<std::size_t>(found - listeners_->begin());
            auto list = std::make_shared<List>(*listeners_);

            removed = *found;
            list->erase(list->begin() + static_cast<typename List::difference_type>(index));
            listeners_ = std::move(list);

            if (index < mainListenersCount_) {
                mainListenersCount_--;
            }
        }

        std::unique_lock lock{removed->mutex};

        removed->removed = true;

        if (!ListenerCallScope::isActive()) {
            removed->callsEnded.wait(lock, [&removed] {
                return removed->calls == 0;
            });
        }
    }

    /**
     * Removes a listener by the id. See remove().
     *
     * @param id The listener id
     */
    void operator-=(std::size_t id) {
        return remove(id);
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
