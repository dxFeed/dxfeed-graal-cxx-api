// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

Session::Session() noexcept : handle_{} {
}

Day::Ptr Session::getDay() const noexcept {
    if (!handle_) {
        return {};
    }

    auto graalDay = isolated::schedule::Session::getDay(handle_.get());

    if (!graalDay) {
        return {};
    }

    std::shared_ptr<Day> day{new (std::nothrow) Day{}};

    if (!day) {
        return {};
    }

    day->handle_ = JavaObjectHandle<Day>(graalDay);

    return day;
}

bool Session::isTrading() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::schedule::Session::isTrading(handle_.get());
}

Session::Ptr Session::getNextSession(const SessionFilter &filter) const noexcept {
    if (!handle_ || !filter.handle_) {
        return {};
    }

    auto graalSession = isolated::schedule::Session::getNextSession(handle_.get(), filter.handle_.get());

    if (!graalSession) {
        return {};
    }

    std::shared_ptr<Session> session{new (std::nothrow) Session{}};

    if (!session) {
        return {};
    }

    session->handle_ = JavaObjectHandle<Session>(graalSession);

    return session;
}

std::string Session::toString() const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Session::toString(handle_.get());
}

} // namespace dxfcpp