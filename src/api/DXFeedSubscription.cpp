// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/DXFeedSubscription.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../include/dxfeed_graal_cpp_api/api/osub/ObservableSubscriptionChangeListener.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeedSubscription.hpp"
#if defined(DXFCXX_ENABLE_METRICS)
#    include "../../include/dxfeed_graal_cpp_api/internal/Metrics.hpp"
#endif
#include "../../include/dxfeed_graal_cpp_api/internal/StopWatch.hpp"

#include <algorithm>
#include <dxfg_api.h>
#include <exception>
#include <fmt/format.h>
#include <memory>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

struct DXFeedSubscription::Impl {
    static auto getManager() {
        return ApiContext::getInstance()->getManager<WeakEntityManager<DXFeedSubscription>>();
    }

    static void onEvents(graal_isolatethread_t * /*thread*/, dxfg_event_type_list *graalNativeEvents, void *userData) {
        runCallback("DXFeedSubscription", [&] {
#if defined(DXFCXX_ENABLE_METRICS)
            StopWatch sw{};
#endif

            const auto id = Id<DXFeedSubscription>::from(dxfcpp::bit_cast<Id<DXFeedSubscription>::ValueType>(userData));

            if (const auto sub = Impl::getManager()->getEntity(id)) {
                // The batch came after close(): it is dropped.
                if (sub->closed_) {
                    return;
                }

#if defined(DXFCXX_ENABLE_METRICS)
                sw.start();
#endif
                auto &&events = EventMapper::fromGraalListSkippingErrors(static_cast<void *>(graalNativeEvents));

                // All the events of the batch were skipped: a listener gets non-empty batches only.
                if (events.empty()) {
                    return;
                }

#if defined(DXFCXX_ENABLE_METRICS)
                sw.stop();

                const auto elapsed = sw.elapsedInNanos().count();
                const auto perEvent = static_cast<double>(elapsed) / static_cast<double>(events.size());
                const auto metricsManager = ApiContext::getInstance()->getManager<MetricsManager>();

                metricsManager->set("DXFCXX.Sub.onEvents.fromGraalList.Batch(ns)", elapsed);
                metricsManager->set("DXFCXX.Sub.onEvents.fromGraalList.Event(ns)", perEvent);

                metricsManager->set(std::format("DXFCXX.Sub.{}.onEvents.fromGraalList.Batch(ns)", id.getValue()), elapsed);
                metricsManager->set(std::format("DXFCXX.Sub.{}.onEvents.fromGraalList.Event(ns)", id.getValue()), perEvent);

#endif

#if defined(DXFCXX_ENABLE_METRICS)
                sw.restart();
#endif
                sub->onEvent_(events);
#if defined(DXFCXX_ENABLE_METRICS)
                sw.stop();

                const auto elapsed2 = sw.elapsedInNanos().count();

                metricsManager->set("DXFCXX.Sub.onEvents.handler.Batch(ns)", elapsed2);
                metricsManager->set(std::format("DXFCXX.Sub.{}.onEvents.handler.Batch(ns)", id.getValue()), elapsed2);
                metricsManager->set("DXFCXX.Sub.onEvents.total.Batch(ns)", elapsed + elapsed2);
                metricsManager->set(std::format("DXFCXX.Sub.{}.onEvents.total.Batch(ns)", id.getValue()),
                                    elapsed + elapsed2);
#endif
            }
        });
    }
};

JavaObjectHandle<DXFeedSubscription>
DXFeedSubscription::createSubscriptionHandleFromEventClassList(const std::unique_ptr<EventClassList> &list) {
    return isolated::api::IsolatedDXFeedSubscription::create(list);
}

void DXFeedSubscription::setEventListenerHandle(Id<DXFeedSubscription> id) {
    eventListenerHandle_ = isolated::api::IsolatedDXFeedSubscription::DXFeedEventListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onEvents), dxfcpp::bit_cast<void *>(id.getValue()));

    isolated::api::IsolatedDXFeedSubscription::addEventListener(handle_, eventListenerHandle_);
}

