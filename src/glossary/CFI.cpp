// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/glossary/CFI.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/glossary/IsolatedCFI.hpp"

DXFCPP_BEGIN_NAMESPACE

void CFI::init() const {
    if (initializer_) {
        std::call_once(initialized_, [this] {
            handle_ = initializer_();
        });
    }
}

const CFI::Ptr &CFI::EMPTY() {
    static const Ptr value = createShared(&isolated::glossary::IsolatedCFI::EMPTY);

    return value;
}

CFI::CFI(LockExternalConstructionTag, JavaObjectHandle<CFI> (*initializer)()) : initializer_(initializer) {
}

CFI::CFI(LockExternalConstructionTag, JavaObjectHandle<CFI> &&handle) : handle_(std::move(handle)) {
}

CFI::~CFI() noexcept {
}

CFI::Ptr CFI::valueOf(const StringLike &code) {
    return createShared(isolated::glossary::IsolatedCFI::valueOf(code));
}

CFI::Ptr CFI::valueOf(std::int32_t intCode) {
    return createShared(isolated::glossary::IsolatedCFI::valueOf(intCode));
}

std::string CFI::getCode() const {
    init();

    return isolated::glossary::IsolatedCFI::getCode(handle_);
}

std::int32_t CFI::getIntCode() const {
    init();

    return isolated::glossary::IsolatedCFI::getIntCode(handle_);
}

std::int16_t CFI::getCategory() const {
    init();

    return isolated::glossary::IsolatedCFI::getCategory(handle_);
}

char CFI::getCategoryChar() const {
    const auto category = getCategory();

    return utf16to8(category);
}

std::int16_t CFI::getGroup() const {
    init();

    return isolated::glossary::IsolatedCFI::getGroup(handle_);
}

char CFI::getGroupChar() const {
    const auto group = getGroup();

    return utf16to8(group);
}

bool CFI::isEquity() const {
    init();

    return isolated::glossary::IsolatedCFI::isEquity(handle_);
}

bool CFI::isDebtInstrument() const {
    init();

    return isolated::glossary::IsolatedCFI::isDebtInstrument(handle_);
}

bool CFI::isEntitlement() const {
    init();

    return isolated::glossary::IsolatedCFI::isEntitlement(handle_);
}

bool CFI::isOption() const {
    init();

    return isolated::glossary::IsolatedCFI::isOption(handle_);
}

bool CFI::isFuture() const {
    init();

    return isolated::glossary::IsolatedCFI::isFuture(handle_);
}

bool CFI::isOther() const {
    init();

    return isolated::glossary::IsolatedCFI::isOther(handle_);
}

std::vector<std::shared_ptr<CFI::Value>> CFI::decipher() const {
    init();

    auto values = isolated::glossary::IsolatedCFI::decipher(handle_);
    std::vector<std::shared_ptr<Value>> result{};

    result.reserve(values.size());

    for (auto &value : values) {
        result.emplace_back(Value::createShared(JavaObjectHandle<Value>{value}));
    }

    result.shrink_to_fit();

    return result;
}

std::string CFI::describe() const {
    init();

    return isolated::glossary::IsolatedCFI::describe(handle_);
}

bool CFI::operator==(const CFI &other) const {
    if (this == &other) {
        return true;
    }

    init();
    other.init();

    return JavaObject::equals(handle_.get(), other.handle_.get());
}

std::size_t CFI::hashCode() const noexcept {
    try {
        init();

        return JavaObject::hashCode(handle_.get());
    } catch (...) {
        return 0;
    }
}

std::string CFI::toString() const {
    init();

    return JavaObject::toString(handle_.get());
}

CFI::Attribute::Attribute(LockExternalConstructionTag, JavaObjectHandle<Attribute> &&handle)
    : handle_(std::move(handle)) {
}

CFI::Attribute::~Attribute() noexcept {
}

std::string CFI::Attribute::getName() const {
    return isolated::glossary::IsolatedCFI::IsolatedAttribute::getName(handle_);
}

std::string CFI::Attribute::getDescription() const {
    return isolated::glossary::IsolatedCFI::IsolatedAttribute::getDescription(handle_);
}

std::vector<std::shared_ptr<CFI::Value>> CFI::Attribute::getValues() const {
    auto values = isolated::glossary::IsolatedCFI::IsolatedAttribute::getValues(handle_);
    std::vector<std::shared_ptr<Value>> result{};

    result.reserve(values.size());

    for (auto &value : values) {
        result.emplace_back(Value::createShared(JavaObjectHandle<Value>{value}));
    }

    result.shrink_to_fit();

    return result;
}

bool CFI::Attribute::operator==(const Attribute &other) const {
    return JavaObject::equals(handle_.get(), other.handle_.get());
}

std::size_t CFI::Attribute::hashCode() const noexcept {
    return JavaObject::hashCode(handle_.get());
}

std::string CFI::Attribute::toString() const {
    return JavaObject::toString(handle_.get());
}

CFI::Value::Value(LockExternalConstructionTag, JavaObjectHandle<Value> &&handle) : handle_(std::move(handle)) {
}

CFI::Value::~Value() noexcept {
}

CFI::Attribute::Ptr CFI::Value::getAttribute() const {
    return Attribute::createShared(isolated::glossary::IsolatedCFI::IsolatedValue::getAttribute(handle_));
}

std::int16_t CFI::Value::getCode() const {
    return isolated::glossary::IsolatedCFI::IsolatedValue::getCode(handle_);
}

char CFI::Value::getCodeChar() const {
    auto code = getCode();

    return utf16to8(code);
}

std::string CFI::Value::getName() const {
    return isolated::glossary::IsolatedCFI::IsolatedValue::getName(handle_);
}

std::string CFI::Value::getDescription() const {
    return isolated::glossary::IsolatedCFI::IsolatedValue::getDescription(handle_);
}

bool CFI::Value::operator==(const Value &other) const {
    return JavaObject::equals(handle_.get(), other.handle_.get());
}

std::size_t CFI::Value::hashCode() const noexcept {
    return JavaObject::hashCode(handle_.get());
}

std::string CFI::Value::toString() const {
    return JavaObject::toString(handle_.get());
}

DXFCPP_END_NAMESPACE