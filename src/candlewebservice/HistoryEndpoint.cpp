// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/candlewebservice/HistoryEndpoint.hpp"

#include "../../include/dxfeed_graal_c_api/api.h"
#include "../../include/dxfeed_graal_cpp_api/isolated/candlewebservice/IsolatedHistoryEndpoint.hpp"

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

HistoryEndpoint::~HistoryEndpoint() {
}

HistoryEndpoint::Builder::Builder(LockExternalConstructionTag,
                                  JavaObjectHandle<Builder> &&handle)
    : handle_(std::move(handle)) {
}

HistoryEndpoint::Builder::~Builder() {
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
    return HistoryEndpoint::createShared(isolated::candlewebservice::IsolatedHistoryEndpoint::Builder::build(handle_));
}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::newBuilder() {
    return Builder::createShared(isolated::candlewebservice::IsolatedHistoryEndpoint::newBuilder());
}

DXFCPP_END_NAMESPACE