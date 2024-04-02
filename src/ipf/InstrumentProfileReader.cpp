// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

InstrumentProfileReader::InstrumentProfileReader() : id_{Id<InstrumentProfileReader>::UNKNOWN}, handle_{} {
    handle_ = dxfcpp::isolated::ipf::InstrumentProfileReader::create();
}

InstrumentProfileReader::Ptr InstrumentProfileReader::create() {
    auto reader = std::shared_ptr<InstrumentProfileReader>(new InstrumentProfileReader());

    reader->id_ = ApiContext::getInstance()->getManager<InstrumentProfileReaderManager>()->registerEntity(reader);

    return reader;
}

std::int64_t InstrumentProfileReader::getLastModified() const {
    return isolated::ipf::InstrumentProfileReader::getLastModified(handle_);
}

bool InstrumentProfileReader::wasComplete() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::ipf::InstrumentProfileReader::wasComplete(handle_.get());
}

std::string InstrumentProfileReader::resolveSourceURL(const std::string &address) noexcept {
    return isolated::ipf::InstrumentProfileReader::resolveSourceURL(address);
}

std::vector<std::shared_ptr<InstrumentProfile>>
InstrumentProfileReader::readFromFile(const std::string &address) const {
    if (!handle_) {
        return {};
    }

    auto *list = dxfcpp::isolated::ipf::InstrumentProfileReader::readFromFile(handle_.get(), address);
    auto result = InstrumentProfile::fromGraalList(list);

    dxfcpp::isolated::ipf::InstrumentProfileList::release(list);

    return result;
}

std::vector<std::shared_ptr<InstrumentProfile>>
InstrumentProfileReader::readFromFile(const std::string &address, const std::string &user,
                                      const std::string &password) const {
    if (!handle_) {
        return {};
    }

    auto *list = dxfcpp::isolated::ipf::InstrumentProfileReader::readFromFile(handle_.get(), address, user, password);
    auto result = InstrumentProfile::fromGraalList(list);

    dxfcpp::isolated::ipf::InstrumentProfileList::release(list);

    return result;
}

DXFCPP_END_NAMESPACE