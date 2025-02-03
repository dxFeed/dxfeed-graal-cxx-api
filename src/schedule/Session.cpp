// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>

#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>
#include <dxfeed_graal_cpp_api/schedule/Day.hpp>
#include <dxfeed_graal_cpp_api/schedule/Session.hpp>
#include <dxfeed_graal_cpp_api/schedule/SessionFilter.hpp>
#include <dxfeed_graal_cpp_api/schedule/SessionType.hpp>
#include <dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp>

#include <dxfeed_graal_cpp_api/isolated/schedule/IsolatedSession.hpp>

DXFCPP_BEGIN_NAMESPACE

Session::Session(JavaObjectHandle<Session> &&handle) noexcept : handle_(std::move(handle)) {
}

Session::Ptr Session::create(JavaObjectHandle<Session> &&handle) {
    if (!handle) {
        throw InvalidArgumentException("Unable to create a Session object. The handle is invalid");
    }

    return std::shared_ptr<Session>(new Session(std::move(handle)));
}

std::shared_ptr<Day> Session::getDay() const {
    return Day::create(isolated::schedule::IsolatedSession::getDay(handle_));
}

const SessionType &Session::getType() const & {
    switch (static_cast<SessionTypeEnum>(isolated::schedule::IsolatedSession::getType(handle_))) {
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

bool Session::isTrading() const {
    return isolated::schedule::IsolatedSession::isTrading(handle_);
}

bool Session::isEmpty() const {
    return isolated::schedule::IsolatedSession::isEmpty(handle_);
}

std::int64_t Session::getStartTime() const {
    return isolated::schedule::IsolatedSession::getStartTime(handle_);
}

std::int64_t Session::getEndTime() const {
    return isolated::schedule::IsolatedSession::getEndTime(handle_);
}

bool Session::containsTime(std::int64_t time) const {
    return isolated::schedule::IsolatedSession::containsTime(handle_, time);
}

Session::Ptr Session::getPrevSession(const SessionFilter &filter) const {
    return create(isolated::schedule::IsolatedSession::getPrevSession(handle_, filter.getHandle()));
}

Session::Ptr Session::getNextSession(const SessionFilter &filter) const {
    return create(isolated::schedule::IsolatedSession::getNextSession(handle_, filter.getHandle()));
}

Session::Ptr Session::findPrevSession(const SessionFilter &filter) const {
    auto sessionHandle = isolated::schedule::IsolatedSession::findPrevSession(handle_, filter.getHandle());

    if (!sessionHandle) {
        return {};
    }

    return create(std::move(sessionHandle));
}

Session::Ptr Session::findNextSession(const SessionFilter &filter) const {
    auto sessionHandle = isolated::schedule::IsolatedSession::findNextSession(handle_, filter.getHandle());

    if (!sessionHandle) {
        return {};
    }

    return create(std::move(sessionHandle));
}

bool Session::operator==(const Session &other) const {
    if (this == &other) {
        return true;
    }

    return isolated::schedule::IsolatedSession::equals(handle_, other.handle_);
}

std::size_t Session::getHashCode() const {
    return isolated::schedule::IsolatedSession::getHashCode(handle_);
}

std::string Session::toString() const {
    return isolated::schedule::IsolatedSession::toString(handle_);
}

DXFCPP_END_NAMESPACE