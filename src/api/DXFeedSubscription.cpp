// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/DXFeedSubscription.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "../../include/dxfeed_graal_cpp_api/api/osub/ObservableSubscriptionChangeListener.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/Quote.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/TimeAndSale.hpp"
#include "../../include/dxfeed_graal_cpp_api/event/market/TradeBase.hpp"
#include "../../include/dxfeed_graal_cpp_api/exceptions/ErrorManager.hpp"
#include "../../include/dxfeed_graal_cpp_api/system/System.hpp"
#include "../internal/CallbackBarrier.hpp"
#include "../internal/Id.hpp"
#include "../internal/context/ApiContext.hpp"
#include "../internal/managers/EntityManager.hpp"
#include "../isolated/api/IsolatedDXFeedSubscription.hpp"
#if defined(DXFCXX_ENABLE_METRICS)
#    include "../internal/Metrics.hpp"
#endif
#include "../internal/StopWatch.hpp"
#include "../internal/utils/debug/Debug.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <dxfg_v4_api.h>
#include <exception>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

namespace {

constexpr auto LISTENER_CHECKS_COMPONENT = "DXFeedSubscription";

// An invalid value of a property is reported once per process. Here, not in DXFeedSubscription::Impl: the static
// members of a nested class of an exported class would be exported.
std::atomic<bool> invalidSlowListenerThresholdReported{false};
std::atomic<bool> invalidEventLagThresholdReported{false};
std::atomic<bool> invalidWarningPeriodReported{false};

// A time period of a property in milliseconds, or the default when the property is not set. A value that is not a time
// period is reported as a warning once per process for each property.
std::int64_t readTimePeriodProperty(const char *name, const char *defaultValue, std::int64_t defaultMillis,
                                    std::atomic<bool> &invalidValueReported) noexcept {
    std::string value{};

    try {
        value = System::getProperty(name);
    } catch (...) {
        return defaultMillis;
    }

    if (value.empty()) {
        return defaultMillis;
    }

    try {
        return TimePeriod::valueOf(value).getTime();
    } catch (...) {
        if (!invalidValueReported.exchange(true)) {
            try {
                ErrorManager::reportWarning(LISTENER_CHECKS_COMPONENT,
                                            fmt::format("The value '{}' of the system property {} is not a time "
                                                        "period (for example 0.1s, 5s or 1m): the default {} is used",
                                                        value, name, defaultValue));
            } catch (...) {
                // No memory for the message: the default is used anyway.
            }
        }

        return defaultMillis;
    }
}

// The warnings of one check of one subscription: the first excess is reported at once, the next ones at most once per
// period, with the number of the excesses since the previous warning and the largest of them. Called on the threads of
// the events; the mutex is taken only on an excess.
class WarningLimiter {
    std::mutex mutex_{};
    std::optional<std::chrono::steady_clock::time_point> lastWarning_{};
    std::int64_t suppressedCount_{};
    std::int64_t suppressedMax_{};

  public:
    struct Suppressed {
        std::int64_t count{};
        std::int64_t max{};
    };

    // Returns the excesses suppressed since the previous warning if a warning is to be reported now.
    std::optional<Suppressed> onExcess(std::int64_t value, std::int64_t periodMillis) {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard lock{mutex_};

        // In milliseconds: the period may be TimePeriod::UNLIMITED, its nanoseconds do not fit into 64 bits.
        if (lastWarning_ &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - *lastWarning_).count() < periodMillis) {
            suppressedCount_++;
            suppressedMax_ = (std::max)(suppressedMax_, value);

            return std::nullopt;
        }

        const Suppressed suppressed{suppressedCount_, suppressedMax_};

        lastWarning_ = now;
        suppressedCount_ = 0;
        suppressedMax_ = 0;

        return suppressed;
    }
};

std::string suppressedToString(const WarningLimiter::Suppressed &suppressed, const char *what) {
    if (suppressed.count == 0) {
        return {};
    }

    return fmt::format(" Since the previous warning: {} more {}, the largest {} ms.", suppressed.count, what,
                       suppressed.max);
}

