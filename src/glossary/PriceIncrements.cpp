// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

void PriceIncrements::init() const {
    if (!initialized_) {
        handle_ = initializer_();
        initialized_ = true;
    }
}

const PriceIncrements::Ptr PriceIncrements::EMPTY(createShared([] {
    return isolated::glossary::IsolatedPriceIncrements::EMPTY();
}));

PriceIncrements::PriceIncrements(LockExternalConstructionTag,
                                 std::function<JavaObjectHandle<PriceIncrements>()> initializer)
    : initializer_(std::move(initializer)) {
}

PriceIncrements::PriceIncrements(LockExternalConstructionTag, JavaObjectHandle<PriceIncrements> &&handle)
    : handle_(std::move(handle)) {
    initialized_ = true;
}

PriceIncrements::~PriceIncrements() noexcept {
}

PriceIncrements::Ptr PriceIncrements::valueOf(const StringLikeWrapper &text) {
    return createShared(isolated::glossary::IsolatedPriceIncrements::valueOf(text));
}

PriceIncrements::Ptr PriceIncrements::valueOf(double increment) {
    return createShared(isolated::glossary::IsolatedPriceIncrements::valueOf(increment));
}

PriceIncrements::Ptr PriceIncrements::valueOf(const std::vector<double> &increments) {
    return createShared(isolated::glossary::IsolatedPriceIncrements::valueOf(increments));
}

PriceIncrements::Ptr PriceIncrements::valueOf(std::initializer_list<double> increments) {
    return createShared(isolated::glossary::IsolatedPriceIncrements::valueOf(increments));
}

std::string PriceIncrements::getText() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::getText(handle_);
}

std::vector<double> PriceIncrements::getPriceIncrements() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrements(handle_);
}

double PriceIncrements::getPriceIncrement() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(handle_);
}

double PriceIncrements::getPriceIncrement(double price) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(handle_, price);
}

double PriceIncrements::getPriceIncrement(double price, std::int32_t direction) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(handle_, price, direction);
}

std::int32_t PriceIncrements::getPricePrecision() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::getPricePrecision(handle_);
}

std::int32_t PriceIncrements::getPricePrecision(double price) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::getPricePrecision(handle_, price);
}

double PriceIncrements::roundPrice(double price) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::roundPrice(handle_, price);
}

double PriceIncrements::roundPrice(double price, std::int32_t direction) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::roundPrice(handle_, price, direction);
}

double PriceIncrements::roundPrice(double price, RoundingMode roundingMode) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::roundPrice(handle_, price, roundingMode);
}

double PriceIncrements::incrementPrice(double price, std::int32_t direction) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::incrementPrice(handle_, price, direction);
}

double PriceIncrements::incrementPrice(double price, std::int32_t direction, double step) const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedPriceIncrements::incrementPrice(handle_, price, direction, step);
}

bool PriceIncrements::operator==(const PriceIncrements &other) const {
    std::scoped_lock lock(mtx_, other.mtx_);

    init();
    other.init();

    return JavaObject::equals(handle_.get(), other.handle_.get()) == 0;
}

std::size_t PriceIncrements::hashCode() const noexcept {
    try {
        std::lock_guard lock(mtx_);

        init();

        return JavaObject::hashCode(handle_.get());
    } catch (...) {
        return 0;
    }
}

std::string PriceIncrements::toString() const {
    std::lock_guard lock(mtx_);

    init();

    return JavaObject::toString(handle_.get());
}

DXFCPP_END_NAMESPACE