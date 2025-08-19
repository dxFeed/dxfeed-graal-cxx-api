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

std::vector<std::shared_ptr<EventType>> HistoryEndpoint::getTimeSeriesImpl(const EventTypeEnum &eventType,
                                                                           const SymbolWrapper &symbol,
                                                                           std::int64_t from, std::int64_t to) const {
    return isolated::candlewebservice::IsolatedHistoryEndpoint::getTimeSeries(handle_, eventType, symbol, from, to);
}

HistoryEndpoint::HistoryEndpoint(RequireMakeShared<HistoryEndpoint>::LockExternalConstructionTag,
                                 JavaObjectHandle<HistoryEndpoint> &&handle)
    : handle_(std::move(handle)) {
}

HistoryEndpoint::Builder::Builder(RequireMakeShared<Builder>::LockExternalConstructionTag,
                                  JavaObjectHandle<Builder> &&handle)
    : handle_(std::move(handle)) {
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAddress(const std::string &address) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withAddress(handle_, address);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withUserName(const std::string &userName) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withUserName(handle_, userName);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withPassword(const std::string &password) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withPassword(handle_, password);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAuthToken(const std::string &authToken) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withAuthToken(handle_, authToken);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withCompression(Compression compression) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withCompression(handle_, compression);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withFormat(Format format) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withFormat(handle_, format);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint> HistoryEndpoint::Builder::build() {
    auto endpoint = RequireMakeShared<HistoryEndpoint>::template createShared(
        isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::build(handle_));
    auto id = ApiContext::getInstance()->getManager<EntityManager<HistoryEndpoint>>()->registerEntity(endpoint);
    ignoreUnused(id);

    return endpoint;
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::newBuilder() {
    auto builder = RequireMakeShared<HistoryEndpoint::Builder>::template createShared(
        isolated::candlewebservice::IsolatedHistoryEndpoint::newBuilder());
    auto id = ApiContext::getInstance()->getManager<EntityManager<HistoryEndpoint::Builder>>()->registerEntity(builder);
    ignoreUnused(id);

    return builder;
}

DXFCPP_END_NAMESPACE