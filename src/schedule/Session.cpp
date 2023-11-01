// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

Session::Session(void* handle) noexcept : handle_(handle) {
}

Session::Ptr Session::create(void *handle) noexcept {
    if (!handle) {
        return {};
    }

    return std::shared_ptr<Session>(new (std::nothrow) Session(handle));
}

Day::Ptr Session::getDay() const noexcept {
    if (!handle_) {
        return {};
    }

    return Day::create(isolated::schedule::Session::getDay(handle_.get()));
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

    return Session::create(isolated::schedule::Session::getNextSession(handle_.get(), filter.handle_.get()));
}

std::string Session::toString() const noexcept {
    if (!handle_) {
        return dxfcpp::String::EMPTY;
    }

    return isolated::schedule::Session::toString(handle_.get());
}

} // namespace dxfcpp