bool DXFeedSubscription::tryToSetEventListenerHandle() {
    std::lock_guard lock{eventListenerMutex_};

    // close() sets the flag before it takes the native listener under this mutex.
    if (closed_) {
        return false;
    }

    if (!eventListenerHandle_) {
        const auto idOpt = Impl::getManager()->getId(sharedAs<DXFeedSubscription>());

        if (!idOpt) {
            return false;
        }

        setEventListenerHandle(idOpt.value());
    }

    return true;
}

void DXFeedSubscription::setSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::setSymbols(handle_, graalSymbolList);
}

void DXFeedSubscription::addSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::addSymbols(handle_, graalSymbolList);
}

void DXFeedSubscription::removeSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::removeSymbols(handle_, graalSymbolList);
}

DXFeedSubscription::DXFeedSubscription() : impl_(std::make_unique<Impl>()) {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeedSubscription", 1);
#endif
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType) : DXFeedSubscription() {
    eventTypes_ = std::unordered_set{eventType};
    handle_ = isolated::api::IsolatedDXFeedSubscription::create(eventType);
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType, JavaObjectHandle<DXFeedSubscription> &&handle)
    : DXFeedSubscription() {
    eventTypes_ = std::unordered_set{eventType};
    handle_ = std::move(handle);
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag) : DXFeedSubscription() {
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag, const EventTypeEnum &eventType)
    : DXFeedSubscription(eventType) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription(eventType = " + eventType.getName() + ")");
    }
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag tag, std::initializer_list<EventTypeEnum> eventTypes)
    : DXFeedSubscription(tag, eventTypes.begin(), eventTypes.end()) {
}

std::string DXFeedSubscription::toString() const {
    return fmt::format("DXFeedSubscription{{{}}}", handle_.toString());
}

DXFeedSubscription::~DXFeedSubscription() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription{" + handle_.toString() + "}::~DXFeedSubscription()");
    }

    if (handle_) {
        runRelease("DXFeedSubscription", [&] {
            close();
        });
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeedSubscription", -1);
#endif
}

std::shared_ptr<DXFeedSubscription> DXFeedSubscription::create(const EventTypeEnum &eventType) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription::create(eventType = " + eventType.getName() + ")");
    }

    auto sub = createShared(eventType);

    registerSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedSubscription> DXFeedSubscription::create(std::initializer_list<EventTypeEnum> eventTypes) {
    auto sub = createShared(eventTypes);

    registerSubscription(sub);

    return sub;
}

void DXFeedSubscription::registerSubscription(const std::shared_ptr<DXFeedSubscription> &subscription) {
    // The registry does not own the subscription: its owners are the user and the feeds it is attached to.
    subscription->id_ = Impl::getManager()->registerEntity(subscription).getValue();
}

void DXFeedSubscription::addFeed(const std::shared_ptr<const DXFeed> &feed) const {
    std::lock_guard guard{feedsMutex_};

    // Attached to the same feed again.
    if (std::ranges::any_of(feeds_, [&feed](const auto &weakFeed) {
            return weakFeed.lock() == feed;
        })) {
        return;
    }

    feeds_.push_back(feed);
}

void DXFeedSubscription::removeFeed(const DXFeed *feed) const {
    std::lock_guard guard{feedsMutex_};

    std::erase_if(feeds_, [feed](const auto &weakFeed) {
        const auto locked = weakFeed.lock();

        return !locked || locked.get() == feed;
    });
}

void DXFeedSubscription::attach(std::shared_ptr<DXFeed> feed) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::attach(feed = " + feed->toString() + ")");
    }

    feed->attachSubscription(sharedAs<DXFeedSubscription>());
}

void DXFeedSubscription::detach(std::shared_ptr<DXFeed> feed) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::detach(feed = " + feed->toString() + ")");
    }

    feed->detachSubscription(sharedAs<DXFeedSubscription>());
}

bool DXFeedSubscription::isClosed() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::isClosed()");
    }

    return closed_ || isolated::api::IsolatedDXFeedSubscription::isClosed(handle_);
}

