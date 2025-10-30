// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../auth/AuthToken.hpp"
#include "../../ipf/InstrumentProfileReader.hpp"

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfileReader {
/* dxfg_instrument_profile_reader_t* */ JavaObjectHandle<InstrumentProfileReader> create();

std::int64_t getLastModified(
    /* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle);

bool wasComplete(
    /* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle);

/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLike &address);

/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLike &address, const StringLike &user, const StringLike &password);

/// dxfg_InstrumentProfileReader_readFromFile3
/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLike &address, const JavaObjectHandle<AuthToken> &token);

std::string resolveSourceURL(const StringLike &address);
} // namespace isolated::ipf::IsolatedInstrumentProfileReader

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()