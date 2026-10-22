// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/glossary/AdditionalUnderlyings.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/glossary/IsolatedAdditionalUnderlyings.hpp"

DXFCPP_BEGIN_NAMESPACE

JavaObjectHandle<AdditionalUnderlyings>
AdditionalUnderlyings::valueOfImpl(const std::vector<std::pair<const char *, double>> &mapLikeEntries) {
    return isolated::glossary::IsolatedAdditionalUnderlyings::valueOf(mapLikeEntries);
}

const AdditionalUnderlyings::Ptr &AdditionalUnderlyings::EMPTY() {
    static const Ptr value = createShared(&isolated::glossary::IsolatedAdditionalUnderlyings::EMPTY);

    return value;
}

AdditionalUnderlyings::AdditionalUnderlyings(LockExternalConstructionTag,
                                             JavaObjectHandle<AdditionalUnderlyings> (*initializer)())
    : handle_(initializer) {
}

AdditionalUnderlyings::AdditionalUnderlyings(LockExternalConstructionTag,
                                                    JavaObjectHandle<AdditionalUnderlyings> &&handle)
    : handle_(std::move(handle)) {
}

AdditionalUnderlyings::~AdditionalUnderlyings() noexcept {
}

AdditionalUnderlyings::Ptr AdditionalUnderlyings::valueOf(const StringLike &text) {
    return createShared(isolated::glossary::IsolatedAdditionalUnderlyings::valueOf(text));
}

double AdditionalUnderlyings::getSPC(const StringLike &text, const StringLike &symbol) {
    return isolated::glossary::IsolatedAdditionalUnderlyings::getSPC(text, symbol);
}

std::string AdditionalUnderlyings::getText() const {
    return isolated::glossary::IsolatedAdditionalUnderlyings::getText(getHandle());
}

std::unordered_map<std::string, double> AdditionalUnderlyings::getMap() const {
    return isolated::glossary::IsolatedAdditionalUnderlyings::getMap(getHandle());
}

double AdditionalUnderlyings::getSPC(const StringLike &symbol) const {
    return isolated::glossary::IsolatedAdditionalUnderlyings::getSPC(getHandle(), symbol);
}

bool AdditionalUnderlyings::operator==(const AdditionalUnderlyings &other) const {
    if (this == &other) {
        return true;
    }

    return JavaObject::equals(getHandle().get(), other.getHandle().get());
}

std::size_t AdditionalUnderlyings::hashCode() const noexcept {
    try {
        return JavaObject::hashCode(getHandle().get());
    } catch (...) {
        return 0;
    }
}

std::string AdditionalUnderlyings::toString() const {
    return JavaObject::toString(getHandle().get());
}

DXFCPP_END_NAMESPACE