void DXFeedSubscription::close() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::close()");
    }

    // The batches that come from now on are dropped.
    if (closed_.exchange(true)) {
        return;
    }

    // Keeps the subscription alive until the end: the feeds may hold its last pointers. Empty in the destructor.
    const auto self = sharedAs<const DXFeedSubscription>();
    std::unordered_map<std::size_t, std::shared_ptr<ObservableSubscriptionChangeListener>> changeListeners{};
    // The listeners and the owners are released even if the Java close fails; its error is thrown at the end.
    std::exception_ptr error{};

    {
        // A change listener that the Java close notifies may add or remove change listeners: the mutex is recursive.
        // It is not held below, where close() waits for the event listeners, which may take it.
        std::lock_guard guard{changeListenersMutex_};

        try {
            isolated::api::IsolatedDXFeedSubscription::close(handle_);
        } catch (...) {
            error = std::current_exception();
        }

        changeListeners = std::exchange(changeListeners_, {});
    }

    onEvent_.removeAll();

    JavaObjectHandle<DXFeedEventListener> eventListenerHandle{};

    {
        std::lock_guard guard{eventListenerMutex_};

        eventListenerHandle = std::move(eventListenerHandle_);
    }

    // Releasing the native listener waits for its calls in progress on the other threads, which cannot reach the
    // listeners anymore.
    if (eventListenerHandle) {
        releaseOutsideCallbacks(
            std::make_shared<JavaObjectHandle<DXFeedEventListener>>(std::move(eventListenerHandle)));
    }

    std::vector<std::weak_ptr<const DXFeed>> feeds{};

    {
        std::lock_guard guard{feedsMutex_};

        feeds = std::exchange(feeds_, {});
    }

    for (const auto &weakFeed : feeds) {
        if (const auto feed = weakFeed.lock()) {
            feed->releaseSubscription(this);
        }
    }

    // By id: in the destructor there is no pointer to the subscription.
    ignoreUnused(Impl::getManager()->unregisterEntity(Id<DXFeedSubscription>::from(id_)));

    if (error) {
        std::rethrow_exception(error);
    }
}

std::unordered_set<EventTypeEnum> DXFeedSubscription::getEventTypes() {
    return eventTypes_;
}

bool DXFeedSubscription::containsEventType(const EventTypeEnum &eventType) {
    return eventTypes_.contains(eventType);
}

void DXFeedSubscription::clear() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::clear()");
    }

    isolated::api::IsolatedDXFeedSubscription::clear(handle_);
}

std::vector<SymbolWrapper> DXFeedSubscription::getSymbols() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::getSymbols()");
    }

    return isolated::api::IsolatedDXFeedSubscription::getSymbols(handle_);
}

std::vector<SymbolWrapper> DXFeedSubscription::getDecoratedSymbols() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::getDecoratedSymbols()");
    }

    return isolated::api::IsolatedDXFeedSubscription::getDecoratedSymbols(handle_);
}

void DXFeedSubscription::setSymbols(std::initializer_list<SymbolWrapper> collection) const {
    setSymbols(collection.begin(), collection.end());
}

void DXFeedSubscription::addSymbols(const SymbolWrapper &symbolWrapper) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::addSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
    }

    const auto graal = symbolWrapper.toGraalUnique();

    isolated::api::IsolatedDXFeedSubscription::addSymbol(handle_, graal.get());
}

void DXFeedSubscription::addSymbols(std::initializer_list<SymbolWrapper> collection) const {
    addSymbols(collection.begin(), collection.end());
}

void DXFeedSubscription::removeSymbols(const SymbolWrapper &symbolWrapper) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::removeSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
    }

    const auto graal = symbolWrapper.toGraalUnique();

    isolated::api::IsolatedDXFeedSubscription::removeSymbol(handle_, graal.get());
}

void DXFeedSubscription::removeSymbols(std::initializer_list<SymbolWrapper> collection) const {
    removeSymbols(collection.begin(), collection.end());
}

TimePeriod DXFeedSubscription::getAggregationPeriod() const {
    return TimePeriod(isolated::api::IsolatedDXFeedSubscription::getAggregationPeriod(handle_));
}

void DXFeedSubscription::setAggregationPeriod(const TimePeriod &aggregationPeriod) const {
    isolated::api::IsolatedDXFeedSubscription::setAggregationPeriod(handle_, aggregationPeriod.getHandle());
}

void DXFeedSubscription::setAggregationPeriod(std::chrono::milliseconds aggregationPeriod) const {
    return setAggregationPeriod(TimePeriod::valueOf(aggregationPeriod));
}

