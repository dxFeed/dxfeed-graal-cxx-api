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
    handle_ = dxfcpp::isolated::ipf::IsolatedInstrumentProfileReader::create();
}

InstrumentProfileReader::Ptr InstrumentProfileReader::create() {
    auto reader = std::shared_ptr<InstrumentProfileReader>(new InstrumentProfileReader());

    reader->id_ = ApiContext::getInstance()->getManager<InstrumentProfileReaderManager>()->registerEntity(reader);

    return reader;
}

std::int64_t InstrumentProfileReader::getLastModified() const {
    return isolated::ipf::IsolatedInstrumentProfileReader::getLastModified(handle_);
}

bool InstrumentProfileReader::wasComplete() const {
    return isolated::ipf::IsolatedInstrumentProfileReader::wasComplete(handle_);
}

std::string InstrumentProfileReader::resolveSourceURL(const StringLikeWrapper &address) {
    return isolated::ipf::IsolatedInstrumentProfileReader::resolveSourceURL(address);
}

std::vector<std::shared_ptr<InstrumentProfile>>
InstrumentProfileReader::readFromFile(const StringLikeWrapper &address) const {
    auto *list = dxfcpp::isolated::ipf::IsolatedInstrumentProfileReader::readFromFile(handle_, address);
    auto result = InstrumentProfile::fromGraalList(list);

    dxfcpp::isolated::ipf::InstrumentProfileList::release(list);

    return result;
}

std::vector<std::shared_ptr<InstrumentProfile>>
InstrumentProfileReader::readFromFile(const StringLikeWrapper &address, const StringLikeWrapper &user,
                                      const StringLikeWrapper &password) const {
    auto *list = dxfcpp::isolated::ipf::IsolatedInstrumentProfileReader::readFromFile(handle_, address, user, password);
    auto result = InstrumentProfile::fromGraalList(list);

    dxfcpp::isolated::ipf::InstrumentProfileList::release(list);

    return result;
}

DXFCPP_END_NAMESPACE