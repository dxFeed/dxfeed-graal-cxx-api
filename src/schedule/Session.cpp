// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

Session::Session(JavaObjectHandle<Session> &&handle) noexcept : handle_(std::move(handle)) {
}

Session::Ptr Session::create(JavaObjectHandle<Session> &&handle) {
    if (!handle) {
        throw InvalidArgumentException("Unable to create a Session object. The handle is invalid");
    }

    return std::shared_ptr<Session>(new Session(std::move(handle)));
}

Day::Ptr Session::getDay() const noexcept {
    if (!handle_) {
        return {};
    }

    return Day::create(JavaObjectHandle<Day>{isolated::schedule::Session::getDay(handle_.get())});
}

const SessionType &Session::getType() const & noexcept {
    if (!handle_) {
        return SessionType::NO_TRADING;
    }

    switch (static_cast<SessionTypeEnum>(isolated::schedule::Session::getType(handle_.get()))) {
    case SessionTypeEnum::NO_TRADING:
        return SessionType::NO_TRADING;
    case SessionTypeEnum::PRE_MARKET:
        return SessionType::PRE_MARKET;
    case SessionTypeEnum::REGULAR:
        return SessionType::REGULAR;
    case SessionTypeEnum::AFTER_MARKET:
        return SessionType::AFTER_MARKET;
    }

    return SessionType::NO_TRADING;
}

bool Session::isTrading() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Session::isTrading(handle_.get());
}

bool Session::isEmpty() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Session::isEmpty(handle_.get());
}

std::int64_t Session::getStartTime() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Session::getStartTime(handle_.get());
}

std::int64_t Session::getEndTime() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::schedule::Session::getEndTime(handle_.get());
}

bool Session::containsTime(std::int64_t time) const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Session::containsTime(handle_.get(), time);
}

Session::Ptr Session::getPrevSession(const SessionFilter &filter) const noexcept {
    if (!handle_ || !filter.getHandle()) {
        return {};
    }

    return Session::create(JavaObjectHandle<Session>{
        isolated::schedule::Session::getPrevSession(handle_.get(), filter.getHandle().get())});
}

Session::Ptr Session::getNextSession(const SessionFilter &filter) const noexcept {
    if (!handle_ || !filter.getHandle()) {
        return {};
    }

    return Session::create(JavaObjectHandle<Session>{
        isolated::schedule::Session::getNextSession(handle_.get(), filter.getHandle().get())});
}

Session::Ptr Session::findPrevSession(const SessionFilter &filter) const noexcept {
    if (!handle_ || !filter.getHandle()) {
        return {};
    }

    return Session::create(JavaObjectHandle<Session>{
        isolated::schedule::Session::findPrevSession(handle_.get(), filter.getHandle().get())});
}

Session::Ptr Session::findNextSession(const SessionFilter &filter) const noexcept {
    if (!handle_ || !filter.getHandle()) {
        return {};
    }

    return Session::create(JavaObjectHandle<Session>{
        isolated::schedule::Session::findNextSession(handle_.get(), filter.getHandle().get())});
}

bool Session::operator==(const Session &other) const noexcept {
    if (!handle_ || !other.handle_) {
        return false;
    }

    if (this == &other) {
        return true;
    }

    return isolated::schedule::Session::equals(handle_.get(), other.handle_.get());
}

std::size_t Session::getHashCode() const noexcept {
    if (!handle_) {
        return dxfcpp::bit_cast<std::size_t>(this);
    }

    return isolated::schedule::Session::getHashCode(handle_.get());
}

std::string Session::toString() const {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Session::toString(handle_.get());
}

DXFCPP_END_NAMESPACE