// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>
#include <dxfeed_graal_cpp_api/isolated/glossary/IsolatedCFI.hpp>

DXFCPP_BEGIN_NAMESPACE

void CFI::init() const {
    if (!initialized_) {
        handle_ = initializer_();
        initialized_ = true;
    }
}

const CFI::Ptr CFI::EMPTY(createShared([] {
    return isolated::glossary::IsolatedCFI::EMPTY();
}));

CFI::CFI(LockExternalConstructionTag, std::function<JavaObjectHandle<CFI>()> initializer)
    : initializer_(std::move(initializer)) {
}

CFI::CFI(LockExternalConstructionTag, JavaObjectHandle<CFI> &&handle) : handle_(std::move(handle)) {
    initialized_ = true;
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
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::getCode(handle_);
}

std::int32_t CFI::getIntCode() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::getIntCode(handle_);
}

std::int16_t CFI::getCategory() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::getCategory(handle_);
}

char CFI::getCategoryChar() const {
    const auto category = getCategory();

    return utf16to8(category);
}

std::int16_t CFI::getGroup() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::getGroup(handle_);
}

char CFI::getGroupChar() const {
    const auto group = getGroup();

    return utf16to8(group);
}

bool CFI::isEquity() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::isEquity(handle_);
}

bool CFI::isDebtInstrument() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::isDebtInstrument(handle_);
}

bool CFI::isEntitlement() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::isEntitlement(handle_);
}

bool CFI::isOption() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::isOption(handle_);
}

bool CFI::isFuture() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::isFuture(handle_);
}

bool CFI::isOther() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::isOther(handle_);
}

std::vector<std::shared_ptr<CFI::Value>> CFI::decipher() const {
    std::lock_guard lock(mtx_);

    init();

    auto values = isolated::glossary::IsolatedCFI::decipher(handle_);
    std::vector<std::shared_ptr<CFI::Value>> result{};

    result.reserve(values.size());

    for (auto &value : values) {
        result.emplace_back(CFI::Value::createShared(JavaObjectHandle<CFI::Value>{value}));
    }

    result.shrink_to_fit();

    return result;
}

std::string CFI::describe() const {
    std::lock_guard lock(mtx_);

    init();

    return isolated::glossary::IsolatedCFI::describe(handle_);
}

bool CFI::operator==(const CFI &other) const {
    std::scoped_lock lock(mtx_, other.mtx_);

    init();
    other.init();

    return JavaObject::equals(handle_.get(), other.handle_.get()) == 0;
}

std::size_t CFI::hashCode() const noexcept {
    try {
        std::lock_guard lock(mtx_);

        init();

        return JavaObject::hashCode(handle_.get());
    } catch (...) {
        return 0;
    }
}

std::string CFI::toString() const {
    std::lock_guard lock(mtx_);

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
    std::vector<std::shared_ptr<CFI::Value>> result{};

    result.reserve(values.size());

    for (auto &value : values) {
        result.emplace_back(CFI::Value::createShared(JavaObjectHandle<CFI::Value>{value}));
    }

    result.shrink_to_fit();

    return result;
}

bool CFI::Attribute::operator==(const Attribute &other) const {
    return JavaObject::equals(handle_.get(), other.handle_.get()) == 0;
}

std::size_t CFI::Attribute::hashCode() const noexcept {
    return JavaObject::hashCode(handle_.get());
}

std::string CFI::Attribute::toString() const {
    return JavaObject::toString(handle_.get());
}

CFI::Value::Value(CFI::Value::LockExternalConstructionTag, JavaObjectHandle<Value> &&handle) : handle_(std::move(handle)) {
}

CFI::Value::~Value() noexcept {
}

CFI::Attribute::Ptr CFI::Value::getAttribute() const {
    return CFI::Attribute::createShared(isolated::glossary::IsolatedCFI::IsolatedValue::getAttribute(handle_));
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
    return JavaObject::equals(handle_.get(), other.handle_.get()) == 0;
}

std::size_t CFI::Value::hashCode() const noexcept {
    return JavaObject::hashCode(handle_.get());
}

std::string CFI::Value::toString() const {
    return JavaObject::toString(handle_.get());
}

DXFCPP_END_NAMESPACE