// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/ipf/live/InstrumentProfileCollector.hpp"

#include "../../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/context/ApiContext.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/ipf/live/IsolatedInstrumentProfileCollector.hpp"

#include <cstring>
#include <dxfg_api.h>
#include <memory>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct NonOwningInstrumentProfileIterator {
    dxfg_iterable_ip_t *iterable;

    [[nodiscard]] bool hasNext() const {
        if (!iterable) {
            return false;
        }

        return isolated::ipf::live::IsolatedInstrumentProfileIterator::hasNext(iterable);
    }

    [[nodiscard]] std::shared_ptr<InstrumentProfile> next() const {
        if (!iterable) {
            return {};
        }

        const auto graalProfile = isolated::ipf::live::IsolatedInstrumentProfileIterator::next(iterable);
        auto result = InstrumentProfile::create(JavaObjectHandle<InstrumentProfile>(graalProfile));

        return result;
    }

    [[nodiscard]] std::vector<std::shared_ptr<InstrumentProfile>> collect() const {
        if (!iterable) {
            return {};
        }

        std::vector<std::shared_ptr<InstrumentProfile>> result{};

        while (hasNext()) {
            result.emplace_back(next());
        }

        return result;
    }
};

struct InstrumentProfileCollector::Impl {
    static auto getManager() {
        return ApiContext::getInstance()->getManager<WeakEntityManager<InstrumentProfileCollector>>();
    }

    static void onInstrumentProfilesUpdate(graal_isolatethread_t * /* thread */, dxfg_iterable_ip_t *profiles,
                                           void *userData) {
        runCallback("InstrumentProfileCollector", [&] {
            auto [collectorId, listenerId] = unpack(dxfcpp::bit_cast<std::size_t>(userData));

            const auto id = Id<InstrumentProfileCollector>::from(collectorId);
            const auto collector = Impl::getManager()->getEntity(id);

            if constexpr (Debugger::isDebug) {
                Debugger::debug("InstrumentProfileCollector::Impl::onInstrumentProfilesUpdate2: id = " +
                                std::to_string(id.getValue()));
            }

            if (collector) {
                auto &&apiProfiles = NonOwningInstrumentProfileIterator{profiles}.collect();
                std::shared_ptr<UpdateListener> listener{};

                {
                    std::lock_guard guard{collector->listenersMutex_};

                    if (const auto found = collector->updateListeners_.find(listenerId);
                        found != collector->updateListeners_.end()) {
                        listener = found->second.listener;
                    }
                }

                // Without the lock: the listener may add or remove the listeners of this collector.
                if (listener) {
                    listener->handler(apiProfiles);
                }
            }
        });
    }
};

InstrumentProfileCollector::InstrumentProfileCollector() : id_{Id<InstrumentProfileCollector>::UNKNOWN} {
    handle_ = isolated::ipf::live::IsolatedInstrumentProfileCollector::create();
}

std::size_t InstrumentProfileCollector::addUpdateListenerImpl(std::shared_ptr<UpdateListener> listener) {
    std::size_t id{};
    std::size_t userData{};

    // The listener is in the map before the native listener is added: the collector notifies it at once (on its
    // executor) if it has profiles.
    {
        std::lock_guard guard{listenersMutex_};

        id = lastListenerId_ + 1;
        // The user data of the native listener: pack() throws before anything changes if an id does not fit.
        userData = pack(id_.getValue(), id);
        lastListenerId_ = id;
        updateListeners_.emplace(id, UpdateListenerEntry{std::move(listener), {}});
    }

    if (!handle_) {
        return id;
    }

    try {
        auto handle = isolated::ipf::live::IsolatedInstrumentProfileUpdateListener::create(
            dxfcpp::bit_cast<void *>(&Impl::onInstrumentProfilesUpdate), dxfcpp::bit_cast<void *>(userData));

        if (!handle) {
            return id;
        }

        isolated::ipf::live::IsolatedInstrumentProfileCollector::addUpdateListener(handle_, handle);

        {
            std::lock_guard guard{listenersMutex_};

            if (const auto found = updateListeners_.find(id); found != updateListeners_.end()) {
                found->second.handle = std::move(handle);

                return id;
            }
        }

        // The listener was removed while it was being added (by itself, for example).
        isolated::ipf::live::IsolatedInstrumentProfileCollector::removeUpdateListener(handle_, handle);
        releaseListenerHandle(std::move(handle));
    } catch (...) {
        std::lock_guard guard{listenersMutex_};

        updateListeners_.erase(id);

        throw;
    }

    return id;
}

