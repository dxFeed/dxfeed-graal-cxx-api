// Copyright (c) 2023 Devexperts LLC.
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

        return InstrumentProfile::fromGraal(isolated::ipf::InstrumentProfileIterator::next(iterable));
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
        auto id =
            Id<InstrumentProfileCollector>::from(dxfcpp::bit_cast<Id<InstrumentProfileCollector>::ValueType>(userData));
        auto collector = ApiContext::getInstance()->getManager<InstrumentProfileCollectorManager>()->getEntity(id);

        if constexpr (Debugger::isDebug) {
            Debugger::debug("InstrumentProfileCollector::Impl::onInstrumentProfilesUpdate: id = " +
                            std::to_string(id.getValue()));
        }

        if (collector) {
            auto &&apiProfiles = NonOwningInstrumentProfileIterator{profiles}.collect();

            collector->onInstrumentProfilesUpdate_(apiProfiles);
        }
    }
};

InstrumentProfileCollector::InstrumentProfileCollector() noexcept
    : id_{Id<InstrumentProfileCollector>::UNKNOWN}, handle_{}, instrumentProfileUpdateListenerHandle_{},
      onInstrumentProfilesUpdate_{1} {
    handle_ = JavaObjectHandle<InstrumentProfileCollector>(dxfcpp::isolated::ipf::InstrumentProfileCollector::create());
}

InstrumentProfileCollector::Ptr InstrumentProfileCollector::create() noexcept {
    auto collector = std::shared_ptr<InstrumentProfileCollector>(new (std::nothrow) InstrumentProfileCollector());

    if (!collector) {
        // TODO: dummy collector & error handling;

        return collector;
    }

    collector->id_ =
        ApiContext::getInstance()->getManager<InstrumentProfileCollectorManager>()->registerEntity(collector);

    collector->instrumentProfileUpdateListenerHandle_ =
        JavaObjectHandle<InstrumentProfileUpdateListener>(isolated::ipf::InstrumentProfileUpdateListener::create(
            dxfcpp::bit_cast<void *>(&InstrumentProfileCollector::Impl::onInstrumentProfilesUpdate),
            dxfcpp::bit_cast<void *>(collector->id_.getValue())));

    if (!collector->handle_ || !collector->instrumentProfileUpdateListenerHandle_) {
        return collector;
    }

    isolated::ipf::InstrumentProfileCollector::addUpdateListener(
        collector->handle_.get(), collector->instrumentProfileUpdateListenerHandle_.get());

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

} // namespace dxfcpp