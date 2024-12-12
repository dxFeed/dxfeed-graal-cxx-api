// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/ipf/IsolatedInstrumentProfileReader.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfileReader {

/* dxfg_instrument_profile_reader_t* */ JavaObjectHandle<InstrumentProfileReader> create() {
    return dxfcpp::JavaObjectHandle<InstrumentProfileReader>{
        runGraalFunctionAndThrowIfNullptr(dxfg_InstrumentProfileReader_new)};
}

std::int64_t getLastModified(
    /* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileReader_getLastModified`. The handle is invalid");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfileReader_getLastModified,
                                                  static_cast<dxfg_instrument_profile_reader_t *>(handle.get()));
}

bool wasComplete(
    /* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileReader_wasComplete`. The handle is invalid");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfileReader_wasComplete,
                                                  static_cast<dxfg_instrument_profile_reader_t *>(handle.get())) == 1;
}

/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLikeWrapper &address) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileReader_readFromFile`. The handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfileReader_readFromFile, static_cast<dxfg_instrument_profile_reader_t *>(handle.get()),
        address.c_str()));
}

/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLikeWrapper &address, const StringLikeWrapper &user, const StringLikeWrapper &password) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileReader_readFromFile`. The handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfileReader_readFromFile2, static_cast<dxfg_instrument_profile_reader_t *>(handle.get()),
        address.c_str(), user.c_str(), password.c_str()));
}

/// dxfg_InstrumentProfileReader_readFromFile3
/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLikeWrapper &address, const JavaObjectHandle<AuthToken> &token) {
    if (!handle) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileReader_readFromFile`. The handle is invalid");
    }

    if (!token) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfileReader_readFromFile3`. The token is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfileReader_readFromFile3, static_cast<dxfg_instrument_profile_reader_t *>(handle.get()),
        address.c_str(), static_cast<dxfg_auth_token_t *>(token.get())));
}

std::string resolveSourceURL(const StringLikeWrapper &address) {
    auto resolvedURL =
        runGraalFunctionAndThrowIfNullptr(dxfg_InstrumentProfileReader_resolveSourceURL, address.c_str());

    auto result = dxfcpp::toString(resolvedURL);

    isolated::internal::IsolatedString::release(resolvedURL);

    return result;
}

} // namespace isolated::ipf::IsolatedInstrumentProfileReader

DXFCPP_END_NAMESPACE