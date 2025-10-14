// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

inline AdditionalUnderlyings::AdditionalUnderlyings(LockExternalConstructionTag,
                                                    JavaObjectHandle<AdditionalUnderlyings> &&handle) noexcept
    : handle_(std::move(handle)) {
}

AdditionalUnderlyings::~AdditionalUnderlyings() noexcept {
}
AdditionalUnderlyings::Ptr AdditionalUnderlyings::valueOf(const StringLikeWrapper &value) {
    return TODO_IMPLEMENT_ME;
}
AdditionalUnderlyings::Ptr AdditionalUnderlyings::valueOf(const std::unordered_map<std::string, double> &map) {
    return TODO_IMPLEMENT_ME;
}
AdditionalUnderlyings::Ptr AdditionalUnderlyings::valueOf(const std::map<std::string, double> &map) {
    return TODO_IMPLEMENT_ME;
}
double AdditionalUnderlyings::getSPC(const StringLikeWrapper &text, const StringLikeWrapper &symbol) {
    return TODO_IMPLEMENT_ME;
}
std::string AdditionalUnderlyings::getText() const {
    return TODO_IMPLEMENT_ME;
}
std::unordered_map<std::string, double> AdditionalUnderlyings::getMap() const {
    return TODO_IMPLEMENT_ME;
}
double AdditionalUnderlyings::getSPC(const StringLikeWrapper &symbol) const {
    return TODO_IMPLEMENT_ME;
}
bool AdditionalUnderlyings::operator==(const AdditionalUnderlyings &other) const {
    return TODO_IMPLEMENT_ME;
}
std::size_t AdditionalUnderlyings::hashCode() const {
    return TODO_IMPLEMENT_ME;
}
std::string AdditionalUnderlyings::toString() const {
    return RequireMakeShared<AdditionalUnderlyings>::toString();
}

DXFCPP_END_NAMESPACE
