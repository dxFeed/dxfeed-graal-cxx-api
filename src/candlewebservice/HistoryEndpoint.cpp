// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/internal/context/ApiContext.hpp>
#include <dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp>
#include <dxfeed_graal_cpp_api/isolated/candlewebservice/IsolatedHistoryEndpoint.hpp>
#include <memory>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

std::vector<std::shared_ptr<EventType>> HistoryEndpoint::getTimeSeriesImpl(const EventTypeEnum &eventType,
                                                                           const SymbolWrapper &symbol,
                                                                           std::int64_t from, std::int64_t to) const {
    return isolated::candlewebservice::IsolatedHistoryEndpoint::getTimeSeries(handle_, eventType, symbol, from, to);
}

HistoryEndpoint::HistoryEndpoint(LockExternalConstructionTag,
                                 JavaObjectHandle<HistoryEndpoint> &&handle)
    : handle_(std::move(handle)) {
}

HistoryEndpoint::Builder::Builder(LockExternalConstructionTag,
                                  JavaObjectHandle<Builder> &&handle)
    : handle_(std::move(handle)) {
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAddress(const StringLike &address) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withAddress(handle_, address);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withUserName(const StringLike &userName) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withUserName(handle_, userName);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withPassword(const StringLike &password) {
    isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::withPassword(handle_, password);

    return this->sharedAs<Builder>();
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAuthToken(const StringLike &authToken) {
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

std::shared_ptr<HistoryEndpoint> HistoryEndpoint::Builder::build() const {
    auto endpoint = HistoryEndpoint::createShared(
        isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::build(handle_));
    const auto id = ApiContext::getInstance()->getManager<EntityManager<HistoryEndpoint>>()->registerEntity(endpoint);
    ignoreUnused(id);

    return endpoint;
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::newBuilder() {
    auto builder = Builder::createShared(
        isolated::candlewebservice::IsolatedHistoryEndpoint::newBuilder());
    const auto id = ApiContext::getInstance()->getManager<EntityManager<Builder>>()->registerEntity(builder);
    ignoreUnused(id);

    return builder;
}

DXFCPP_END_NAMESPACE