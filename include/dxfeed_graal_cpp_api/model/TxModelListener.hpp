// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventType.hpp"
#include "../event/IndexedEvent.hpp"
#include "../event/IndexedEventSource.hpp"
#include "../event/TimeSeriesEvent.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Id.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../internal/managers/EntityManager.hpp"
#include "../internal/context/ApiContext.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

class IndexedEventSource;

struct DXFCPP_EXPORT TxModelListenerTag{};

struct DXFCPP_EXPORT TxModelListenerCommon : virtual SharedEntity {
    TxModelListenerCommon();
    ~TxModelListenerCommon() noexcept override;

    /**
     * Returns a handle of the current listener.
     *
     * @return The listener's handle.
     */
    const JavaObjectHandle<TxModelListenerTag> &getHandle() const;

    std::string toString() const override;

    friend std::ostream &operator<<(std::ostream &os, const TxModelListenerCommon &l) {
        return os << l.toString();
    }

    std::size_t hashCode() const;

    bool operator==(const TxModelListenerCommon &other) const noexcept;

    protected:
    mutable std::recursive_mutex mutex_{};
    JavaObjectHandle<TxModelListenerTag> handle_;
    SimpleHandler<void(const IndexedEventSource & /* source */,
                       const std::vector<std::shared_ptr<EventType>> & /* events */, bool /* isSnapshot */)>
        onEventsReceived_{};

    struct Impl;

    std::unique_ptr<Impl> impl_;

    void createHandle(Id<TxModelListenerTag> id);
};

/**
 * The listener for receiving indexed events with the specified type from the IndexedTxModel.
 */
template <Derived<IndexedEvent> E>
struct /* DXFCPP_EXPORT */ IndexedTxModelListener final : TxModelListenerCommon,
                                                    RequireMakeShared<IndexedTxModelListener<E>> {
    IndexedTxModelListener(RequireMakeShared<IndexedTxModelListener<E>>::LockExternalConstructionTag) {};

    ~IndexedTxModelListener() noexcept override {
    }

    /**
     * Creates a listener for receiving indexed events (with instantiation by event type `E` and verification)
     *
     * ```cpp
     * auto listener = IndexedTxModelListener<Order>::create([](const auto & orderSource, const auto &events, bool
     * isSnapshot) { if (isSnapshot) { std::cout << "Snapshot:" << std::endl; } else { std::cout << "Update:" <<
     * std::endl;
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
    static std::shared_ptr<IndexedTxModelListener<E>>
    create(std::function<void(const IndexedEventSource & /* source */,
                              const std::vector<std::shared_ptr<E>> & /* events */, bool /* isSnapshot */)>
               onEventsReceived) {
        auto listener = RequireMakeShared<IndexedTxModelListener<E>>::createShared();
        listener->createHandle(ApiContext::getInstance()
                                   ->getManager<EntityManager<TxModelListenerCommon, TxModelListenerTag>>()
                                   ->registerEntity(listener));

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

    std::string toString() const override {
        return TxModelListenerCommon::toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const IndexedTxModelListener<E> &l) {
        return os << l.toString();
    }

    bool operator==(const IndexedTxModelListener<E> &other) const noexcept {
        return TxModelListenerCommon::operator==(other);
    }
};

/**
 * The listener for receiving time series events with the specified type from the TimeSeriesTxModel.
 */
template <Derived<TimeSeriesEvent> E>
struct /* DXFCPP_EXPORT */ TimeSeriesTxModelListener final : TxModelListenerCommon,
                                                       RequireMakeShared<TimeSeriesTxModelListener<E>> {
    TimeSeriesTxModelListener(RequireMakeShared<TimeSeriesTxModelListener<E>>::LockExternalConstructionTag) {};

    ~TimeSeriesTxModelListener() noexcept override {
    }

    /**
     * Creates a listener for receiving time series events (with instantiation by event type `E` and verification)
     *
     * ```cpp
     * auto listener = TimeSeriesTxModelListener<Candle>::create([](const auto &events, bool isSnapshot) {
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
    static std::shared_ptr<TimeSeriesTxModelListener<E>>
    create(std::function<void(const std::vector<std::shared_ptr<E>> & /* events */, bool /* isSnapshot */)>
               onEventsReceived) {
        auto listener = RequireMakeShared<TimeSeriesTxModelListener<E>>::createShared();
        listener->createHandle(ApiContext::getInstance()
                                   ->getManager<EntityManager<TxModelListenerCommon, TxModelListenerTag>>()
                                   ->registerEntity(listener));

        listener->onEventsReceived_ +=
            [l = onEventsReceived](const IndexedEventSource &, const std::vector<std::shared_ptr<EventType>> &events,
                                   bool isSnapshot) {
                std::vector<std::shared_ptr<E>> filteredEvents{};

                filteredEvents.reserve(events.size());

                for (const auto &e : events) {
                    if (auto expected = e->template sharedAs<E>(); expected) {
                        filteredEvents.emplace_back(expected);
                    }
                }

                l(filteredEvents, isSnapshot);
            };

        return listener;
    }

    std::string toString() const override {
        return TxModelListenerCommon::toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const TimeSeriesTxModelListener<E> &l) {
        return os << l.toString();
    }

    bool operator==(const TimeSeriesTxModelListener<E> &other) const noexcept {
        return TxModelListenerCommon::operator==(other);
    }
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::TxModelListenerCommon> {
    std::size_t operator()(const dxfcpp::TxModelListenerCommon &l) const noexcept {
        return l.hashCode();
    }
};

template <dxfcpp::Derived<dxfcpp::IndexedEvent> E> struct std::hash<dxfcpp::IndexedTxModelListener<E>> {
    std::size_t operator()(const dxfcpp::IndexedTxModelListener<E> &l) const noexcept {
        return l.hashCode();
    }
};

template <dxfcpp::Derived<dxfcpp::TimeSeriesEvent> E> struct std::hash<dxfcpp::TimeSeriesTxModelListener<E>> {
    std::size_t operator()(const dxfcpp::TimeSeriesTxModelListener<E> &l) const noexcept {
        return l.hashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()