// The time of a market action of an event in milliseconds since the epoch, or 0 when the event has none.
std::int64_t getMarketTime(const EventType &event) noexcept {
    if (const auto quote = dynamic_cast<const Quote *>(&event)) {
        return quote->getTime();
    }

    if (const auto trade = dynamic_cast<const TradeBase *>(&event)) {
        return trade->getTime();
    }

    if (const auto timeAndSale = dynamic_cast<const TimeAndSale *>(&event)) {
        return timeAndSale->getTime();
    }

    return 0;
}

std::int64_t steadyNowNanos() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// The subscriptions with the silence watchdog on (DXFeedSubscription::setSilenceWatchdog()) and the thread that checks
// them. One thread for all the subscriptions, started on the first use and never stopped.
struct SilenceWatchdog {
    struct Entry {
        std::weak_ptr<DXFeedSubscription> subscription{};
        std::chrono::milliseconds timeout{};
        // Each setSilenceWatchdog() creates a new entry: the results of a check of the replaced one are dropped.
        std::uint64_t generation{};
        std::chrono::steady_clock::time_point deadline{};
        // The start of the silence that has been reported, in nanoseconds of steady_clock (only the thread changes it).
        std::int64_t reportedSilenceStart{};
    };

    std::mutex mutex{};
    std::condition_variable changed{};
    std::unordered_map<std::size_t, Entry> entries{}; // by the id of the subscription
    std::uint64_t lastGeneration{};
    bool started{false};

    // Never destroyed: the detached thread uses it until the end of the process.
    static SilenceWatchdog &getInstance() {
        static auto &instance = *new SilenceWatchdog{};

        return instance;
    }

    // Removes the entry of a subscription. The weak pointer is released under the mutex: no destructor of a subscription
    // runs here.
    void remove(std::size_t id) {
        std::lock_guard lock{mutex};

        if (entries.erase(id) > 0) {
            changed.notify_all();
        }
    }
};

} // namespace

struct DXFeedSubscription::Impl {
    static constexpr auto DEFAULT_SLOW_LISTENER_THRESHOLD = "0.1s";
    static constexpr std::int64_t DEFAULT_SLOW_LISTENER_THRESHOLD_MILLIS = 100;
    static constexpr auto DEFAULT_EVENT_LAG_THRESHOLD = "0";
    static constexpr std::int64_t DEFAULT_EVENT_LAG_THRESHOLD_MILLIS = 0;
    static constexpr auto DEFAULT_WARNING_PERIOD = "1m";
    static constexpr std::int64_t DEFAULT_WARNING_PERIOD_MILLIS = 60'000;

    // The checks of the listeners (the page listener_threads in DXFeedSubscription.hpp), read once when the
    // subscription is created. A threshold of 0 or less turns its check off.
    const std::int64_t slowListenerThresholdMillis_;
    const std::int64_t eventLagThresholdMillis_;
    const std::int64_t warningPeriodMillis_;

    WarningLimiter slowListenerWarnings_{};
    WarningLimiter eventLagWarnings_{};

    // The silence watchdog (setSilenceWatchdog()); the times are nanoseconds of steady_clock. While it is on, the thread
    // of the events only stores the time of each batch.
    std::atomic<bool> silenceWatchdogOn_{false};
    std::atomic<std::int64_t> lastBatchNanos_{0};
    // The time of setSilenceWatchdog() or of the last attach to a feed.
    std::atomic<std::int64_t> silenceArmedNanos_{0};
    // A pointer: the watchdog takes the function under the mutex without a call of the code of the user.
    std::mutex silenceMutex_{};
    std::shared_ptr<const OnSilenceHandler> onSilence_{};
    // Under the mutex: the generation of the entry of the watchdog, 0 while it is off. A check that waited behind
    // another onSilence reports nothing after the watchdog was turned off or replaced.
    std::uint64_t silenceGeneration_{};

