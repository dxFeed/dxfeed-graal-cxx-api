// Copyright (c) 2024 Devexperts LLC.
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

InstrumentProfile::Ptr InstrumentProfile::create() {
    return createShared(isolated::ipf::IsolatedInstrumentProfile::create());
}

InstrumentProfile::Ptr InstrumentProfile::create(Ptr ip) {
    return createShared(isolated::ipf::IsolatedInstrumentProfile::create(ip->handle_));
}

std::string InstrumentProfile::getType() const {
    return isolated::ipf::IsolatedInstrumentProfile::getType(handle_);
}

void InstrumentProfile::setType(const StringLikeWrapper &type) const {
    isolated::ipf::IsolatedInstrumentProfile::setType(handle_, type);
}

std::string InstrumentProfile::getSymbol() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSymbol(handle_);
}

void InstrumentProfile::setSymbol(const StringLikeWrapper &symbol) const {
    isolated::ipf::IsolatedInstrumentProfile::setSymbol(handle_, symbol);
}

std::string InstrumentProfile::getDescription() const {
    return isolated::ipf::IsolatedInstrumentProfile::getDescription(handle_);
}

void InstrumentProfile::setDescription(const StringLikeWrapper &description) const {
    isolated::ipf::IsolatedInstrumentProfile::setDescription(handle_, description);
}


std::string InstrumentProfile::getLocalSymbol() const {
    return isolated::ipf::IsolatedInstrumentProfile::getLocalSymbol(handle_);
}

void InstrumentProfile::setLocalSymbol(const StringLikeWrapper &localSymbol) const {
    isolated::ipf::IsolatedInstrumentProfile::setLocalSymbol(handle_, localSymbol);
}

InstrumentProfile::~InstrumentProfile() noexcept {}

InstrumentProfile::InstrumentProfile(LockExternalConstructionTag, JavaObjectHandle<InstrumentProfile> &&handle) : handle_(std::move(handle)) {
}

InstrumentProfile::Ptr InstrumentProfile::create(JavaObjectHandle<InstrumentProfile> &&handle) {
    return createShared(std::move(handle));
}

std::vector<std::shared_ptr<InstrumentProfile>> InstrumentProfile::List::fromGraal(void *graalList) {
    using ListType = dxfg_instrument_profile_list;
    using SizeType = decltype(ListType::size);

    if (!graalList) {
        return {};
    }

    std::vector<std::shared_ptr<InstrumentProfile>> result{};

    const auto list = static_cast<ListType *>(graalList);

    if (list->size <= 0 || list->elements == nullptr) {
        return result;
    }

    for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
        if (list->elements[elementIndex]) {
            result.emplace_back(create(JavaObjectHandle<InstrumentProfile>(list->elements[elementIndex])));
        }
    }

    return result;
}

DXFCPP_END_NAMESPACE