void InstrumentProfileCollector::releaseListenerHandle(JavaObjectHandle<InstrumentProfileUpdateListener> &&handle) {
    // Inside a callback, the native listener may be the one that is calling.
    releaseOutsideCallbacks(std::make_shared<JavaObjectHandle<InstrumentProfileUpdateListener>>(std::move(handle)));
}

InstrumentProfileCollector::~InstrumentProfileCollector() noexcept {
    runRelease("InstrumentProfileCollector", [this] {
        ignoreUnused(Impl::getManager()->unregisterEntity(id_));
    });

    std::unordered_map<std::size_t, UpdateListenerEntry> listeners{};

    {
        std::lock_guard guard{listenersMutex_};

        listeners.swap(updateListeners_);
    }

    for (auto &[id, entry] : listeners) {
        if (!entry.handle) {
            continue;
        }

        if (handle_) {
            runRelease("InstrumentProfileCollector", [this, &entry] {
                isolated::ipf::live::IsolatedInstrumentProfileCollector::removeUpdateListener(handle_, entry.handle);
            });
        }

        // Even if the native listener was not removed.
        runRelease("InstrumentProfileCollector", [&entry] {
            releaseListenerHandle(std::move(entry.handle));
        });
    }
}

InstrumentProfileCollector::Ptr InstrumentProfileCollector::create() {
    auto collector = std::shared_ptr<InstrumentProfileCollector>(new InstrumentProfileCollector());

    collector->id_ = Impl::getManager()->registerEntity(collector);

    return collector;
}

std::int64_t InstrumentProfileCollector::getLastUpdateTime() const {
    if (!handle_) {
        return 0;
    }

    return isolated::ipf::live::IsolatedInstrumentProfileCollector::getLastUpdateTime(handle_);
}

std::chrono::milliseconds InstrumentProfileCollector::getLastUpdateTimeAsDuration() const {
    return std::chrono::milliseconds(getLastUpdateTime());
}

void InstrumentProfileCollector::updateInstrumentProfile(std::shared_ptr<InstrumentProfile> ip) const {
    if (!handle_) {
        return;
    }

    isolated::ipf::live::IsolatedInstrumentProfileCollector::updateInstrumentProfile(handle_, ip->handle_);
}

std::shared_ptr<IterableInstrumentProfile> InstrumentProfileCollector::view() const {
    if (!handle_) {
        return {};
    }

    const auto iterable = isolated::ipf::live::IsolatedInstrumentProfileCollector::view(handle_);

    return IterableInstrumentProfile::create(iterable);
}

void InstrumentProfileCollector::removeUpdateListener(std::size_t listenerId) {
    UpdateListenerEntry entry{};

    {
        std::lock_guard guard{listenersMutex_};

        const auto found = updateListeners_.find(listenerId);

        if (found == updateListeners_.end()) {
            return;
        }

        entry = std::move(found->second);
        updateListeners_.erase(found);
    }

    if (handle_ && entry.handle) {
        try {
            isolated::ipf::live::IsolatedInstrumentProfileCollector::removeUpdateListener(handle_, entry.handle);
        } catch (...) {
            // The native listener is still added: the listener stays, so that the call can be repeated.
            std::lock_guard guard{listenersMutex_};

            updateListeners_.emplace(listenerId, std::move(entry));

            throw;
        }
    }

    // Waits for the call of the listener in progress on another thread (see SimpleHandler::remove()).
    entry.listener->handler.remove(entry.listener->handlerListenerId);

    if (entry.handle) {
        releaseListenerHandle(std::move(entry.handle));
    }
}

DXFCPP_END_NAMESPACE