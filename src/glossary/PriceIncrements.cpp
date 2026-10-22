// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/glossary/PriceIncrements.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/glossary/IsolatedPriceIncrements.hpp"

DXFCPP_BEGIN_NAMESPACE

const PriceIncrements::Ptr &PriceIncrements::EMPTY() {
    static const Ptr value = createShared(&isolated::glossary::IsolatedPriceIncrements::EMPTY);

    return value;
}

PriceIncrements::PriceIncrements(LockExternalConstructionTag, JavaObjectHandle<PriceIncrements> (*initializer)())
    : handle_(initializer) {
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
    return isolated::glossary::IsolatedPriceIncrements::getText(getHandle());
}

std::vector<double> PriceIncrements::getPriceIncrements() const {
    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrements(getHandle());
}

double PriceIncrements::getPriceIncrement() const {
    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(getHandle());
}

double PriceIncrements::getPriceIncrement(double price) const {
    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(getHandle(), price);
}

double PriceIncrements::getPriceIncrement(double price, std::int32_t direction) const {
    return isolated::glossary::IsolatedPriceIncrements::getPriceIncrement(getHandle(), price, direction);
}

std::int32_t PriceIncrements::getPricePrecision() const {
    return isolated::glossary::IsolatedPriceIncrements::getPricePrecision(getHandle());
}

std::int32_t PriceIncrements::getPricePrecision(double price) const {
    return isolated::glossary::IsolatedPriceIncrements::getPricePrecision(getHandle(), price);
}

double PriceIncrements::roundPrice(double price) const {
    return isolated::glossary::IsolatedPriceIncrements::roundPrice(getHandle(), price);
}

double PriceIncrements::roundPrice(double price, std::int32_t direction) const {
    return isolated::glossary::IsolatedPriceIncrements::roundPrice(getHandle(), price, direction);
}

double PriceIncrements::roundPrice(double price, RoundingMode roundingMode) const {
    return isolated::glossary::IsolatedPriceIncrements::roundPrice(getHandle(), price, roundingMode);
}

double PriceIncrements::incrementPrice(double price, std::int32_t direction) const {
    return isolated::glossary::IsolatedPriceIncrements::incrementPrice(getHandle(), price, direction);
}

double PriceIncrements::incrementPrice(double price, std::int32_t direction, double step) const {
    return isolated::glossary::IsolatedPriceIncrements::incrementPrice(getHandle(), price, direction, step);
}

bool PriceIncrements::operator==(const PriceIncrements &other) const {
    if (this == &other) {
        return true;
    }

    return JavaObject::equals(getHandle().get(), other.getHandle().get());
}

std::size_t PriceIncrements::hashCode() const noexcept {
    try {
        return JavaObject::hashCode(getHandle().get());
    } catch (...) {
        return 0;
    }
}

std::string PriceIncrements::toString() const {
    return JavaObject::toString(getHandle().get());
}

DXFCPP_END_NAMESPACE