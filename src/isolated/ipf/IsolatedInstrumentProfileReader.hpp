// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/auth/AuthToken.hpp"
#include "../../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfileReader.hpp"

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfileReader {
/* dxfg_v4_instrument_profile_reader_t* */ JavaObjectHandle<InstrumentProfileReader> create();

std::int64_t getLastModified(
    /* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle);

bool wasComplete(
    /* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle);

/* dxfg_v4_instrument_profile_list* */ void *
readFromFile(/* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             std::string_view address);

/* dxfg_v4_instrument_profile_list* */ void *
readFromFile(/* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             std::string_view address, std::string_view user, std::string_view password);

/// dxfg_v4_InstrumentProfileReader_readFromFileWithTokenAsHandles
/* dxfg_v4_instrument_profile_list* */ void *
readFromFile(/* dxfg_v4_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             std::string_view address, const JavaObjectHandle<AuthToken> &token);

std::string resolveSourceURL(std::string_view address);
} // namespace isolated::ipf::IsolatedInstrumentProfileReader

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()