// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "../internal/Handler.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventType.hpp"
#include "../event/IndexedEvent.hpp"
#include "../event/IndexedEventSource.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../internal/context/ApiContext.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

class IndexedEventSource;

/**
 * The listener for receiving indexed events with the specified type (if necessary) from the IndexedTxModel or
 * TimeSeriesTxModel.
 */
struct DXFCPP_EXPORT TxModelListener : RequireMakeShared<TxModelListener> {
    explicit TxModelListener(LockExternalConstructionTag);
    ~TxModelListener() noexcept override;

  private:
    void createHandle(Id<TxModelListener> id);

  public:
    /**
     * Creates a listener for receiving indexed events.
     *
     * ```cpp
     * auto listener = TxModelListener::create([](const auto &, const auto &events, bool isSnapshot) {
     *     if (isSnapshot) {
     *         std::cout << "Snapshot:" << std::endl;
     *     } else {
     *         std::cout << "Update:" << std::endl;
     *     }
     *
     *     for (const auto &e : events) {
     *         if (auto o = e->template sharedAs<Order>()) {
     *              std::cout << "[" << o->getEventFlagsMask().toString() << "]:" << o << std::endl;
     *         }
     *     }
     *
     *     std::cout << std::endl;
     * });
     *
     * builder->withListener(listener);
     * ```
     *
     * @param onEventsReceived A functional object, lambda, or function to which indexed event data will be passed.
     * @return A smart pointer to the listener.
     */
    static std::shared_ptr<TxModelListener>
    create(std::function<void(const IndexedEventSource & /* source */,
                              const std::vector<std::shared_ptr<EventType>> & /* events */, bool /* isSnapshot */)>
               onEventsReceived);

    /**
     * Creates a listener for receiving indexed events (with instantiation by event type `E` and verification)
     *
     * ```cpp
     * auto listener = TxModelListener::create<Order>([](const auto &, const auto &events, bool isSnapshot) {
     *     if (isSnapshot) {
     *         std::cout << "Snapshot:" << std::endl;
     *     } else {
     *         std::cout << "Update:" << std::endl;
     *     }
     *
     *     for (const auto &e : events) {
     *         std::cout << "[" << e->getEventFlagsMask().toString() << "]:" << e << std::endl;
     *     }
     *
     *     std::cout << std::endl;
     * });
     *
     * builder->withListener(listener);
     * ```
     * @tparam E The event type.
     * @param onEventsReceived A functional object, lambda, or function to which indexed event data will be passed.
     * @return A smart pointer to the listener.
     */
    template <Derived<IndexedEvent> E>
    static std::shared_ptr<TxModelListener>
    create(std::function<void(const IndexedEventSource & /* source */,
                              const std::vector<std::shared_ptr<E>> & /* events */, bool /* isSnapshot */)>
               onEventsReceived) {
        auto listener = createShared();
        listener->createHandle(
            ApiContext::getInstance()->getManager<EntityManager<TxModelListener>>()->registerEntity(listener));

        listener->onEventsReceived_ +=
            [l = onEventsReceived](const IndexedEventSource &source,
                                   const std::vector<std::shared_ptr<EventType>> &events, bool isSnapshot) {
                std::vector<std::shared_ptr<E>> filteredEvents{};

                filteredEvents.reserve(events.size());

                for (const auto &e : events) {
                    if (auto expected = e->template sharedAs<E>(); expected) {
                        filteredEvents.emplace_back(expected);
                    }
                }

                l(source, filteredEvents, isSnapshot);
            };

        return listener;
    }

    /**
     * Returns a handle of the current listener.
     *
     * @return The listener's handle.
     */
    const JavaObjectHandle<TxModelListener> &getHandle() const;

  private:
    mutable std::recursive_mutex mutex_{};
    JavaObjectHandle<TxModelListener> handle_;
    SimpleHandler<void(const IndexedEventSource & /* source */,
                       const std::vector<std::shared_ptr<EventType>> & /* events */, bool /* isSnapshot */)>
        onEventsReceived_{};

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

DXFCPP_END_NAMESPACE