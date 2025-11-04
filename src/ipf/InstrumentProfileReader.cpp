// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfileReader.hpp"

#include "../../include/dxfeed_graal_cpp_api/auth/AuthToken.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/context/ApiContext.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/ipf/IsolatedInstrumentProfile.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/ipf/IsolatedInstrumentProfileReader.hpp"

#include <memory>

DXFCPP_BEGIN_NAMESPACE

InstrumentProfileReader::InstrumentProfileReader() : id_{Id<InstrumentProfileReader>::UNKNOWN} {
    handle_ = isolated::ipf::IsolatedInstrumentProfileReader::create();
}

InstrumentProfileReader::Ptr InstrumentProfileReader::create() {
    auto reader = std::shared_ptr<InstrumentProfileReader>(new InstrumentProfileReader());

    reader->id_ =
        ApiContext::getInstance()->getManager<EntityManager<InstrumentProfileReader>>()->registerEntity(reader);

    return reader;
}

std::int64_t InstrumentProfileReader::getLastModified() const {
    return isolated::ipf::IsolatedInstrumentProfileReader::getLastModified(handle_);
}

bool InstrumentProfileReader::wasComplete() const {
    return isolated::ipf::IsolatedInstrumentProfileReader::wasComplete(handle_);
}

std::string InstrumentProfileReader::resolveSourceURL(const StringLike &address) {
    return isolated::ipf::IsolatedInstrumentProfileReader::resolveSourceURL(address);
}

std::vector<std::shared_ptr<InstrumentProfile>> InstrumentProfileReader::readFromFile(const StringLike &address) const {
    const auto list = isolated::ipf::IsolatedInstrumentProfileList::toUniqueWrapper(
        isolated::ipf::IsolatedInstrumentProfileReader::readFromFile(handle_, address));
    auto result = InstrumentProfile::List::fromGraal(list.get());

    return result;
}

std::vector<std::shared_ptr<InstrumentProfile>>
InstrumentProfileReader::readFromFile(const StringLike &address, const StringLike &user,
                                      const StringLike &password) const {
    const auto list = isolated::ipf::IsolatedInstrumentProfileList::toUniqueWrapper(
        isolated::ipf::IsolatedInstrumentProfileReader::readFromFile(handle_, address, user, password));
    auto result = InstrumentProfile::List::fromGraal(list.get());

    return result;
}

std::vector<std::shared_ptr<InstrumentProfile>> InstrumentProfileReader::readFromFile(const StringLike &address,
                                                                                      const AuthToken &token) const {
    const auto list = isolated::ipf::IsolatedInstrumentProfileList::toUniqueWrapper(
        isolated::ipf::IsolatedInstrumentProfileReader::readFromFile(handle_, address, token.getHandle()));
    auto result = InstrumentProfile::List::fromGraal(list.get());

    return result;
}

DXFCPP_END_NAMESPACE