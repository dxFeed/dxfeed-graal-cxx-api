// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/glossary/AdditionalUnderlyings.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/glossary/IsolatedAdditionalUnderlyings.hpp"

DXFCPP_BEGIN_NAMESPACE

void AdditionalUnderlyings::init() const {
    if (initializer_) {
        std::call_once(initialized_, [this] {
            handle_ = initializer_();
        });
    }
}

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
    : initializer_(initializer) {
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
    init();

    return isolated::glossary::IsolatedAdditionalUnderlyings::getText(handle_);
}

std::unordered_map<std::string, double> AdditionalUnderlyings::getMap() const {
    init();

    return isolated::glossary::IsolatedAdditionalUnderlyings::getMap(handle_);
}

double AdditionalUnderlyings::getSPC(const StringLike &symbol) const {
    init();

    return isolated::glossary::IsolatedAdditionalUnderlyings::getSPC(handle_, symbol);
}

bool AdditionalUnderlyings::operator==(const AdditionalUnderlyings &other) const {
    if (this == &other) {
        return true;
    }

    init();
    other.init();

    return JavaObject::equals(handle_.get(), other.handle_.get());
}

std::size_t AdditionalUnderlyings::hashCode() const noexcept {
    try {
        init();

        return JavaObject::hashCode(handle_.get());
    } catch (...) {
        return 0;
    }
}

std::string AdditionalUnderlyings::toString() const {
    init();

    return JavaObject::toString(handle_.get());
}

DXFCPP_END_NAMESPACE
