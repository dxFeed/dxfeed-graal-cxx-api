// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/ipf/live/IterableInstrumentProfile.hpp"

#include "../../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfile.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/ipf/live/IsolatedInstrumentProfileCollector.hpp"

#include <memory>

DXFCPP_BEGIN_NAMESPACE

IterableInstrumentProfile::IterableInstrumentProfile(void *handle) noexcept : handle_(handle) {
}

std::shared_ptr<IterableInstrumentProfile> IterableInstrumentProfile::create(void *handle) noexcept {
    // ReSharper disable once CppDFAMemoryLeak
    return std::shared_ptr<IterableInstrumentProfile>(new IterableInstrumentProfile(handle));
}

bool IterableInstrumentProfile::hasNext() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::ipf::live::IsolatedInstrumentProfileIterator::hasNext(handle_.get());
}

std::shared_ptr<InstrumentProfile> IterableInstrumentProfile::next() const {
    const auto graalProfile = isolated::ipf::live::IsolatedInstrumentProfileIterator::next(handle_.get());
    auto result = InstrumentProfile::create(JavaObjectHandle<InstrumentProfile>(graalProfile));

    return result;
};

DXFCPP_END_NAMESPACE
