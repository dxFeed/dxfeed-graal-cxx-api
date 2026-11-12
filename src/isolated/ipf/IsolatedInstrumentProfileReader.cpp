// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedInstrumentProfileReader.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfileReader {

/* dxfg_v4_instrument_profile_reader_t* */ JavaObjectHandle<InstrumentProfileReader> create() {
    return dxfcpp::JavaObjectHandle<InstrumentProfileReader>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_InstrumentProfileReader_new)};
}

std::int64_t getLastModified(
    /* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileReader_getLastModified`. The handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_InstrumentProfileReader_getLastModified,
                                        static_cast<dxfg_v4_instrument_profile_reader_t *>(handle.get()));
}

bool wasComplete(
    /* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileReader_wasComplete`. The handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_InstrumentProfileReader_wasComplete,
                                        static_cast<dxfg_v4_instrument_profile_reader_t *>(handle.get())) == 1;
}

/* dxfg_v4_instrument_profile_list* */ void *
readFromFile(/* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             std::string_view address) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_InstrumentProfileReader_readFromFileAsHandles`. The handle is "
            "invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_InstrumentProfileReader_readFromFileAsHandles,
        static_cast<dxfg_v4_instrument_profile_reader_t *>(handle.get()),
        SdkString{address, "InstrumentProfileReader::readFromFile(address)"}.c_str()));
}

/* dxfg_v4_instrument_profile_list* */ void *
readFromFile(/* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             std::string_view address, std::string_view user, std::string_view password) {
    if (!handle) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_InstrumentProfileReader_readFromFileWithUserAndPasswordAsHandles`. "
                                       "The handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_InstrumentProfileReader_readFromFileWithUserAndPasswordAsHandles,
        static_cast<dxfg_v4_instrument_profile_reader_t *>(handle.get()),
        SdkString{address, "InstrumentProfileReader::readFromFile(address)"}.c_str(),
        SdkString{user, StringLabel::secretOf("InstrumentProfileReader::readFromFile(user)")}.c_str(),
        SdkString{password, StringLabel::secretOf("InstrumentProfileReader::readFromFile(password)")}.c_str()));
}

/// dxfg_v4_InstrumentProfileReader_readFromFileWithTokenAsHandles
/* dxfg_v4_instrument_profile_list* */ void *
readFromFile(/* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             std::string_view address, const JavaObjectHandle<AuthToken> &token) {
    if (!handle) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_InstrumentProfileReader_readFromFileWithTokenAsHandles`. The handle "
                                       "is invalid");
    }

    if (!token) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_InstrumentProfileReader_readFromFileWithTokenAsHandles`. The token "
                                       "is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_InstrumentProfileReader_readFromFileWithTokenAsHandles,
        static_cast<dxfg_v4_instrument_profile_reader_t *>(handle.get()),
        SdkString{address, "InstrumentProfileReader::readFromFile(address)"}.c_str(),
        static_cast<dxfg_v4_auth_token_t *>(token.get())));
}

std::string resolveSourceURL(std::string_view address) {
    const auto resolvedURL = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_InstrumentProfileReader_resolveSourceURL,
        SdkString{address, "InstrumentProfileReader::resolveSourceURL(address)"}.c_str()));

    return dxfcpp::toString(resolvedURL.get());
}

} // namespace isolated::ipf::IsolatedInstrumentProfileReader

DXFCPP_END_NAMESPACE
