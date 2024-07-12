// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <string>
#include <string_view>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../auth/AuthToken.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfileReader {
/* dxfg_instrument_profile_reader_t* */ JavaObjectHandle<InstrumentProfileReader> create();

std::int64_t getLastModified(
    /* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle);

bool wasComplete(
    /* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle);

/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLikeWrapper &address);

/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLikeWrapper &address, const StringLikeWrapper &user, const StringLikeWrapper &password);

/// dxfg_InstrumentProfileReader_readFromFile3
/* dxfg_instrument_profile_list* */ void *
readFromFile(/* dxfg_instrument_profile_reader_t * */ const JavaObjectHandle<InstrumentProfileReader> &handle,
             const StringLikeWrapper &address, const JavaObjectHandle<AuthToken> &token);

std::string resolveSourceURL(const StringLikeWrapper &address);
} // namespace isolated::ipf::IsolatedInstrumentProfileReader

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()