    Impl()
        : slowListenerThresholdMillis_(readTimePeriodProperty(
              SLOW_LISTENER_THRESHOLD_PROPERTY, DEFAULT_SLOW_LISTENER_THRESHOLD, DEFAULT_SLOW_LISTENER_THRESHOLD_MILLIS,
              invalidSlowListenerThresholdReported)),
          eventLagThresholdMillis_(readTimePeriodProperty(EVENT_LAG_THRESHOLD_PROPERTY, DEFAULT_EVENT_LAG_THRESHOLD,
                                                          DEFAULT_EVENT_LAG_THRESHOLD_MILLIS,
                                                          invalidEventLagThresholdReported)),
          warningPeriodMillis_(readTimePeriodProperty(WARNING_PERIOD_PROPERTY, DEFAULT_WARNING_PERIOD,
                                                      DEFAULT_WARNING_PERIOD_MILLIS, invalidWarningPeriodReported)) {
    }

    static std::string getName(const DXFeedSubscription &sub) {
        std::vector<std::string> eventTypeNames{};

        eventTypeNames.reserve(sub.eventTypes_.size());

        for (const auto &eventType : sub.eventTypes_) {
            eventTypeNames.push_back(eventType.getClassName());
        }

        std::ranges::sort(eventTypeNames);

        return fmt::format("DXFeedSubscription #{} [{}]", sub.id_, fmt::join(eventTypeNames, ", "));
    }

    // The age of the newest event with a market time when the batch is received.
    void checkEventLag(const DXFeedSubscription &sub, const std::vector<std::shared_ptr<EventType>> &events) {
        // Its history comes with past times.
        if (dynamic_cast<const DXFeedTimeSeriesSubscription *>(&sub) != nullptr) {
            return;
        }

        std::int64_t newestTime = 0;

        for (const auto &event : events) {
            if (event) {
                newestTime = (std::max)(newestTime, getMarketTime(*event));
            }
        }

        // No event with a time.
        if (newestTime <= 0) {
            return;
        }

        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();
        const auto lag = now - newestTime;

        if (lag <= eventLagThresholdMillis_) {
            return;
        }

        if (const auto suppressed = eventLagWarnings_.onExcess(lag, warningPeriodMillis_)) {
            ErrorManager::reportWarning(
                LISTENER_CHECKS_COMPONENT,
                fmt::format("{}: the newest event of a batch of {} events is {} ms old (the threshold is {} ms, the "
                            "system property {}).{} The events come late: a listener that blocks, a slow network or "
                            "a difference between the clocks of this computer and of the source.",
                            getName(sub), events.size(), lag, eventLagThresholdMillis_, EVENT_LAG_THRESHOLD_PROPERTY,
                            suppressedToString(*suppressed, "late batches")));
        }
    }

    void checkSlowListeners(const DXFeedSubscription &sub, std::chrono::steady_clock::duration elapsed,
                            std::size_t batchSize) {
        const auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

        if (elapsedMillis <= slowListenerThresholdMillis_) {
            return;
        }

        if (const auto suppressed = slowListenerWarnings_.onExcess(elapsedMillis, warningPeriodMillis_)) {
            ErrorManager::reportWarning(
                LISTENER_CHECKS_COMPONENT,
                fmt::format("{}: the listeners took {} ms for a batch of {} events (the threshold is {} ms, the "
                            "system property {}).{} A listener delays all the events of its subscription: move long "
                            "work to another thread.",
                            getName(sub), elapsedMillis, batchSize, slowListenerThresholdMillis_,
                            SLOW_LISTENER_THRESHOLD_PROPERTY, suppressedToString(*suppressed, "slow batches")));
        }
    }

    static auto getManager() {
        return ApiContext::getInstance().getManager<WeakEntityManager<DXFeedSubscription>>();
    }

    // The result of a check of one subscription by the silence watchdog.
    struct SilenceCheck {
        std::size_t id{};
        std::uint64_t generation{};
        bool remove{};
        std::chrono::steady_clock::time_point deadline{};
        std::int64_t reportedSilenceStart{};
    };

    // The earliest connection among the connected endpoints of the feeds of a subscription.
    static std::optional<std::chrono::steady_clock::time_point> getConnectedSince(const DXFeedSubscription &sub) {
        std::vector<std::weak_ptr<const DXFeed>> feeds{};

        {
            std::lock_guard guard{sub.feedsMutex_};

            feeds = sub.feeds_;
        }

        std::optional<std::chrono::steady_clock::time_point> result{};

        for (const auto &weakFeed : feeds) {
            const auto feed = weakFeed.lock();

            if (!feed) {
                continue;
            }

            const auto endpoint = feed->endpoint_.lock();

            if (!endpoint) {
                continue;
            }

            if (const auto since = endpoint->getConnectedSince(); since && (!result || *since < *result)) {
                result = since;
            }
        }

        return result;
    }

