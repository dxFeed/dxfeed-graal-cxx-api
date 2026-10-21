// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/glossary/PriceIncrements.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/glossary/IsolatedPriceIncrements.hpp"

DXFCPP_BEGIN_NAMESPACE

void PriceIncrements::init() const {
    if (initializer_) {
        std::call_once(initialized_, [this] {
            handle_ = initializer_();
        });
    }
}

const PriceIncrements::Ptr &PriceIncrements::EMPTY() {
    static const Ptr value = createShared(&isolated::glossary::IsolatedPriceIncrements::EMPTY);

    return value;
}

PriceIncrements::PriceIncrements(LockExternalConstructionTag, JavaObjectHandle<PriceIncrements> (*initializer)())
    : initializer_(initializer) {
}

PriceIncrements::PriceIncrements(LockExternalConstructionTag, JavaObjectHandle<PriceIncrements> &&handle)
    : handle_(std::move(handle)) {
}

PriceIncrements::~PriceIncrements() noexcept {
}

PriceIncrements::Ptr PriceIncrements::valueOf(const StringLike &text) {
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
    init();

    return isolated::glossary::IsolatedPriceIncrements::getText(handle_);
}

std::vector<double> PriceIncrements::getPriceIncrements() const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrements(handle_);
}

double PriceIncrements::getPriceIncrement() const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(handle_);
}

double PriceIncrements::getPriceIncrement(double price) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(handle_, price);
}

double PriceIncrements::getPriceIncrement(double price, std::int32_t direction) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(handle_, price, direction);
}

std::int32_t PriceIncrements::getPricePrecision() const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::getPricePrecision(handle_);
}

std::int32_t PriceIncrements::getPricePrecision(double price) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::getPricePrecision(handle_, price);
}

double PriceIncrements::roundPrice(double price) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::roundPrice(handle_, price);
}

double PriceIncrements::roundPrice(double price, std::int32_t direction) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::roundPrice(handle_, price, direction);
}

double PriceIncrements::roundPrice(double price, RoundingMode roundingMode) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::roundPrice(handle_, price, roundingMode);
}

double PriceIncrements::incrementPrice(double price, std::int32_t direction) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::incrementPrice(handle_, price, direction);
}

double PriceIncrements::incrementPrice(double price, std::int32_t direction, double step) const {
    init();

    return isolated::glossary::IsolatedPriceIncrements::incrementPrice(handle_, price, direction, step);
}

bool PriceIncrements::operator==(const PriceIncrements &other) const {
    if (this == &other) {
        return true;
    }

    init();
    other.init();

    return JavaObject::equals(handle_.get(), other.handle_.get());
}

std::size_t PriceIncrements::hashCode() const noexcept {
    try {
        init();

        return JavaObject::hashCode(handle_.get());
    } catch (...) {
        return 0;
    }
}

std::string PriceIncrements::toString() const {
    init();

    return JavaObject::toString(handle_.get());
}

DXFCPP_END_NAMESPACE