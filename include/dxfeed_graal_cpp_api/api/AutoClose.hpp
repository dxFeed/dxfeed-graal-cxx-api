// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/CallbackBarrier.hpp"

#include <memory>
#include <utility>

/**
 * @page entity_lifetime Lifetime of the entities
 *
 * The entities of the library live as in the Java API: an entity that is closed is released when the last pointer to
 * it is released, and the library does not call its listeners after it is closed.
 *
 * **Owners.** Releasing the last pointer of the user does not close everything:
 *
 * - A subscription created by DXFeed::createSubscription() or attached by DXFeed::attachSubscription() is owned by the
 *   feed: it receives events until DXFeedSubscription::close(), DXFeedSubscription::detach() or the close of the
 *   endpoint, even without a pointer of the user. A subscription without a feed is closed with its last pointer.
 * - An endpoint created by DXEndpoint::create() or DXEndpoint::Builder::build() lives until DXEndpoint::close() or
 *   DXEndpoint::closeAndAwaitTermination(); then it is released with its feed, publisher and OnDemandService when the
 *   user releases it. The default endpoints of DXEndpoint::getInstance(), DXFeed::getInstance() and
 *   DXPublisher::getInstance() stay for the whole process.
 * - The transaction models, MarketDepthModel, InstrumentProfileConnection and PriceLevelService are closed with their
 *   last pointer. A model owns its listener, an InstrumentProfileConnection owns its collector, an endpoint owns its
 *   OnDemandService.
 * - A listener that keeps a pointer to the entity it listens to keeps that entity: keep a `std::weak_ptr` in it.
 *
 * An endpoint is not closed with its last pointer, as in the Java API, where an endpoint is not closed by the garbage
 * collector. `DXEndpoint::create()->connect(address)->getFeed()` keeps no pointer to the endpoint, and the feed does
 * not own its endpoint: closing it there would leave the feed without data and without an error. The last pointer can
 * also be released in a listener on a thread of the library or at the exit of the process, where close() could block
 * or fail.
 *
 * AutoClose closes an entity when it goes out of scope, as try-with-resources does in Java.
 *
 * **Closing elsewhere.** To close an entity in another scope or on another thread, keep a `std::shared_ptr` to it and
 * call close() there: close() can be called from any thread and more than once. Or move the AutoClose there: return
 * it from a function, keep it in a member of the owner or move it into a lambda.
 *
 * ```cpp
 * struct Session {
 *     dxfcpp::AutoClose<dxfcpp::DXEndpoint> endpoint; // Closed when the Session is destroyed.
 * };
 *
 * std::thread worker{[endpoint = dxfcpp::AutoClose{dxfcpp::DXEndpoint::create()->connect(address)}] {
 *     // ...
 * }}; // The endpoint is closed when the thread ends.
 * ```
 *
 * One AutoClose must not be used from two threads at once, as one `std::shared_ptr` must not; moving it to another
 * thread is fine.
 *
 * **close().** DXFeedSubscription::close() returns after the calls of its listeners in progress on other threads
 * end; called from a listener of the library, it does not wait. A batch of events that the Java code delivers after
 * close() is dropped. A subscription or an endpoint can be closed from a listener of a subscription. `CLOSED` is the
 * last state notification of an endpoint. A closed subscription delivers no events; adding a listener to it, attaching
 * it to a feed or closing it again does nothing.
 *
 * DXEndpoint::close() stops the events, but it does not wait for the calls of the listeners in progress: the
 * subscriptions of the endpoint can be released on another thread. Close the subscription whose listener uses the
 * objects of a scope before these objects are destroyed, for example with an AutoClose declared after them.
 *
 * **Executor.** DXEndpoint::executor() called after DXEndpoint::getFeed() applies to the next events of the existing
 * subscriptions, as in the Java API.
 *
 * **Exit.** At the exit of the process the library does not close the endpoints: the listeners that use the objects
 * of the program can be called while these objects are being destroyed. Close the endpoints before that, or call
 * dxfcpp::shutdown() (see its documentation).
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * \addtogroup dxfcpp_api
 * @{
 */

/**
 * Closes an entity when it goes out of scope: a subscription, an endpoint, a model, an instrument profile connection
 * or any other entity with a `close()` method. See @ref entity_lifetime "Lifetime of the entities".
 *
 * The AutoClose is moved, not copied: one AutoClose closes the entity once. The other pointers to the entity stay
 * valid and point to the closed entity. An exception of `close()` in the destructor or in reset() is reported to
 * ErrorManager.
 *
 * Do not wrap a default endpoint (DXEndpoint::getInstance()): closing it closes it for the whole process.
 *
 * ```cpp
 * const dxfcpp::AutoClose endpoint{dxfcpp::DXEndpoint::create()->connect("demo.dxfeed.com:7300")};
 * const dxfcpp::AutoClose subscription{endpoint->getFeed()->createSubscription(dxfcpp::Quote::TYPE)};
 *
 * subscription->addEventListener<dxfcpp::Quote>([&](const auto &quotes) {
 *     // ...
 * });
 * subscription->addSymbols("AAPL");
 * // ...
 * // Here the subscription is closed first: its listener is not called after the scope. Then the endpoint is closed.
 * ```
 *
 * @tparam T The type of the entity.
 */
template <typename T> class AutoClose final {
    std::shared_ptr<T> entity_{};

    public:
    /// Creates an empty AutoClose.
    AutoClose() noexcept = default;

    /**
     * Creates an AutoClose that closes the entity.
     *
     * @param entity The entity or an empty pointer.
     */
    explicit AutoClose(std::shared_ptr<T> entity) noexcept : entity_{std::move(entity)} {
    }

    AutoClose(const AutoClose &) = delete;
    AutoClose &operator=(const AutoClose &) = delete;

    /// Takes the entity of the other AutoClose, which becomes empty.
    AutoClose(AutoClose &&other) noexcept : entity_{std::move(other.entity_)} {
    }

    /// Closes the current entity and takes the entity of the other AutoClose, which becomes empty.
    AutoClose &operator=(AutoClose &&other) noexcept {
        if (this != &other) {
            reset();
            entity_ = std::move(other.entity_);
        }

        return *this;
    }

    /// Closes the entity.
    ~AutoClose() noexcept {
        reset();
    }

    /// Closes the entity now and makes this AutoClose empty.
    void reset() noexcept {
        if (auto entity = std::move(entity_)) {
            runRelease("AutoClose", [&entity] {
                entity->close();
            });
        }
    }

    /**
     * Makes this AutoClose empty without closing the entity.
     *
     * @return The entity or an empty pointer.
     */
    [[nodiscard]] std::shared_ptr<T> release() noexcept {
        return std::move(entity_);
    }

    /// @return The entity or an empty pointer.
    [[nodiscard]] const std::shared_ptr<T> &get() const noexcept {
        return entity_;
    }

    /// @return The entity; this AutoClose must not be empty.
    T *operator->() const noexcept {
        return entity_.get();
    }

    /// @return The entity; this AutoClose must not be empty.
    T &operator*() const noexcept {
        return *entity_;
    }

    /// @return `true` if this AutoClose has an entity.
    explicit operator bool() const noexcept {
        return static_cast<bool>(entity_);
    }
};

/** @} */ // end of dxfcpp_api

DXFCPP_END_NAMESPACE