    // Checks one subscription: reports its silence and calls onSilence once the silence lasts for the timeout.
    static void checkSilence(const std::shared_ptr<DXFeedSubscription> &sub, std::chrono::milliseconds timeout,
                             SilenceCheck &check) {
        auto &impl = *sub->impl_;
        const auto now = std::chrono::steady_clock::now();
        const auto connectedSince = getConnectedSince(*sub);

        // Not connected: checked again later for a connection.
        if (!connectedSince) {
            check.deadline = now + timeout;

            return;
        }

        const auto silenceStart = (std::max)(
            {impl.lastBatchNanos_.load(), impl.silenceArmedNanos_.load(),
             static_cast<std::int64_t>(
                 std::chrono::duration_cast<std::chrono::nanoseconds>(connectedSince->time_since_epoch()).count())});
        const auto silence = std::chrono::nanoseconds{
            std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count() - silenceStart};

        if (silence < timeout) {
            check.deadline = now + (timeout - silence);

            return;
        }

        // Checked again later for a new batch or a new connection.
        check.deadline = now + timeout;

        if (check.reportedSilenceStart == silenceStart) {
            return;
        }

        check.reportedSilenceStart = silenceStart;

        std::shared_ptr<const OnSilenceHandler> onSilence{};

        {
            std::lock_guard lock{impl.silenceMutex_};

            // Turned off or replaced after the check began.
            if (impl.silenceGeneration_ != check.generation) {
                return;
            }

            onSilence = impl.onSilence_;
        }

        if (sub->closed_) {
            return;
        }

        const auto silenceMillis = std::chrono::duration_cast<std::chrono::milliseconds>(silence);

        try {
            ErrorManager::reportWarning(
                LISTENER_CHECKS_COMPONENT,
                fmt::format("{}: no events for {} ms while the endpoint is connected (the silence watchdog, the timeout "
                            "is {} ms). The data may have stopped before it reached this endpoint; a symbol that does "
                            "not trade, a quiet market or a subscription without symbols is silent too.",
                            getName(*sub), silenceMillis.count(), timeout.count()));
        } catch (...) {
            // No memory for the message: onSilence is called anyway.
        }

        if (onSilence && *onSilence) {
            try {
                (*onSilence)(sub, silenceMillis);
            } catch (...) {
                ErrorManager::reportException(LISTENER_CHECKS_COMPONENT, std::current_exception());
            }
        }
    }

    // Checks the subscriptions whose deadlines have come.
    static void checkSilences(SilenceWatchdog &watchdog, std::vector<std::pair<std::size_t, SilenceWatchdog::Entry>> &due,
                              std::vector<SilenceCheck> &checks) {
        // Released after the mutex below. The last pointer to a subscription closes it here, in the callback scope: the
        // close passes the native listener to the release thread (releaseOutsideCallbacks()).
        std::vector<std::shared_ptr<DXFeedSubscription>> subs{};

        subs.reserve(due.size());
        checks.reserve(due.size());

        for (const auto &[id, entry] : due) {
            SilenceCheck check{id, entry.generation, false, entry.deadline, entry.reportedSilenceStart};
            auto sub = entry.subscription.lock();

            if (!sub || sub->closed_) {
                check.remove = true;
            } else {
                try {
                    checkSilence(sub, entry.timeout, check);
                } catch (...) {
                    ErrorManager::reportException(LISTENER_CHECKS_COMPONENT, std::current_exception());
                }

                subs.push_back(std::move(sub));
            }

            checks.push_back(check);
        }

        std::lock_guard lock{watchdog.mutex};

        for (const auto &check : checks) {
            const auto found = watchdog.entries.find(check.id);

            // Turned off or replaced during the check.
            if (found == watchdog.entries.end() || found->second.generation != check.generation) {
                continue;
            }

            if (check.remove) {
                watchdog.entries.erase(found);
            } else {
                found->second.deadline = check.deadline;
                found->second.reportedSilenceStart = check.reportedSilenceStart;
            }
        }
    }

