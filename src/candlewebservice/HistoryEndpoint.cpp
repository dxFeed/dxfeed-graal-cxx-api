// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

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

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAddress(const std::string &address) {}

std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withUserName(const std::string &userName) {}
std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withPassword(const std::string &password) {}
std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withAuthToken(const std::string &authToken){}
std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withCompression(Compression compression) {}
std::shared_ptr<HistoryEndpoint::Builder> HistoryEndpoint::Builder::withFormat(Format format) {}
std::shared_ptr<HistoryEndpoint> HistoryEndpoint::Builder::build() {}

std::shared_ptr<HistoryEndpoint::Builder> newBuilder() {}

DXFCPP_END_NAMESPACE