void DXFeedSubscription::setAggregationPeriod(std::int64_t aggregationPeriod) const {
    return setAggregationPeriod(TimePeriod::valueOf(aggregationPeriod));
}

TimePeriodInfo DXFeedSubscription::getAggregationPeriodInfo() const {
    const auto [min, max] = isolated::api::IsolatedDXFeedSubscription::getAggregationPeriodInfo(handle_);

    return min < 0 || max < 0 ? TimePeriodInfo::UNKNOWN : TimePeriodInfo::valueOf(min, max);
}

std::int64_t DXFeedSubscription::getTotalDroppedEvents() const {
    return isolated::api::IsolatedDXFeedSubscription::getTotalDroppedEvents(handle_);
}

void DXFeedSubscription::removeEventListener(std::size_t listenerId) {
    onEvent_ -= listenerId;
}

DXFeedSubscription::OnEventHandler &DXFeedSubscription::onEvent() {
    tryToSetEventListenerHandle();

    return onEvent_;
}

std::size_t DXFeedSubscription::addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) {
    std::lock_guard guard{changeListenersMutex_};

    if (isClosed()) {
        return FAKE_CHANGE_LISTENER_ID;
    }

    isolated::api::IsolatedDXFeedSubscription::addChangeListener(
        handle_, listener->getHandle(ObservableSubscriptionChangeListener::Key{}));

    if (isClosed()) {
        return FAKE_CHANGE_LISTENER_ID;
    }

    if (lastChangeListenerId_ >= FAKE_CHANGE_LISTENER_ID - 1) {
        return FAKE_CHANGE_LISTENER_ID;
    }

    auto id = ++lastChangeListenerId_;

    changeListeners_.emplace(id, listener);

    return id;
}

void DXFeedSubscription::removeChangeListener(std::size_t changeListenerId) {
    std::lock_guard guard{changeListenersMutex_};

    if (changeListenerId == FAKE_CHANGE_LISTENER_ID) {
        return;
    }

    if (auto found = changeListeners_.find(changeListenerId); found != changeListeners_.end()) {
        auto listener = found->second;

        isolated::api::IsolatedDXFeedSubscription::removeChangeListener(
            handle_, listener->getHandle(ObservableSubscriptionChangeListener::Key{}));

        changeListeners_.erase(changeListenerId);
    }
}

std::int32_t DXFeedSubscription::getEventsBatchLimit() const {
    return isolated::api::IsolatedDXFeedSubscription::getEventsBatchLimit(handle_);
}

void DXFeedSubscription::setEventsBatchLimit(std::int32_t eventsBatchLimit) const {
    isolated::api::IsolatedDXFeedSubscription::setEventsBatchLimit(handle_, eventsBatchLimit);
}

DXFeedTimeSeriesSubscription::DXFeedTimeSeriesSubscription(
    RequireMakeShared<DXFeedTimeSeriesSubscription>::LockExternalConstructionTag) {

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

DXFeedTimeSeriesSubscription::DXFeedTimeSeriesSubscription(
    RequireMakeShared<DXFeedTimeSeriesSubscription>::LockExternalConstructionTag, const EventTypeEnum &eventType,
    JavaObjectHandle<DXFeedSubscription> &&handle)
    : DXFeedSubscription(eventType, std::move(handle)) {

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

std::string DXFeedTimeSeriesSubscription::toString() const {
    return fmt::format("DXFeedTimeSeriesSubscription{{{}}}", handle_.toString());
}

std::int64_t DXFeedTimeSeriesSubscription::getFromTime() {
    return fromTime_;
}

void DXFeedTimeSeriesSubscription::setFromTime(std::int64_t fromTime) {
    if (fromTime != fromTime_) {
        fromTime_ = fromTime;
        isolated::api::IsolatedDXFeedTimeSeriesSubscription::setFromTime(handle_, fromTime);
    }
}

void DXFeedTimeSeriesSubscription::setFromTime(std::chrono::milliseconds fromTime) {
    setFromTime(fromTime.count());
}

DXFeedTimeSeriesSubscription::~DXFeedTimeSeriesSubscription() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", -1);
#endif
}

void DXFeedTimeSeriesSubscription::registerEntity() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

DXFCPP_END_NAMESPACE