    // The thread of the silence watchdog: sleeps until the nearest deadline or a change of the entries.
    static void runSilenceWatchdog() noexcept {
        auto &watchdog = SilenceWatchdog::getInstance();
        std::vector<std::pair<std::size_t, SilenceWatchdog::Entry>> due{};
        std::vector<SilenceCheck> checks{};

        for (;;) {
            try {
                due.clear();
                checks.clear();

                {
                    std::unique_lock lock{watchdog.mutex};

                    for (;;) {
                        const auto now = std::chrono::steady_clock::now();
                        std::optional<std::chrono::steady_clock::time_point> nearest{};

                        for (const auto &[id, entry] : watchdog.entries) {
                            if (!nearest || entry.deadline < *nearest) {
                                nearest = entry.deadline;
                            }
                        }

                        if (nearest && *nearest <= now) {
                            for (auto &[id, entry] : watchdog.entries) {
                                if (entry.deadline <= now) {
                                    // Until the check sets the next deadline: a skipped or failed check (the shutdown
                                    // of the library, no memory) does not make a busy loop.
                                    entry.deadline = now + entry.timeout;
                                    due.emplace_back(id, entry);
                                }
                            }

                            break;
                        }

                        if (nearest) {
                            watchdog.changed.wait_until(lock, *nearest);
                        } else {
                            watchdog.changed.wait(lock);
                        }
                    }
                }

                // Outside the mutex of the watchdog: onSilence may turn the watchdog off or close the subscription. The
                // shutdown of the library waits for the check in progress and skips the next ones.
                runCallback(LISTENER_CHECKS_COMPONENT, [&] {
                    checkSilences(watchdog, due, checks);
                });
            } catch (...) {
                ErrorManager::reportException(LISTENER_CHECKS_COMPONENT, std::current_exception());

                // No memory: waits before the next attempt.
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    // Adds or replaces the entry of a subscription and starts the thread on the first use. Returns the generation of the
    // entry.
    static std::uint64_t watchSilence(std::size_t id, std::weak_ptr<DXFeedSubscription> sub, std::chrono::milliseconds timeout,
                             std::chrono::steady_clock::time_point now) {
        auto &watchdog = SilenceWatchdog::getInstance();
        std::lock_guard lock{watchdog.mutex};

        if (!watchdog.started) {
            std::thread{runSilenceWatchdog}.detach();
            watchdog.started = true;
        }

        const auto generation = ++watchdog.lastGeneration;

        watchdog.entries.insert_or_assign(
            id, SilenceWatchdog::Entry{std::move(sub), timeout, generation, now + timeout, 0});
        watchdog.changed.notify_all();

        return generation;
    }

    static void onEvents(graal_isolatethread_t * /*thread*/, dxfg_v4_event_type_list *graalNativeEvents,
                         void *userData) {
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

                auto &impl = *sub->impl_;

                // Before the skipped events: a batch of them is live data too.
                if (impl.silenceWatchdogOn_.load(std::memory_order_relaxed)) {
                    impl.lastBatchNanos_.store(steadyNowNanos(), std::memory_order_relaxed);
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
                const auto metricsManager = ApiContext::getInstance().getManager<MetricsManager>();

                metricsManager->set("DXFCXX.Sub.onEvents.fromGraalList.Batch(ns)", elapsed);
                metricsManager->set("DXFCXX.Sub.onEvents.fromGraalList.Event(ns)", perEvent);

                metricsManager->set(std::format("DXFCXX.Sub.{}.onEvents.fromGraalList.Batch(ns)", id.getValue()), elapsed);
                metricsManager->set(std::format("DXFCXX.Sub.{}.onEvents.fromGraalList.Event(ns)", id.getValue()), perEvent);

#endif

                // On receipt, before the listeners. A failed check must not stop the delivery of the batch.
                if (impl.eventLagThresholdMillis_ > 0) {
                    try {
                        impl.checkEventLag(*sub, events);
                    } catch (...) {
                    }
                }

                const auto checkSlowListeners = impl.slowListenerThresholdMillis_ > 0;
                const auto listenersStart =
                    checkSlowListeners ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};

#if defined(DXFCXX_ENABLE_METRICS)
                sw.restart();
#endif
                sub->onEvent_(events);
#if defined(DXFCXX_ENABLE_METRICS)
                sw.stop();
#endif

                if (checkSlowListeners) {
                    try {
                        impl.checkSlowListeners(*sub, std::chrono::steady_clock::now() - listenersStart,
                                                events.size());
                    } catch (...) {
                    }
                }

#if defined(DXFCXX_ENABLE_METRICS)

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

void DXFeedSubscription::setEventListenerHandle(std::size_t id) {
    auto eventListenerHandle = isolated::api::IsolatedDXFeedSubscription::DXFeedEventListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onEvents), dxfcpp::bit_cast<void *>(id));

    isolated::api::IsolatedDXFeedSubscription::addEventListener(handle_, eventListenerHandle);
    // Only when added: after an exception (IllegalStateException of a subscription with symbols), the next call tries
    // again instead of taking the native listener for added.
    eventListenerHandle_ = std::move(eventListenerHandle);
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

        setEventListenerHandle(idOpt.value().getValue());
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
    ApiContext::getInstance().getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeedSubscription", 1);
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
    ApiContext::getInstance().getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeedSubscription", -1);
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

    // The silence is counted from the attach.
    impl_->silenceArmedNanos_.store(steadyNowNanos());
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
    // The silence watchdog stops without waiting for a call of onSilence in progress; the function is released at the
    // end, outside the mutex.
    std::shared_ptr<const OnSilenceHandler> onSilence{};

    {
        std::lock_guard lock{impl_->silenceMutex_};

        if (impl_->silenceWatchdogOn_.exchange(false)) {
            SilenceWatchdog::getInstance().remove(id_);
        }

        impl_->silenceGeneration_ = 0;
        onSilence = std::move(impl_->onSilence_);
    }

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

void DXFeedSubscription::setSilenceWatchdog(std::chrono::milliseconds timeout, OnSilenceHandler onSilence) {
    // The deadlines in nanoseconds of steady_clock must not overflow: up to 100 years.
    constexpr auto MAX_TIMEOUT = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours{24 * 365 * 100});

    timeout = (std::min)(timeout, MAX_TIMEOUT);

    const auto on = timeout.count() > 0;

    // The batches reach the watchdog through the native listener (Impl::onEvents).
    if (on && !tryToSetEventListenerHandle()) {
        return;
    }

    auto handler = on && onSilence ? std::make_shared<const OnSilenceHandler>(std::move(onSilence))
                                   : std::shared_ptr<const OnSilenceHandler>{};
    // Released after the mutex.
    std::shared_ptr<const OnSilenceHandler> previous{};
    std::lock_guard lock{impl_->silenceMutex_};

    if (!on || closed_) {
        if (impl_->silenceWatchdogOn_.exchange(false)) {
            SilenceWatchdog::getInstance().remove(id_);
        }

        impl_->silenceGeneration_ = 0;
        previous = std::move(impl_->onSilence_);

        return;
    }

    const auto now = std::chrono::steady_clock::now();

    // First: nothing changes if the thread cannot be started.
    impl_->silenceGeneration_ = Impl::watchSilence(id_, sharedAs<DXFeedSubscription>(), timeout, now);

    impl_->silenceArmedNanos_.store(std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
    previous = std::exchange(impl_->onSilence_, std::move(handler));
    impl_->silenceWatchdogOn_.store(true);
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
    ApiContext::getInstance().getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

DXFeedTimeSeriesSubscription::DXFeedTimeSeriesSubscription(
    RequireMakeShared<DXFeedTimeSeriesSubscription>::LockExternalConstructionTag, const EventTypeEnum &eventType,
    JavaObjectHandle<DXFeedSubscription> &&handle)
    : DXFeedSubscription(eventType, std::move(handle)) {

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance().getManager<dxfcpp::MetricsManager>()->add(
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
    ApiContext::getInstance().getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", -1);
#endif
}

void DXFeedTimeSeriesSubscription::registerEntity() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance().getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

DXFCPP_END_NAMESPACE
