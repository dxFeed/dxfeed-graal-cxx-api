// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

void AdditionalUnderlyings::init() const {
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

AdditionalUnderlyings::Ptr AdditionalUnderlyings::valueOf(const StringLikeWrapper &text) {
    return createShared(isolated::glossary::IsolatedAdditionalUnderlyings::valueOf(text));
}

double AdditionalUnderlyings::getSPC(const StringLikeWrapper &text, const StringLikeWrapper &symbol) {
    return isolated::glossary::IsolatedAdditionalUnderlyings::getSPC(text, symbol);
}

std::string AdditionalUnderlyings::getText() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedAdditionalUnderlyings::getText(handle_);
}

std::unordered_map<std::string, double> AdditionalUnderlyings::getMap() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedAdditionalUnderlyings::getMap(handle_);
}

double AdditionalUnderlyings::getSPC(const StringLikeWrapper &symbol) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedAdditionalUnderlyings::getSPC(handle_, symbol);
}

bool AdditionalUnderlyings::operator==(const AdditionalUnderlyings &other) const {
    std::scoped_lock lock(mtx_, other.mtx_);

    init();
    other.init();

    return JavaObject::equals(handle_.get(), other.handle_.get()) == 0;
}

std::size_t AdditionalUnderlyings::hashCode() const noexcept {
    try {
        std::lock_guard lock(mtx_);

        init();

        return JavaObject::hashCode(handle_.get());
    } catch (...) {
        return 0;
    }
}

std::string AdditionalUnderlyings::toString() const {
    std::lock_guard lock(mtx_);

    init();

    return JavaObject::toString(handle_.get());
}

DXFCPP_END_NAMESPACE
