// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "dxfeed_graal_cpp_api/isolated/Isolated.hpp"
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

namespace dxfcpp {

struct NonOwningInstrumentProfileIterator {
    dxfg_iterable_ip_t *iterable;

    [[nodiscard]] bool hasNext() const noexcept {
        if (!iterable) {
            return false;
        }

        return isolated::ipf::InstrumentProfileIterator::hasNext(iterable);
    }

    [[nodiscard]] std::shared_ptr<InstrumentProfile> next() const noexcept {
        if (!iterable) {
            return {};
        }

        auto graalProfile = isolated::ipf::InstrumentProfileIterator::next(iterable);
        auto result = InstrumentProfile::fromGraal(graalProfile);
        isolated::ipf::InstrumentProfile::release(graalProfile);

        return result;
    };

    [[nodiscard]] std::vector<std::shared_ptr<InstrumentProfile>> collect() const noexcept {
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
    static void onInstrumentProfilesUpdate(graal_isolatethread_t *thread, dxfg_iterable_ip_t *profiles,
                                           void *userData) noexcept {
        auto [collectorId, listenerId] = dxfcpp::unpack(dxfcpp::bit_cast<std::size_t>(userData));

        auto id = Id<InstrumentProfileCollector>::from(collectorId);
        auto collector = ApiContext::getInstance()->getManager<InstrumentProfileCollectorManager>()->getEntity(id);

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

InstrumentProfileCollector::InstrumentProfileCollector() noexcept
    : id_{Id<InstrumentProfileCollector>::UNKNOWN}, handle_{} {
    handle_ = JavaObjectHandle<InstrumentProfileCollector>(dxfcpp::isolated::ipf::InstrumentProfileCollector::create());
}

void InstrumentProfileCollector::addListenerHandle(std::size_t id) noexcept {
    if (!handle_) {
        return;
    }

    listenerHandles_.emplace(
        id, JavaObjectHandle<InstrumentProfileUpdateListener>(isolated::ipf::InstrumentProfileUpdateListener::create(
                dxfcpp::bit_cast<void *>(&InstrumentProfileCollector::Impl::onInstrumentProfilesUpdate),
                dxfcpp::bit_cast<void *>(dxfcpp::pack(id_.getValue(), id)))));

    if (!listenerHandles_[id]) {
        return;
    }

    isolated::ipf::InstrumentProfileCollector::addUpdateListener(handle_.get(), listenerHandles_[id].get());
}

void InstrumentProfileCollector::removeListenerHandle(std::size_t id) noexcept {
    if (!handle_) {
        return;
    }

    if (!listenerHandles_.contains(id) || !listenerHandles_[id]) {
        return;
    }

    isolated::ipf::InstrumentProfileCollector::removeUpdateListener(handle_.get(), listenerHandles_[id].get());
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

InstrumentProfileCollector::Ptr InstrumentProfileCollector::create() noexcept {
    auto collector = std::shared_ptr<InstrumentProfileCollector>(new (std::nothrow) InstrumentProfileCollector());

    if (!collector) {
        // TODO: dummy collector & error handling [EN-8232];

        return collector;
    }

    collector->id_ =
        ApiContext::getInstance()->getManager<InstrumentProfileCollectorManager>()->registerEntity(collector);

    return collector;
}

std::int64_t InstrumentProfileCollector::getLastUpdateTime() const noexcept {
    if (!handle_) {
        return 0;
    }

    return dxfcpp::isolated::ipf::InstrumentProfileCollector::getLastUpdateTime(handle_.get());
}

void InstrumentProfileCollector::updateInstrumentProfile(std::shared_ptr<InstrumentProfile> ip) const noexcept {
    if (!handle_) {
        return;
    }

    auto graal = ip->toGraal();

    if (graal) {
        dxfcpp::isolated::ipf::InstrumentProfileCollector::updateInstrumentProfile(handle_.get(), graal);

        InstrumentProfile::freeGraal(graal);
    }
}

void InstrumentProfileCollector::updateInstrumentProfile(const InstrumentProfile &ip) const noexcept {
    if (!handle_) {
        return;
    }

    auto graal = ip.toGraal();

    if (graal) {
        dxfcpp::isolated::ipf::InstrumentProfileCollector::updateInstrumentProfile(handle_.get(), graal);

        InstrumentProfile::freeGraal(graal);
    }
}

std::shared_ptr<IterableInstrumentProfile> InstrumentProfileCollector::view() const noexcept {
    if (!handle_) {
        return {};
    }

    auto iterable = dxfcpp::isolated::ipf::InstrumentProfileCollector::view(handle_.get());

    return IterableInstrumentProfile::create(iterable);
}

} // namespace dxfcpp