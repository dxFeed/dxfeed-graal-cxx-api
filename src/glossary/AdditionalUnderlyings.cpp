// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

void AdditionalUnderlyings::init() const {
    std::lock_guard lock(mtx_);

    if (!initialized_) {
        handle_ = initializer_();
        initialized_ = true;
    }
}

JavaObjectHandle<AdditionalUnderlyings>
AdditionalUnderlyings::valueOfImpl(const std::vector<std::pair<const char *, double>> &mapLikeEntries) {
    return isolated::glossary::IsolatedAdditionalUnderlyings::valueOf(mapLikeEntries);
}

const AdditionalUnderlyings::Ptr AdditionalUnderlyings::EMPTY(createShared([] {
    return isolated::glossary::IsolatedAdditionalUnderlyings::EMPTY();
}));

AdditionalUnderlyings::AdditionalUnderlyings(
    LockExternalConstructionTag, std::function<JavaObjectHandle<AdditionalUnderlyings>()> initializer)
    : initializer_(std::move(initializer)) {
}

AdditionalUnderlyings::AdditionalUnderlyings(LockExternalConstructionTag,
                                                    JavaObjectHandle<AdditionalUnderlyings> &&handle)
    : handle_(std::move(handle)) {
    initialized_ = true;
}

AdditionalUnderlyings::~AdditionalUnderlyings() noexcept {
}

AdditionalUnderlyings::Ptr AdditionalUnderlyings::valueOf(const StringLikeWrapper &value) {
    return createShared(isolated::glossary::IsolatedAdditionalUnderlyings::valueOf(value));
}

double AdditionalUnderlyings::getSPC(const StringLikeWrapper &text, const StringLikeWrapper &symbol) {
    return isolated::glossary::IsolatedAdditionalUnderlyings::getSPC(text, symbol);
}

std::string AdditionalUnderlyings::getText() const {
    return {};
}

std::unordered_map<std::string, double> AdditionalUnderlyings::getMap() const {
    return {};
}
double AdditionalUnderlyings::getSPC(const StringLikeWrapper &symbol) const {
    return {};
}
bool AdditionalUnderlyings::operator==(const AdditionalUnderlyings &other) const {
    return {};
}
std::size_t AdditionalUnderlyings::hashCode() const noexcept {
    return {};
}
std::string AdditionalUnderlyings::toString() const {
    return RequireMakeShared<AdditionalUnderlyings>::toString();
}

DXFCPP_END_NAMESPACE
