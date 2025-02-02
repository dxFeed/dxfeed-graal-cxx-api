// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

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

        auto graalProfile = isolated::ipf::live::IsolatedInstrumentProfileIterator::next(iterable);
        auto result = InstrumentProfile::create(JavaObjectHandle<InstrumentProfile>(graalProfile));

        return result;
    };

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
    static void onInstrumentProfilesUpdate(graal_isolatethread_t * /* thread */, dxfg_iterable_ip_t *profiles,
                                           void *userData) {
        auto [collectorId, listenerId] = dxfcpp::unpack(dxfcpp::bit_cast<std::size_t>(userData));

        auto id = Id<InstrumentProfileCollector>::from(collectorId);
        auto collector =
            ApiContext::getInstance()->getManager<EntityManager<InstrumentProfileCollector>>()->getEntity(id);

        if constexpr (Debugger::isDebug) {
            Debugger::debug("InstrumentProfileCollector::Impl::onInstrumentProfilesUpdate2: id = " +
                            std::to_string(id.getValue()));
        }

        if (collector) {
            auto &&apiProfiles = NonOwningInstrumentProfileIterator{profiles}.collect();

            std::lock_guard guard{collector->listenersMutex_};

            if (collector->onInstrumentProfilesUpdateHandlers_.contains(listenerId)) {
                collector->onInstrumentProfilesUpdateHandlers_[listenerId](apiProfiles);
            }
        }
    }
};

InstrumentProfileCollector::InstrumentProfileCollector() : id_{Id<InstrumentProfileCollector>::UNKNOWN} {
    handle_ = dxfcpp::isolated::ipf::live::IsolatedInstrumentProfileCollector::create();
}

void InstrumentProfileCollector::addListenerHandle(std::size_t id) {
    if (!handle_) {
        return;
    }

    listenerHandles_.emplace(
        id, isolated::ipf::live::IsolatedInstrumentProfileUpdateListener::create(
                dxfcpp::bit_cast<void *>(&InstrumentProfileCollector::Impl::onInstrumentProfilesUpdate),
                dxfcpp::bit_cast<void *>(dxfcpp::pack(id_.getValue(), id))));

    if (!listenerHandles_[id]) {
        return;
    }

    isolated::ipf::live::IsolatedInstrumentProfileCollector::addUpdateListener(handle_, listenerHandles_[id]);
}

void InstrumentProfileCollector::removeListenerHandle(std::size_t id) {
    if (!handle_) {
        return;
    }

    if (!listenerHandles_.contains(id) || !listenerHandles_[id]) {
        return;
    }

    isolated::ipf::live::IsolatedInstrumentProfileCollector::removeUpdateListener(handle_, listenerHandles_[id]);
}

InstrumentProfileCollector::~InstrumentProfileCollector() noexcept {
    std::lock_guard guard{listenersMutex_};

    std::vector<std::size_t> listenerIds{};
    listenerIds.reserve(onInstrumentProfilesUpdateHandlers_.size());

    std::transform(onInstrumentProfilesUpdateHandlers_.cbegin(), onInstrumentProfilesUpdateHandlers_.cend(),
                   std::back_inserter(listenerIds), [](const auto &idAndHandler) {
                       return idAndHandler.first;
                   });

    for (auto id : listenerIds) {
        removeUpdateListenerImpl(id);
    }
}

InstrumentProfileCollector::Ptr InstrumentProfileCollector::create() {
    auto collector = std::shared_ptr<InstrumentProfileCollector>(new InstrumentProfileCollector());

    collector->id_ =
        ApiContext::getInstance()->getManager<EntityManager<InstrumentProfileCollector>>()->registerEntity(collector);

    return collector;
}

std::int64_t InstrumentProfileCollector::getLastUpdateTime() const {
    if (!handle_) {
        return 0;
    }

    return dxfcpp::isolated::ipf::live::IsolatedInstrumentProfileCollector::getLastUpdateTime(handle_);
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

    auto iterable = dxfcpp::isolated::ipf::live::IsolatedInstrumentProfileCollector::view(handle_);

    return IterableInstrumentProfile::create(iterable);
}

DXFCPP_END_NAMESPACE