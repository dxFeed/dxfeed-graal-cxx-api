// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

HistoryEndpoint::Builder::Builder(RequireMakeShared<Builder>::LockExternalConstructionTag,
                                  JavaObjectHandle<Builder> &&handle)
    : handle_(std::move(handle)) {
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAddress(const std::string &address) {
    return {};
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withUserName(const std::string &userName) {
    return {};
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withPassword(const std::string &password) {
    return {};
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAuthToken(const std::string &authToken) {
    return {};
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withCompression(Compression compression) {
    return {};
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withFormat(Format format) {
    return {};
}

std::shared_ptr<HistoryEndpoint> HistoryEndpoint::Builder::build() {
    return {};
}

std::shared_ptr<HistoryEndpoint::Builder> newBuilder() {
    return RequireMakeShared<HistoryEndpoint::Builder>::template createShared(
        isolated::candlewebservice::IsolatedHistoryEndpoint::newBuilder());
}

DXFCPP_END_NAMESPACE